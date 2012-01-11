#include "workerclient.h"

using namespace pyqtc;


WorkerClient::WorkerClient(QIODevice* device, QObject* parent)
    : AbstractMessageHandler(device, parent)
{
}

WorkerClient::ReplyType* WorkerClient::CreateProject(const QString& project_root) {
  pb::Message message;
  pb::CreateProjectRequest* req = message.mutable_create_project_request();

  req->set_project_root(project_root);

  return SendMessageWithReply(&message);
}

WorkerClient::ReplyType* WorkerClient::DestroyProject(const QString& project_root) {
  pb::Message message;
  pb::DestroyProjectRequest* req = message.mutable_destroy_project_request();

  req->set_project_root(project_root);

  return SendMessageWithReply(&message);
}

WorkerClient::ReplyType* WorkerClient::Completion(const QString& file_path,
                                                  const QString& source_text,
                                                  int cursor_position) {
  pb::Message message;
  pb::CompletionRequest* req = message.mutable_completion_request();

  req->set_file_path(file_path);
  req->set_source_text(source_text);
  req->set_cursor_position(cursor_position);

  return SendMessageWithReply(&message);
}
