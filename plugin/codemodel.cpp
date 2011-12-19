#include "closure.h"
#include "codemodel.h"
#include "codemodel.pb.h"
#include "messagehandler.h"
#include "workerpool.h"
#include "workerreply.h"

#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>
#include <utils/qtcassert.h>

#include <QtDebug>

using namespace pyqtc;

const char* CodeModel::kUnknownModuleName = "<unknown>";


CodeModel::CodeModel(WorkerPool* worker_pool, QObject* parent)
  : QObject(parent),
    worker_pool_(worker_pool)
{
  ProjectExplorer::ProjectExplorerPlugin *pe =
     ProjectExplorer::ProjectExplorerPlugin::instance();
  QTC_ASSERT(pe, return);

  ProjectExplorer::SessionManager *session = pe->session();
  QTC_ASSERT(session, return);

  connect(session, SIGNAL(projectAdded(ProjectExplorer::Project*)),
          SLOT(ProjectAdded(ProjectExplorer::Project*)));
  connect(session, SIGNAL(aboutToRemoveProject(ProjectExplorer::Project *)),
          this, SLOT(AboutToRemoveProject(ProjectExplorer::Project*)));
}

void CodeModel::ProjectAdded(ProjectExplorer::Project* project) {
  qDebug() << "Project added" << project->displayName();

  NewClosure(project, SIGNAL(fileListChanged()),
             this, SLOT(ProjectFilesChanged(ProjectExplorer::Project*)), project)
      ->SetSingleShot(false);

  UpdateProject(project);
}

void CodeModel::AboutToRemoveProject(ProjectExplorer::Project* project) {
  qDebug() << "Project removed" << project->displayName();
}

void CodeModel::ProjectFilesChanged(ProjectExplorer::Project* project) {
  UpdateProject(project);
}

void CodeModel::UpdateProject(ProjectExplorer::Project* project) {
  QSet<QString> filenames;

  // Parse files that were added to the project.
  foreach (const QString& filename, project->files(ProjectExplorer::Project::AllFiles)) {
    if (!filename.endsWith(".py"))
      continue;

    filenames << filename;

    if (files_.contains(filename))
      continue;

    WorkerReply* reply = worker_pool_->ParseFile(filename);
    NewClosure(reply, SIGNAL(Finished()),
               this, SLOT(ParseFileFinished(WorkerReply*,QString,ProjectExplorer::Project*)),
               reply, filename, project);
  }

  // Remove files that are no longer in the project
  for (FilesMap::iterator it = files_.begin() ; it != files_.end() ; ) {
    if (filenames.contains(it.key())) {
      ++it;
    } else {
      RemoveFile(it.value());
    }
  }
}

void CodeModel::ParseFileFinished(WorkerReply* reply, const QString& filename,
                                  ProjectExplorer::Project* project) {
  reply->deleteLater();

  if (files_.contains(filename)) {
    delete files_[filename];
  }

  const QString module_name = DottedModuleName(filename);

  AddFile(new File(filename, module_name, project,
                   reply->message().parse_file_response().module()));
}

void CodeModel::AddFile(File* file) {
  files_[file->filename()] = file;
  InitScopeRecursive(file, file->module_name());
}

void CodeModel::InitScopeRecursive(Scope* scope, const QString& dotted_name) {
  // Icon type
  CPlusPlus::Icons::IconType icon_type = CPlusPlus::Icons::UnknownIconType;

  switch (scope->type()) {
  case pb::Scope_Type_CLASS:
    icon_type = CPlusPlus::Icons::ClassIconType;
    break;

  case pb::Scope_Type_FUNCTION:
    if (scope->name().startsWith("_")) {
      icon_type = CPlusPlus::Icons::FuncPrivateIconType;
    } else {
      icon_type = CPlusPlus::Icons::FuncPublicIconType;
    }
    break;

  case pb::Scope_Type_MODULE:
    icon_type = CPlusPlus::Icons::NamespaceIconType;
    break;
  }

  scope->icon_ = icons_.iconForType(icon_type);

  // Dotted name
  if (scope->type() == pb::Scope_Type_MODULE) {
    scope->full_dotted_name_ = dotted_name;
  } else {
    scope->full_dotted_name_ = dotted_name + "." + scope->name();
  }

  // Add a reference to this type
  types_.insertMulti(scope->full_dotted_name_, scope);

  // Do child variables
  for (int i=0 ; i<scope->pb_->child_variable_size() ; ++i) {
    scope->child_variables_.append(&scope->pb_->child_variable(i));
  }

  // Do child scopes
  for (int i=0 ; i<scope->pb_->child_scope_size() ; ++i) {
    Scope* child_scope = new Scope(&scope->pb_->child_scope(i), scope, scope->module_);
    scope->child_scopes_.append(child_scope);
    InitScopeRecursive(child_scope, scope->full_dotted_name_);
  }
}

void CodeModel::RemoveFile(File* file) {
  RemoveScopeRecursive(file);

  files_.remove(file->filename());
  delete file;
}

void CodeModel::RemoveScopeRecursive(Scope* scope) {
  foreach (const Scope* child_scope_const, scope->child_scopes_) {
    Scope* child_scope = const_cast<Scope*>(child_scope_const);
    RemoveScopeRecursive(child_scope);
    delete child_scope;
  }

  types_.remove(scope->full_dotted_name_, scope);
}

QString CodeModel::DottedModuleName(const QString& filename) {
  // Guess the module name for a file by walking up the filesystem, adding the
  // names of any directories that contain a __init__.py.

  if (filename.isEmpty()) {
    return kUnknownModuleName;
  }

  int slash = filename.lastIndexOf('/');
  int dot = filename.lastIndexOf('.');

  if (slash == -1) {
    return kUnknownModuleName;
  }

  QString ret = filename.mid(slash + 1, dot - slash - 1);
  QString path = filename.left(slash);

  while (slash != -1) {
    if (!QFile::exists(path + "/__init__.py") &&
        !QFile::exists(path + "/__init__.pyc") &&
        !QFile::exists(path + "/__init__.pyo")) {
      break;
    }

    slash = path.lastIndexOf('/');
    ret.prepend(path.mid(slash + 1) + ".");
    path = path.left(slash);
  }

  return ret;
}

const File* CodeModel::FileByFilename(const QString& name) {
  FilesMap::const_iterator it = files_.find(name);
  if (it == files_.end())
    return NULL;
  return it.value();
}

const File* CodeModel::FileByModuleName(const QString& module_name) {
  for (FilesMap::const_iterator it = files_.begin() ; it != files_.end() ; ++it) {
    if (it.value()->module_name() == module_name) {
      return it.value();
    }
  }
  return NULL;
}

CodeModel::FilesMap CodeModel::AllFiles() {
  return files_;
}

const Scope* CodeModel::ScopeByTypeName(const QString& dotted_type_name,
                                        const Scope* relative_scope) {
  QMap<QString, Scope*>::const_iterator it = types_.find(dotted_type_name);
  if (it == types_.end()) {
    if (relative_scope) {
      return ScopeByTypeName(relative_scope->full_dotted_name() + "." + dotted_type_name);
    }
    return NULL;
  }
  return it.value();
}



File::File(const QString& filename, const QString& module_name,
           ProjectExplorer::Project* project, const pb::Scope& complete_pb)
  : Scope(&complete_pb_, NULL, this),
    filename_(filename),
    module_name_(module_name),
    project_(project)
{
  complete_pb_.CopyFrom(complete_pb);
}

Scope::Scope(const pb::Scope* pb, Scope* parent, File* module)
  : pb_(pb),
    module_(module),
    parent_(parent)
{
}

const Scope* Scope::GetChildScope(const QString& name) const {
  foreach (const Scope* child_scope, child_scopes_) {
    if (child_scope->name() == name) {
      return child_scope;
    }
  }
  return NULL;
}

const pb::Variable* Scope::GetChildVariable(const QString& name) const {
  foreach (const pb::Variable* child_variable, child_variables_) {
    if (child_variable->name() == name) {
      return child_variable;
    }
  }
  return NULL;
}

const Scope* Scope::FindLocalScope(int line_number, int indent_amount) const {
  for (int i=child_scopes_.count() - 1 ; i >= 0 ; --i) {
    const Scope* child = child_scopes_[i];
    if (child->declaration_pos().line() < line_number &&
        child->declaration_pos().column() < indent_amount) {
      return child->FindLocalScope(line_number, indent_amount);
    }
  }

  return this;
}

QIcon CodeModel::IconForVariable(const QString& variable_name) const {
  if (variable_name.startsWith("_")) {
    return icons_.iconForType(CPlusPlus::Icons::VarPrivateIconType);
  }
  return icons_.iconForType(CPlusPlus::Icons::VarPublicIconType);
}
