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

  ReplyType* RebuildSymbolIndex(const QString& project_root);
  ReplyType* UpdateSymbolIndex(const QString& file_path);

  ReplyType* Completion(const QString& file_path,
                        const QString& source_text,
                        int cursor_position);
  ReplyType* Tooltip(const QString& file_path,
                     const QString& source_text,
                     int cursor_position);
  ReplyType* DefinitionLocation(const QString& file_path,
                                const QString& source_text,
                                int cursor_position);

  ReplyType* Search(const QString& query,
                    const QString& file_path = QString(),
                    pb::SymbolType type = pb::ALL);
};

} // namespace

#endif // PYQTC_WORKERCLIENT_H
