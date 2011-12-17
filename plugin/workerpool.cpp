#include "closure.h"
#include "config.h"
#include "messagehandler.h"
#include "rpc.pb.h"
#include "workerpool.h"
#include "workerreply.h"

#include <QLocalServer>
#include <QLocalSocket>
#include <QThread>
#include <QtDebug>

using namespace pyqtc;

WorkerPool::WorkerPool(QObject* parent)
  : QObject(parent),
    next_worker_(0),
    next_id_(0)
{
  for (int i=0 ; i<QThread::idealThreadCount() ; ++i) {
    Worker worker;
    worker.server_ = new QLocalServer(this);

    // Find a unique name for this server's socket
    const QString name_prefix = QString("pyqtc-%1-").arg(
          quint64(worker.server_), 0, 16);
    int name_suffix = 0;
    forever {
      const QString name = name_prefix + QString::number(name_suffix);
      if (worker.server_->listen(name))
        break;
    }

    // Start the worker process
    worker.process_ = new QProcess(this);
    worker.process_->setProcessChannelMode(QProcess::ForwardedChannels);
    worker.process_->start("python", QStringList()
                           << config::kWorkerSourcePath
                           << worker.server_->fullServerName());

    // The handler gets created when the worker connects to the socket.
    worker.handler_ = NULL;
    workers_ << worker;

    connect(worker.process_, SIGNAL(error(QProcess::ProcessError)),
            SLOT(ProcessError(QProcess::ProcessError)));

    NewClosure(worker.server_, SIGNAL(newConnection()),
               this, SLOT(WorkerConnected(int)), i);
  }
}

WorkerPool::~WorkerPool() {
  foreach (const Worker& worker, workers_) {
    if (worker.process_->state() == QProcess::Running) {
      disconnect(worker.process_, 0, this, 0);
      worker.process_->terminate();
    }
  }

  foreach (const Worker& worker, workers_) {
    if (worker.process_->state() == QProcess::Running) {
      worker.process_->waitForFinished(1000);
      if (worker.process_->state() == QProcess::Running) {
        worker.process_->kill();
      }
    }
  }
}

void WorkerPool::WorkerConnected(int index) {
  Worker* worker = &workers_[index];

  qDebug() << "Worker" << index << "connected";

  worker->handler_ = new MessageHandler(worker->server_->nextPendingConnection(),
                                        this);

  connect(worker->handler_, SIGNAL(MessageArrived(pyqtc::Message)),
          SLOT(MessageArrived(pyqtc::Message)));

  // Send any queued messages
  while (!queued_requests_.isEmpty()) {
    Message message = queued_requests_.dequeue();
    SendMessage(&message);
  }
}

void WorkerPool::ProcessError(QProcess::ProcessError error) {
  qDebug() << "Process failed:" << error;
}

WorkerReply* WorkerPool::SendMessage(pyqtc::Message* message) {
  int id = -1;

  if (message->has_id()) {
    id = message->id();
  } else {
    id = next_id_ ++;
    message->set_id(id);
  }

  // Find a worker that is connected
  int using_worker = -1;

  for (int i=0 ; i<workers_.count() ; ++i) {
    const int worker_index = (next_worker_ + i) % workers_.count();
    if (workers_[worker_index].handler_) {
      using_worker = worker_index;
      break;
    }
  }
  next_worker_ = (next_worker_ + 1) % workers_.count();

  // Send the message or queue it
  if (using_worker == -1) {
    qDebug() << "Queueing message because no workers are connected yet";
    queued_requests_.enqueue(*message);
  } else {
    workers_[using_worker].handler_->SendMessage(*message);
  }

  WorkerReply* reply = new WorkerReply(id);
  pending_replies_[id] = reply;

  return reply;
}

WorkerReply* WorkerPool::ParseFile(const QString& filename) {
  Message request;
  request.mutable_parse_file_request()->set_filename(
        DataCommaSizeFromQString(filename));

  return SendMessage(&request);
}

void WorkerPool::MessageArrived(const pyqtc::Message& message) {
  if (!message.has_id()) {
    qDebug() << "Received message with no ID:" <<
                QStringFromStdString(message.DebugString());
    return;
  }

  const int id = message.id();
  WorkerReply* reply = pending_replies_.take(id);
  if (!reply) {
    qDebug() << "Received unexpected message:" <<
                QStringFromStdString(message.DebugString());
    return;
  }

  reply->SetReply(message);
}
