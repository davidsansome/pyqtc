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
  MiniParser parser;
  parser.Parse(interface);

  if (parser.parts().isEmpty())
    return NULL;

  // With the line number and logical line indentation, we can guess which
  // local scope we're in.
  const Scope* locals = globals->FindLocalScope(line, parser.logical_line_indent());

  // Resolve all the dotted parts of the token under the cursor.
  QList<const Scope*> possible_scopes;
  possible_scopes << locals << globals;

  for (int i=0 ; i<parser.parts().count() - 1 ; ++i) {
    const QString name = parser.parts()[i].name_;

    QList<const Scope*> new_possible_scopes;

    foreach (const Scope* scope, possible_scopes) {
      const Scope* target_scope = scope->GetChildScope(name);
      if (target_scope) {
        if (target_scope->type() != pb::Scope_Type_FUNCTION) {
          new_possible_scopes << target_scope;
        }
        continue;
      }

      const pb::Variable* target_variable = scope->GetChildVariable(name);
      if (target_variable) {
        foreach (const QString& type_name, target_variable->possible_type_id()) {
          const Scope* type_scope = model_->ScopeByTypeName(type_name, scope->module());
          if (type_scope) {
            new_possible_scopes << type_scope;
          }
        }
      }
    }

    possible_scopes = new_possible_scopes;
  }

  const QString match_text = parser.parts().last().name_;

  QList<TextEditor::BasicProposalItem*> items;

  // Add any children of those scopes to the completion model.
  foreach (const Scope* scope, possible_scopes) {
    foreach (const Scope* child_scope, scope->child_scopes()) {
      if (!child_scope->name().startsWith(match_text, Qt::CaseInsensitive)) {
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
