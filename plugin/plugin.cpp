#include "codemodel.h"
#include "plugin.h"
#include "pythonfilter.h"
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
  : worker_pool_(new WorkerPool(this)),
    code_model_(NULL)
{
}

Plugin::~Plugin() {
}

bool Plugin::initialize(const QStringList& arguments, QString* errorString) {
  Q_UNUSED(arguments)
  Q_UNUSED(errorString)

  code_model_ = new CodeModel(worker_pool_);

  addAutoReleasedObject(new PythonFilter(code_model_));
  addAutoReleasedObject(new PythonCurrentDocumentFilter(code_model_));
  addAutoReleasedObject(new PythonClassFilter(code_model_));
  addAutoReleasedObject(new PythonFunctionFilter(code_model_));

  return true;
}

void Plugin::extensionsInitialized() {
}

ExtensionSystem::IPlugin::ShutdownFlag Plugin::aboutToShutdown() {
  return SynchronousShutdown;
}

Q_EXPORT_PLUGIN2(pyqtc, Plugin)

