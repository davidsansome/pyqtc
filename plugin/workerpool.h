#ifndef WORKERPOOL_H
#define WORKERPOOL_H

#include <QMap>
#include <QObject>
#include <QProcess>
#include <QQueue>

class QLocalServer;


namespace pyqtc {

namespace pb {
  class Message;
}

class MessageHandler;
class WorkerReply;

class WorkerPool : public QObject {
  Q_OBJECT

public:
  WorkerPool(QObject* parent = 0);
  ~WorkerPool();

  WorkerReply* ParseFile(const QString& filename);
  WorkerReply* GetPythonPath();

private slots:
  void ProcessError(QProcess::ProcessError error);
  void MessageArrived(const pyqtc::pb::Message& message);

  void WorkerConnected(int index);

private:
  WorkerReply* SendNewMessage(pyqtc::pb::Message* message);
  void SendMessage(pyqtc::pb::Message* message);

private:
  struct Worker {
    QProcess* process_;
    QLocalServer* server_;
    MessageHandler* handler_;
  };

  QList<Worker> workers_;
  int next_worker_;
  int next_id_;

  QMap<int, WorkerReply*> pending_replies_;
  QQueue<pyqtc::pb::Message> queued_requests_;
};

} // namespace

#endif // WORKERPOOL_H
