#include "completionassist.h"
#include "workerclient.h"
#include "workerpool.h"

#include <coreplugin/ifile.h>
#include <texteditor/codeassist/basicproposalitem.h>
#include <texteditor/codeassist/basicproposalitemlistmodel.h>
#include <texteditor/codeassist/genericproposal.h>
#include <texteditor/codeassist/iassistinterface.h>
#include <texteditor/convenience.h>

#include <QTextDocument>
#include <QtDebug>

using namespace pyqtc;

CompletionAssistProvider::CompletionAssistProvider(
    WorkerPool<WorkerClient>* worker_pool)
  : worker_pool_(worker_pool)
{
}

bool CompletionAssistProvider::supportsEditor(const QString& editorId) const {
  return true;
}

int CompletionAssistProvider::activationCharSequenceLength() const {
  return 1;
}

bool CompletionAssistProvider::isActivationCharSequence(const QString& sequence) const {
  return sequence == ".";
}

TextEditor::IAssistProcessor* CompletionAssistProvider::createProcessor() const {
  return new CompletionAssistProcessor(worker_pool_);
}

CompletionAssistProcessor::CompletionAssistProcessor(
    WorkerPool<WorkerClient>* worker_pool)
  : worker_pool_(worker_pool)
{
}

TextEditor::IAssistProposal* CompletionAssistProcessor::perform(
    const TextEditor::IAssistInterface* interface) {
  QScopedPointer<const TextEditor::IAssistInterface> scoped_interface(interface);

  WorkerClient::ReplyType* reply =
      worker_pool_->NextHandler()->Completion(
        interface->file()->fileName(),
        interface->document()->toPlainText(),
        interface->position());
  reply->WaitForFinished();
  reply->deleteLater();

  if (!reply->is_successful())
    return NULL;

  const pb::CompletionResponse* response = &reply->message().completion_response();

  QList<TextEditor::BasicProposalItem*> items;

  foreach (const pb::CompletionResponse_Proposal& proposal, response->proposal()) {
    TextEditor::BasicProposalItem* item = new TextEditor::BasicProposalItem;
    item->setText(proposal.name());

    items << item;
  }

  return new TextEditor::GenericProposal(
        response->insertion_position(),
        new TextEditor::BasicProposalItemListModel(items));
}
