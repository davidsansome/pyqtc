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
      delete it.value();
      it = files_.erase(it);
    }
  }
}

void CodeModel::ParseFileFinished(WorkerReply* reply, const QString& filename,
                                  ProjectExplorer::Project* project) {
  reply->deleteLater();

  if (files_.contains(filename)) {
    delete files_[filename];
  }

  File* file = new File(filename, project, this,
                        reply->message().parse_file_response().module());
  files_[filename] = file;
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
    if (!QFile::exists(path + "/init.py") &&
        !QFile::exists(path + "/init.pyc") &&
        !QFile::exists(path + "/init.pyo")) {
      break;
    }

    slash = path.lastIndexOf('/');
    ret.prepend(path.mid(slash + 1) + ".");
    path = path.left(slash);
  }

  return ret;
}

File* CodeModel::FindFile(const QString& name) {
  FilesMap::const_iterator it = files_.find(name);
  if (it == files_.end())
    return NULL;
  return it.value();
}

CodeModel::FilesMap CodeModel::AllFiles() {
  return files_;
}



File::File(const QString& filename, ProjectExplorer::Project* project,
           CodeModel* model, const pb::Scope& pb)
  : Scope(&complete_pb_, NULL, this),
    filename_(filename),
    module_name_(CodeModel::DottedModuleName(filename)),
    model_(model),
    project_(project)
{
  complete_pb_.CopyFrom(pb);
  Init();
}

Scope::Scope(const pb::Scope* pb, Scope* parent, File* module)
  : pb_(pb),
    module_(module),
    parent_(parent)
{
}

void Scope::Init() {
  CPlusPlus::Icons::IconType icon_type = CPlusPlus::Icons::UnknownIconType;

  switch (type()) {
  case pb::Scope_Type_CLASS:
    icon_type = CPlusPlus::Icons::ClassIconType;
    break;

  case pb::Scope_Type_FUNCTION:
    if (name().startsWith("_")) {
      icon_type = CPlusPlus::Icons::FuncPrivateIconType;
    } else {
      icon_type = CPlusPlus::Icons::FuncPublicIconType;
    }
    break;

  case pb::Scope_Type_MODULE:
    icon_type = CPlusPlus::Icons::NamespaceIconType;
    break;
  }

  icon_ = module_->model_->icons_.iconForType(icon_type);

  for (int i=0 ; i<pb_->child_scope_size() ; ++i) {
    Scope* child_scope = new Scope(&pb_->child_scope(i), this, module_);
    child_scope->Init();
    children_.append(child_scope);
  }
}

Scope::~Scope() {
  qDeleteAll(children_);
}

QString Scope::DottedName() const {
  if (type() == pb::Scope_Type_MODULE) {
    return module_->module_name();
  }

  if (parent_) {
    return parent_->DottedName() + "." + name();
  }

  return name();
}

QString Scope::ParentDottedName() const {
  if (parent_)
    return parent_->DottedName();
  return module_->module_name();
}
