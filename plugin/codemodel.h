#ifndef PYQTC_CODEMODEL_H
#define PYQTC_CODEMODEL_H

#include <QMultiMap>
#include <QObject>

#include "codemodel.pb.h"


namespace ProjectExplorer {
  class Project;
}


namespace pyqtc {

class WorkerPool;
class WorkerReply;

class CodeModel : public QObject {
  Q_OBJECT

public:
  CodeModel(WorkerPool* worker_pool, QObject* parent = 0);

  struct File {
    ProjectExplorer::Project* project_;
    Scope scope_;
  };

  typedef QMap<QString, File> FilesMap;
  const FilesMap& files() const { return files_; }

private slots:
  void ProjectAdded(ProjectExplorer::Project* project);
  void AboutToRemoveProject(ProjectExplorer::Project* project);
  void ProjectFilesChanged(ProjectExplorer::Project* project);

  void ParseFileFinished(WorkerReply* reply, const QString& filename);

private:
  void UpdateProject(ProjectExplorer::Project* project);

private:
  WorkerPool* worker_pool_;

  FilesMap files_;
};

} // namespace pyqtc

#endif // PYQTC_CODEMODEL_H
