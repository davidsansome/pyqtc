#include "config.h"
#include "constants.h"
#include "completionassist.h"
#include "editorfactory.h"
#include "hoverhandler.h"
#include "plugin.h"
#include "projects.h"
#include "workerpool.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/icontext.h>
#include <coreplugin/mimedatabase.h>

#include <QAction>
#include <QMessageBox>
#include <QMainWindow>
#include <QMenu>

#include <QtDebug>
#include <QtPlugin>

using namespace pyqtc;

const char* Plugin::kJumpToDefinition = "pyqtc.JumpToDefinition";


inline void InitResources() {
  Q_INIT_RESOURCE(pyqtc);
}


Plugin::Plugin()
  : worker_pool_(new WorkerPool<WorkerClient>(this))
{
  InitResources();

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

  Core::ICore* core = Core::ICore::instance();

  if (!core->mimeDatabase()->addMimeTypes(
        QLatin1String(":/pythoneditor/PythonEditor.mimetypes.xml"), errorString))
      return false;

  addAutoReleasedObject(new Projects(worker_pool_));
  addAutoReleasedObject(new CompletionAssistProvider(worker_pool_));
  addAutoReleasedObject(new HoverHandler(worker_pool_));
  addAutoReleasedObject(new EditorFactory);

  Core::ActionManager* am = core->actionManager();
  Core::Context context(constants::kEditorId);
  Core::ActionContainer* menu = am->createMenu(constants::kMenuContext);

  QAction* action = new QAction(tr("Follow Symbol Under Cursor"), this);
  Core::Command* cmd = am->registerAction(
        action, constants::kJumpToDefinitionId, context);
  cmd->setDefaultKeySequence(QKeySequence(Qt::Key_F2));
  connect(action, SIGNAL(triggered()), this, SLOT(JumpToDefinition()));
  menu->addAction(cmd);

  return true;
}

void Plugin::extensionsInitialized() {
}

ExtensionSystem::IPlugin::ShutdownFlag Plugin::aboutToShutdown() {
  return SynchronousShutdown;
}

void Plugin::JumpToDefinition() {
  qDebug() << "Jump";
}

Q_EXPORT_PLUGIN2(pyqtc, Plugin)

