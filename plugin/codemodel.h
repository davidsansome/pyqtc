#ifndef PYQTC_CODEMODEL_H
#define PYQTC_CODEMODEL_H

#include <QIcon>
#include <QMultiMap>
#include <QObject>

#include <cplusplus/Icons.h>

#include "codemodel.pb.h"


namespace ProjectExplorer {
  class Project;
}


namespace pyqtc {

class CodeModel;
class File;
class WorkerPool;
class WorkerReply;

class Scope {
  friend class CodeModel;

public:
  Scope(const pb::Scope* pb, Scope* parent, File* module);

  // The position in the file that this type was declared.
  const pb::Position& declaration_pos() const { return pb_->declaration_pos(); }

  // The name of this type, without any dots.
  const QString& name() const { return pb_->name(); }

  // The complete name of this type, including dotted module name and dotted
  // name within the module.
  const QString& full_dotted_name() const { return full_dotted_name_; }

  // What kind of type this is - module, class or function.
  pb::Scope_Type type() const { return pb_->type(); }

  // The type that contains this type within this module.  Modules do not have
  // parents.
  Scope* parent() const { return parent_; }

  // The module that contains this type.  If this type is a module then module()
  // returns itself.
  File* module() const { return module_; }

  // An icon for this type.
  QIcon icon() const { return icon_; }

  const QList<const Scope*>& child_scopes() const { return child_scopes_; }
  const QList<const pb::Variable*>& child_variables() const { return child_variables_; }

  const Scope* GetChildScope(const QString& name) const;
  const pb::Variable* GetChildVariable(const QString& name) const;

  // Returns a scope that might correspond to the given line in the file,
  // assuming the start of the line is indented by the given number of spaces.
  const Scope* FindLocalScope(int line_number, int indent_amount) const;

protected:
  const pb::Scope* pb_;

  // Top-level parent, can point to this if I am a module.
  File* module_;

  // Immediate parent, can be null if I am a module.
  Scope* parent_;

  // Immediate children.  I own these.
  QList<const Scope*> child_scopes_;

  // Computed variables
  QIcon icon_;
  QString full_dotted_name_;
  QList<const pb::Variable*> child_variables_;
};


class File : public Scope {
  friend class CodeModel;

public:
  File(const QString& filename, const QString& module_name,
       ProjectExplorer::Project* project, const pb::Scope& complete_pb);

  const QString& filename() const { return filename_; }
  const QString& module_name() const { return module_name_; }
  ProjectExplorer::Project* project() const { return project_; }

private:
  // Variables set by constructor
  QString filename_;
  QString module_name_;
  ProjectExplorer::Project* project_;
  pb::Scope complete_pb_;
};


class CodeModel : public QObject {
  Q_OBJECT
  friend class File;
  friend class Scope;

public:
  CodeModel(WorkerPool* worker_pool, QObject* parent = 0);

  static const char* kUnknownModuleName;
  typedef QMap<QString, File*> FilesMap;

  const File* FileByFilename(const QString& file_name);
  const File* FileByModuleName(const QString& module_name);
  FilesMap AllFiles();

  const Scope* ScopeByTypeName(const QString& dotted_type_name,
                               const Scope* relative_scope = NULL);

  QIcon IconForVariable(const QString& variable_name) const;

private slots:
  void ProjectAdded(ProjectExplorer::Project* project);
  void AboutToRemoveProject(ProjectExplorer::Project* project);
  void ProjectFilesChanged(ProjectExplorer::Project* project);

  void ParseFileFinished(WorkerReply* reply, const QString& filename,
                         ProjectExplorer::Project* project);

private:
  void UpdateProject(ProjectExplorer::Project* project);
  static QString DottedModuleName(const QString& filename);

  void AddFile(File* file);
  void RemoveFile(File* file);

  void InitScopeRecursive(Scope* scope, const QString& dotted_name);
  void RemoveScopeRecursive(Scope* scope);

private:
  WorkerPool* worker_pool_;

  CPlusPlus::Icons icons_;
  QMap<QString, File*> files_;
  QMultiMap<QString, Scope*> types_;
};

} // namespace pyqtc

#endif // PYQTC_CODEMODEL_H
