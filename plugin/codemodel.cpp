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
               this, SLOT(ParseFileFinished(WorkerReply*,QString)),
               reply, filename);
  }

  // Remove files that are no longer in the project
  for (FilesMap::iterator it = files_.begin() ; it != files_.end() ; ) {
    if (filenames.contains(it.key())) {
      ++it;
    } else {
      it = files_.erase(it);
    }
  }
}

void CodeModel::ParseFileFinished(WorkerReply* reply, const QString& filename) {
  qDebug() << "Parse finished" << filename
           << QStringFromStdString(reply->message().parse_file_response().DebugString());

  reply->deleteLater();
}
