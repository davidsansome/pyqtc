#include "completionassist.h"
#include "workerclient.h"
#include "workerpool.h"

#include <coreplugin/ifile.h>
#include <texteditor/codeassist/basicproposalitem.h>
#include <texteditor/codeassist/basicproposalitemlistmodel.h>
#include <texteditor/codeassist/functionhintproposal.h>
#include <texteditor/codeassist/functionhintproposalwidget.h>
#include <texteditor/codeassist/genericproposal.h>
#include <texteditor/codeassist/iassistinterface.h>
#include <texteditor/convenience.h>

#include <QApplication>
#include <QStack>
#include <QTextDocument>
#include <QtDebug>

using namespace pyqtc;

CompletionAssistProvider::CompletionAssistProvider(
    WorkerPool<WorkerClient>* worker_pool)
  : worker_pool_(worker_pool)
{
}

bool CompletionAssistProvider::supportsEditor(const Core::Id &editorId) const {
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
  FunctionHintProposalModel* model =
      new FunctionHintProposalModel(text);
  return new TextEditor::FunctionHintProposal(position, model);
}


FunctionHintProposalModel::FunctionHintProposalModel(const QString& text)
  : text_(text),
    current_arg_(0)
{
}

QString FunctionHintProposalModel::text(int index) const {
  const int open_paren    = text_.indexOf('(');
  if (open_paren == -1) {
    return text_;
  }

  const int close_paren   = text_.lastIndexOf(')');
  const int last_dot      = qMax(0, text_.lastIndexOf('.', open_paren));
  const QString args_str  = text_.mid(open_paren + 1,
                                     close_paren - open_paren - 1);
  const QStringList args  = args_str.split(',');

  QStringList rich_args;
  for (int i=0 ; i<args.count() ; ++i) {
    const QString arg_trimmed = Qt::escape(args[i].trimmed());

    if (current_arg_ == i) {
      rich_args << QString("<b>%1</b>").arg(arg_trimmed);
    } else {
      rich_args << arg_trimmed;
    }
  }

  // Pick a color between the tooltip background and foreground for the module
  // name.
  const QPalette& palette = qApp->palette();
  const QColor& foreground = palette.color(QPalette::ToolTipText);

  return QString("<font style=\"color: rgba(%1, %2, %3, 75%)\">%4</font>%5(%6)").arg(
        QString::number(foreground.red()),
        QString::number(foreground.green()),
        QString::number(foreground.blue()),
        Qt::escape(text_.left(last_dot)),
        Qt::escape(text_.mid(last_dot, open_paren - last_dot)),
        rich_args.join(", "));
}

int FunctionHintProposalModel::activeArgument(const QString& prefix) const {
  QStack<QChar> expecting_end_char;
  int arg = 0;

  foreach (const QChar& c, prefix) {
    if (!expecting_end_char.isEmpty()) {
      // We're in some nested scope, waiting to find the end character
      if (c == expecting_end_char.top()) {
        expecting_end_char.pop();
      }
      continue;
    }

    if      (c == ',')  arg ++;
    else if (c == '(')  expecting_end_char.push(')');
    else if (c == '[')  expecting_end_char.push(']');
    else if (c == '{')  expecting_end_char.push('}');
    else if (c == '\'') expecting_end_char.push('\'');
    else if (c == '"')  expecting_end_char.push('"');
    else if (c == ')') {
      return -1;
    }
  }

  current_arg_ = arg;
  return arg;
}
