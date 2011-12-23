#include "codemodel.h"
#include "completionassist.h"
#include "miniparser.h"

#include <coreplugin/ifile.h>
#include <texteditor/codeassist/basicproposalitem.h>
#include <texteditor/codeassist/basicproposalitemlistmodel.h>
#include <texteditor/codeassist/genericproposal.h>
#include <texteditor/codeassist/iassistinterface.h>
#include <texteditor/convenience.h>

#include <QtDebug>

using namespace pyqtc;

CompletionAssistProvider::CompletionAssistProvider(CodeModel* model)
  : model_(model)
{
}

bool CompletionAssistProvider::supportsEditor(const Core::Id& editorId) const {
  return true;
}

int CompletionAssistProvider::activationCharSequenceLength() const {
  return 1;
}

bool CompletionAssistProvider::isActivationCharSequence(const QString& sequence) const {
  return sequence == ".";
}

TextEditor::IAssistProcessor* CompletionAssistProvider::createProcessor() const {
  return new CompletionAssistProcessor(model_);
}

CompletionAssistProcessor::CompletionAssistProcessor(CodeModel* model)
  : model_(model)
{
}

TextEditor::IAssistProposal* CompletionAssistProcessor::perform(
    const TextEditor::IAssistInterface* interface) {
  // Resolve global scope
  const Scope* globals = model_->FileByFilename(interface->file()->fileName());
  if (!globals)
    return NULL;

  // Get the line and column number
  int line = -1;
  int column = -1;
  if (!TextEditor::Convenience::convertPosition(
        interface->document(), interface->position(), &line, &column)) {
    return NULL;
  }

  // Get the token under the cursor
  pb::Type token_type;
  MiniParser parser(&token_type);
  parser.Parse(interface);

  // With the line number and logical line indentation, we can guess which
  // local scope we're in.
  const Scope* locals = globals->FindLocalScope(line, parser.logical_line_indent());
  qDebug() << "Local scope at line" << line << "indent" << parser.logical_line_indent()
           << "is" << locals->full_dotted_name();
  
  if (token_type.reference_size() != 0) {
    const Scope* new_locals = NULL;
    const Scope* new_globals = NULL;
    
    QString type_id = model_->ResolveType(token_type, locals, globals,
                                          &new_locals, &new_globals);
    
    locals = new_locals;
    globals = NULL;
    
    qDebug() << "Expression resolved to type" << type_id;
  }

  const QString match_text = parser.last_section();

  QList<TextEditor::BasicProposalItem*> items;

  // Add any children of those scopes to the completion model.
  foreach (const Scope* scope, model_->LookupScopes(locals, globals, false)) {
    qDebug() << "Adding scope" << scope->full_dotted_name() << "to completions";
    foreach (const Scope* child_scope, scope->child_scopes()) {
      if (!child_scope->name().startsWith(match_text, Qt::CaseInsensitive) ||
          !child_scope->has_declaration_pos()) {
        continue;
      }

      TextEditor::BasicProposalItem* item = new TextEditor::BasicProposalItem;
      item->setIcon(child_scope->icon());
      item->setText(child_scope->name());
      item->setData(child_scope->name().mid(match_text.length()));
      items << item;
    }

    foreach (const pb::Variable* child_variable, scope->child_variables()) {
      if (!child_variable->name().startsWith(match_text, Qt::CaseInsensitive)) {
        continue;
      }

      TextEditor::BasicProposalItem* item = new TextEditor::BasicProposalItem;
      item->setIcon(model_->IconForVariable(child_variable->name()));
      item->setText(child_variable->name());
      item->setData(child_variable->name().mid(match_text.length()));
      items << item;
    }
  }

  return new TextEditor::GenericProposal(
        interface->position(),
        new TextEditor::BasicProposalItemListModel(items));
}
