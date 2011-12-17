#ifndef WORKERREPLY_H
#define WORKERREPLY_H

#include <QObject>

#include "rpc.pb.h"

namespace pyqtc {

class WorkerReply : public QObject {
  Q_OBJECT

public:
  WorkerReply(int id);

  int id() const { return id_; }
  bool is_finished() const { return finished_; }
  const Message& message() const { return message_; }

  void SetReply(const Message& message);

signals:
  void Finished();

private:
  int id_;
  bool finished_;
  Message message_;
};

} // namespace

#endif // WORKERREPLY_H
