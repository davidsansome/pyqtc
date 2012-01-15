#include "projects.h"

#include "closure.h"
#include "messagehandler.h"

#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>
#include <utils/qtcassert.h>

#include <QtDebug>

using namespace pyqtc;


Projects::Projects(WorkerPool<WorkerClient>* worker_pool, QObject* parent)
  : QObject(parent),
    worker_pool_(worker_pool)
{
  ProjectExplorer::ProjectExplorerPlugin* pe =
     ProjectExplorer::ProjectExplorerPlugin::instance();
  QTC_ASSERT(pe, return);

  ProjectExplorer::SessionManager* session = pe->session();
  QTC_ASSERT(session, return);

  connect(session, SIGNAL(projectAdded(ProjectExplorer::Project*)),
          SLOT(ProjectAdded(ProjectExplorer::Project*)));
  connect(session, SIGNAL(aboutToRemoveProject(ProjectExplorer::Project *)),
          this, SLOT(AboutToRemoveProject(ProjectExplorer::Project*)));
}

void Projects::ProjectAdded(ProjectExplorer::Project* project) {
  const QString project_root = project->projectDirectory();

  WorkerClient::ReplyType* reply =
      worker_pool_->NextHandler()->CreateProject(project_root);
  NewClosure(reply, SIGNAL(Finished(bool)),
             this, SLOT(CreateProjectFinished(WorkerClient::ReplyType*,QString)),
             reply, project_root);
}

void Projects::CreateProjectFinished(WorkerClient::ReplyType* reply,
                                     const QString& project_root) {
  reply->deleteLater();

  reply = worker_pool_->NextHandler()->RebuildSymbolIndex(project_root);
  connect(reply, SIGNAL(Finished(bool)), reply, SLOT(deleteLater()));
}

void Projects::AboutToRemoveProject(ProjectExplorer::Project* project) {
  WorkerClient::ReplyType* reply =
      worker_pool_->NextHandler()->DestroyProject(project->projectDirectory());

  connect(reply, SIGNAL(Finished(bool)), reply, SLOT(deleteLater()));
}
