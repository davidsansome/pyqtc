#include "config.h"
#include "completionassist.h"
#include "hoverhandler.h"
#include "plugin.h"
#include "projects.h"
#include "workerpool.h"

#include <coreplugin/icore.h>
#include <coreplugin/icontext.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/coreconstants.h>

#include <QAction>
#include <QMessageBox>
#include <QMainWindow>
#include <QMenu>

#include <QtDebug>
#include <QtPlugin>

using namespace pyqtc;

Plugin::Plugin()
  : worker_pool_(new WorkerPool<WorkerClient>(this))
{
  worker_pool_->SetExecutableName(config::kWorkerSourcePath);
  worker_pool_->SetWorkerCount(1);
  worker_pool_->SetLocalServerName("pyqtc");
  worker_pool_->Start();
}

Plugin::~Plugin() {
}

bool Plugin::initialize(const QStringList& arguments, QString* errorString) {
  Q_UNUSED(arguments)
  Q_UNUSED(errorString)

  addAutoReleasedObject(new Projects(worker_pool_));
  addAutoReleasedObject(new CompletionAssistProvider(worker_pool_));
  addAutoReleasedObject(new HoverHandler(worker_pool_));

  return true;
}

void Plugin::extensionsInitialized() {
}

ExtensionSystem::IPlugin::ShutdownFlag Plugin::aboutToShutdown() {
  return SynchronousShutdown;
}

Q_EXPORT_PLUGIN2(pyqtc, Plugin)

