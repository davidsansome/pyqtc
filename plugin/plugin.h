#ifndef PYQTC_PLUGIN_H
#define PYQTC_PLUGIN_H

#include <extensionsystem/iplugin.h>

namespace pyqtc {

class CodeModel;
class WorkerPool;

class Plugin : public ExtensionSystem::IPlugin {
  Q_OBJECT

public:
  Plugin();
  ~Plugin();

  bool initialize(const QStringList& arguments, QString* errorString);
  void extensionsInitialized();
  ShutdownFlag aboutToShutdown();

private:
  WorkerPool* worker_pool_;
  CodeModel* code_model_;
};

} // namespace pyqtc

#endif // PYQTC_PLUGIN_H

