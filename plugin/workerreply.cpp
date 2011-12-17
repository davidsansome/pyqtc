#include "workerreply.h"

using namespace pyqtc;

WorkerReply::WorkerReply(int id)
  : id_(id),
    finished_(false)
{
}

void WorkerReply::SetReply(const pb::Message& message) {
  Q_ASSERT(!finished_);

  message_.MergeFrom(message);
  finished_ = true;

  emit Finished();
}
