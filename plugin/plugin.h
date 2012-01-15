#ifndef PYQTC_PLUGIN_H
#define PYQTC_PLUGIN_H

#include "workerclient.h"
#include "workerpool.h"

#include <extensionsystem/iplugin.h>

namespace pyqtc {

class PythonIcons;

class Plugin : public ExtensionSystem::IPlugin {
  Q_OBJECT

public:
  Plugin();
  ~Plugin();

  bool initialize(const QStringList& arguments, QString* errorString);
  void extensionsInitialized();
  ShutdownFlag aboutToShutdown();

private slots:
  void JumpToDefinition();
  void JumpToDefinitionFinished(WorkerClient::ReplyType* reply);

private:
  static const char* kJumpToDefinition;

  WorkerPool<WorkerClient>* worker_pool_;
  PythonIcons* icons_;
};

} // namespace pyqtc

#endif // PYQTC_PLUGIN_H

