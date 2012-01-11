#ifndef PYQTC_WORKERCLIENT_H
#define PYQTC_WORKERCLIENT_H

#include "messagehandler.h"
#include "rpc.pb.h"

namespace pyqtc {

class WorkerClient : public AbstractMessageHandler<pb::Message> {
public:
  WorkerClient(QIODevice* device, QObject* parent);

  ReplyType* CreateProject(const QString& project_root);
  ReplyType* DestroyProject(const QString& project_root);
  ReplyType* Completion(const QString& file_path,
                        const QString& source_text,
                        int cursor_position);
};

} // namespace

#endif // PYQTC_WORKERCLIENT_H
