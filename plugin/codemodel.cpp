#include "codemodel.h"

#include "closure.h"
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


uint qHash(const RecursionGuardEntry& entry) {
  return qHash(&entry.type_) ^
         qHash(entry.locals_) ^
         qHash(entry.globals_);
}

bool RecursionGuardEntry::operator ==(const RecursionGuardEntry& other) const {
  return type_ == other.type_ &&
         locals_ == other.locals_ &&
         globals_ == other.globals_;
}


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
  
  // Get the current pythonpath
  WorkerReply* reply = worker_pool->GetPythonPath();
  NewClosure(reply, SIGNAL(Finished()),
             this, SLOT(GetPythonPathFinished(WorkerReply*)),
             reply);
}

bool CodeModel::PathHasInitPy(const QString& path) {
  return QFile::exists(path + "/__init__.py") ||
         QFile::exists(path + "/__init__.pyc") ||
         QFile::exists(path + "/__init__.pyo");
}

void CodeModel::GetPythonPathFinished(WorkerReply* reply) {
  reply->deleteLater();
  
  // Parse all the files in the pythonpath
  foreach (const QString& path,
           reply->message().get_python_path_response().path_entry()) {
    WalkPythonPath(path, path);
  }
}

void CodeModel::WalkPythonPath(const QString& root_path, const QString& path) {
  // Check for an __init__.py in this directory if it's a package.
  if ((root_path != path) && !PathHasInitPy(path)) {
    return;
  }
  
  // Find .py files in this directory
  QDirIterator py_file_it(
        path, QStringList() << "*.py",
        QDir::Files | QDir::NoDotAndDotDot | QDir::Readable,
        QDirIterator::FollowSymlinks);
  
  while (py_file_it.hasNext()) {
    const QString filename = py_file_it.next();
    const QString module_name = DottedModuleName(filename, root_path);

    WorkerReply* reply = worker_pool_->ParseFile(filename, module_name);
    NewClosure(reply, SIGNAL(Finished()),
               this, SLOT(ParseFileFinished(WorkerReply*,QString,QString,ProjectExplorer::Project*)),
               reply, filename, module_name, NULL);
  }
  
  // Find subdirectories in this directory
  QDirIterator subdirectory_it(
        path, QDir::Dirs | QDir::NoDotAndDotDot | QDir::Readable,
        QDirIterator::FollowSymlinks);
  
  while (subdirectory_it.hasNext()) {
    WalkPythonPath(root_path, subdirectory_it.next());
  }
}

void CodeModel::ProjectAdded(ProjectExplorer::Project* project) {
  NewClosure(project, SIGNAL(fileListChanged()),
             this, SLOT(ProjectFilesChanged(ProjectExplorer::Project*)), project)
      ->SetSingleShot(false);

  UpdateProject(project);
}

void CodeModel::AboutToRemoveProject(ProjectExplorer::Project* project) {
  // TODO
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
    
    const QString module_name = DottedModuleName(filename);

    WorkerReply* reply = worker_pool_->ParseFile(filename, module_name);
    NewClosure(reply, SIGNAL(Finished()),
               this, SLOT(ParseFileFinished(WorkerReply*,QString,QString,ProjectExplorer::Project*)),
               reply, filename, module_name, project);
  }

  // Remove files that are no longer in the project
  for (FilesMap::iterator it = files_.begin() ; it != files_.end() ; ) {
    if (it.value()->project() != project || filenames.contains(it.key())) {
      ++it;
    } else {
      RemoveFile(it.value());
    }
  }
}

void CodeModel::ParseFileFinished(WorkerReply* reply, const QString& filename,
                                  const QString& module_name,
                                  ProjectExplorer::Project* project) {
  reply->deleteLater();

  if (files_.contains(filename)) {
    delete files_[filename];
  }
  
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

  switch (scope->kind()) {
  case pb::Scope_Kind_CLASS:
    icon_type = CPlusPlus::Icons::ClassIconType;
    break;

  case pb::Scope_Kind_FUNCTION:
    if (scope->name().startsWith("_")) {
      icon_type = CPlusPlus::Icons::FuncPrivateIconType;
    } else {
      icon_type = CPlusPlus::Icons::FuncPublicIconType;
    }
    break;

  case pb::Scope_Kind_MODULE:
    icon_type = CPlusPlus::Icons::NamespaceIconType;
    break;
  }

  scope->icon_ = icons_.iconForType(icon_type);

  // Dotted name
  if (scope->kind() == pb::Scope_Kind_MODULE) {
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
    if (!PathHasInitPy(path))
      break;

    slash = path.lastIndexOf('/');
    ret.prepend(path.mid(slash + 1) + ".");
    path = path.left(slash);
  }

  return ret;
}

QString CodeModel::DottedModuleName(const QString& filename,
                                    const QString& path_directory) {
  // Get the module name for a file that is inside a PYTHONPATH directory.
  
  if (filename.isEmpty() || path_directory.isEmpty()) {
    return kUnknownModuleName;
  }
  
  const QDir path(path_directory);
  QString relative_filename = path.relativeFilePath(filename);
  
  // Remove the .py extension
  const int dot_py = relative_filename.lastIndexOf(".py");
  relative_filename = relative_filename.mid(0, dot_py);
  
  const QStringList parts = relative_filename.split('/', QString::SkipEmptyParts);
  return parts.join(".");
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

QString CodeModel::ResolveType(const pb::Type& type,
                               const Scope* locals, const Scope* globals,
                               const Scope** locals_out, const Scope** globals_out,
                               RecursionGuard* recursion_guard) const {
  *locals_out = NULL;
  *globals_out = NULL;
  QString type_id;
  
  if (type.reference_size() == 0)
    return type_id;
  
  RecursionGuard local_recursion_guard;
  if (recursion_guard == NULL) {
    recursion_guard = &local_recursion_guard;
  }
  
  RecursionGuardEntry entry;
  entry.type_ = &type;
  entry.locals_ = locals;
  entry.globals_ = globals;
  
  if (recursion_guard->contains(entry)) {
    return type_id;
  }
  recursion_guard->insert(entry);
  
  for (int i=0 ; i<type.reference_size() ; ++i) {
    const Scope* next_locals = NULL;
    const Scope* next_globals = NULL;
    
    type_id = ResolveTypeRef(type.reference(i),
                             locals, globals,
                             &next_locals, &next_globals,
                             recursion_guard);
    
    locals = next_locals;
    globals = next_globals;
  }
  
  *locals_out = locals;
  *globals_out = globals;
  return type_id;
}

QString CodeModel::ResolveTypeRef(const pb::Type_Reference& ref,
                                  const Scope* locals, const Scope* globals,
                                  const Scope** locals_out, const Scope** globals_out,
                                  RecursionGuard* recursion_guard) const {
  *locals_out = NULL;
  *globals_out = NULL;
  
  QString ret;
  const pb::Type* ret_type = NULL;
  
  switch (ref.kind()) {
  case pb::Type_Reference_Kind_ABSOLUTE_TYPE_ID:
    ret = ref.name();
    break;
    
  case pb::Type_Reference_Kind_SCOPE_LOOKUP:
  case pb::Type_Reference_Kind_SCOPE_CALL:
    // Look for this type in all the scopes we can see.
    foreach (const Scope* scope, LookupScopes(locals, globals, recursion_guard)) {
      const Scope* child_scope = scope->GetChildScope(ref.name());
      if (child_scope) {
        if (ref.kind() == pb::Type_Reference_Kind_SCOPE_LOOKUP) {
          ret = child_scope->full_dotted_name();
          *locals_out = child_scope;
          *globals_out = child_scope->module();
        } else if (ref.kind() == pb::Type_Reference_Kind_SCOPE_CALL) {
          ret_type = &child_scope->pb().call_type();
        }
        break;
      }
      
      const pb::Variable* child_variable = scope->GetChildVariable(ref.name());
      if (child_variable) {
        ret_type = &child_variable->type();
        break;
      }
    }
    break;
  }
  
  if (!ret.isEmpty() && *locals_out == NULL && *globals_out == NULL) {
    // We know the name of the type but we need to find its scope.
    QMultiMap<QString, Scope*>::const_iterator it = types_.find(ret);
    if (it != types_.end()) {
      *locals_out = it.value();
      *globals_out = it.value()->module();
    }
  }
  
  if (ret_type && *locals_out == NULL && *globals_out == NULL) {
    // We know the type descriptor but we need to resolve it to a type.
    ret = ResolveType(*ret_type, locals, globals,
                      locals_out, globals_out, recursion_guard);
  }
  
  return ret;
}

QList<const Scope*> CodeModel::LookupScopes(const Scope* locals, const Scope* globals,
                                            RecursionGuard* recursion_guard) const {
  // This is probably slow and ought to be cached (or somehow made into a
  // generator like the Python implementation).
  QList<const Scope*> ret;
  
  if (locals) {
    ret << locals;
    
    for (int i=0 ; i<locals->pb().base_type_size() ; ++i) {
      const pb::Type& base = locals->pb().base_type(i);
      const Scope* new_locals = NULL;
      const Scope* new_globals = NULL;
      
      // Find this base
      ResolveType(base,
                  NULL, globals,
                  &new_locals, &new_globals,
                  recursion_guard);
      
      // Add this base and all its bases
      ret.append(LookupScopes(new_locals, new_globals, recursion_guard));
    }
  }
  
  if (globals) {
    ret << globals;
  }
  
  return ret;
}
