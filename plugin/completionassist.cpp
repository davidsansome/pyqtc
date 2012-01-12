#include "completionassist.h"
#include "workerclient.h"
#include "workerpool.h"

#include <coreplugin/ifile.h>
#include <texteditor/codeassist/basicproposalitem.h>
#include <texteditor/codeassist/basicproposalitemlistmodel.h>
#include <texteditor/codeassist/functionhintproposal.h>
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
  return new CompletionAssistProcessor(worker_pool_, &icons_);
}

CompletionAssistProcessor::CompletionAssistProcessor(
      WorkerPool<WorkerClient>* worker_pool,
      const CPlusPlus::Icons* icons)
  : worker_pool_(worker_pool),
    icons_(icons)
{
}

CPlusPlus::Icons::IconType CompletionAssistProcessor::IconTypeForProposal(
      const pb::CompletionResponse_Proposal& proposal) {
  const bool is_private = proposal.name().startsWith("_");

  // Keywords are treated differently
  switch (proposal.scope()) {
  case pb::CompletionResponse_Proposal_Scope_KEYWORD:
    return CPlusPlus::Icons::KeywordIconType;

  default:
    // Continue to look at the type
    break;
  }

  switch (proposal.type()) {
  case pb::CompletionResponse_Proposal_Type_INSTANCE:
    return is_private ?
          CPlusPlus::Icons::VarPrivateIconType :
          CPlusPlus::Icons::VarPublicIconType;

  case pb::CompletionResponse_Proposal_Type_CLASS:
    return CPlusPlus::Icons::ClassIconType;

  case pb::CompletionResponse_Proposal_Type_FUNCTION:
    return is_private ?
          CPlusPlus::Icons::FuncPrivateIconType :
          CPlusPlus::Icons::FuncPublicIconType;

  case pb::CompletionResponse_Proposal_Type_MODULE:
    return CPlusPlus::Icons::NamespaceIconType;
  }

  return CPlusPlus::Icons::UnknownIconType;
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

  if (response->has_calltip()) {
    return CreateCalltipProposal(response->insertion_position(),
                                 response->calltip());
  }

  if (response->proposal_size()) {
    return CreateCompletionProposal(response);
  }

  return NULL;
}

TextEditor::IAssistProposal* CompletionAssistProcessor::CreateCompletionProposal(
    const pb::CompletionResponse* response) {
  QList<TextEditor::BasicProposalItem*> items;

  foreach (const pb::CompletionResponse_Proposal& proposal,
           response->proposal()) {
    TextEditor::BasicProposalItem* item = new TextEditor::BasicProposalItem;
    item->setText(proposal.name());
    item->setIcon(icons_->iconForType(IconTypeForProposal(proposal)));

    items << item;
  }

  return new TextEditor::GenericProposal(
        response->insertion_position(),
        new TextEditor::BasicProposalItemListModel(items));
}

TextEditor::IAssistProposal* CompletionAssistProcessor::CreateCalltipProposal(
    int position, const QString& text) {
  FunctionHintProposalModel* model = new FunctionHintProposalModel(text);
  return new TextEditor::FunctionHintProposal(position, model);
}
