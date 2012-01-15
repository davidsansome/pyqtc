#ifndef PYQTC_PROJECTS_H
#define PYQTC_PROJECTS_H

#include <QIcon>
#include <QMultiMap>
#include <QObject>

#include <cplusplus/Icons.h>

#include "workerclient.h"
#include "workerpool.h"


namespace ProjectExplorer {
  class Project;
}

namespace pyqtc {

class WorkerClient;

class Projects : public QObject {
  Q_OBJECT

public:
  Projects(WorkerPool<WorkerClient>* worker_pool, QObject* parent = 0);

private slots:
  void ProjectAdded(ProjectExplorer::Project* project);
  void AboutToRemoveProject(ProjectExplorer::Project* project);

  void CreateProjectFinished(WorkerClient::ReplyType* reply,
                             const QString& project_root);

private:
  WorkerPool<WorkerClient>* worker_pool_;
};

} // namespace pyqtc

#endif // PYQTC_PROJECTS_H
