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
public:
  Scope(const pb::Scope* pb, Scope* parent, File* module);
  ~Scope();

  const pb::Position& declaration_pos() const { return pb_->declaration_pos(); }
  const QString& name() const { return pb_->name(); }
  pb::Scope_Type type() const { return pb_->type(); }

  Scope* parent() const { return parent_; }
  const QList<Scope*>& children() const { return children_; }
  QIcon icon() const { return icon_; }

  QString DottedName() const;
  QString ParentDottedName() const;

protected:
  void Init();

private:
  Q_DISABLE_COPY(Scope);

  const pb::Scope* pb_;
  File* module_;
  Scope* parent_;
  QList<Scope*> children_;

  QIcon icon_;
};


class File : public Scope {
  friend class Scope;

public:
  File(const QString& filename, ProjectExplorer::Project* project,
       CodeModel* model, const pb::Scope& pb);

  const QString& filename() const { return filename_; }
  const QString& module_name() const { return module_name_; }
  ProjectExplorer::Project* project() const { return project_; }

private:
  QString filename_;
  QString module_name_;
  ProjectExplorer::Project* project_;
  CodeModel* model_;

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

  File* FindFile(const QString& name);
  FilesMap AllFiles();

private slots:
  void ProjectAdded(ProjectExplorer::Project* project);
  void AboutToRemoveProject(ProjectExplorer::Project* project);
  void ProjectFilesChanged(ProjectExplorer::Project* project);

  void ParseFileFinished(WorkerReply* reply, const QString& filename,
                         ProjectExplorer::Project* project);

private:
  void UpdateProject(ProjectExplorer::Project* project);
  static QString DottedModuleName(const QString& filename);

private:
  WorkerPool* worker_pool_;

  CPlusPlus::Icons icons_;
  FilesMap files_;
};

} // namespace pyqtc

#endif // PYQTC_CODEMODEL_H
