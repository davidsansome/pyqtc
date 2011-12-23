#include "codemodel.pb.h"
#include "miniparser.h"

#include <texteditor/codeassist/iassistinterface.h>

#include <QStringList>
#include <QtDebug>


using namespace pyqtc;

MiniParser::MiniParser(pb::Type* type)
  : type_(type),
    logical_line_indent_(0)
{
}

void MiniParser::Parse(const TextEditor::IAssistInterface* iface) {
  // Parse() does two things - gets the name of the identifier under the cursor,
  // and gets the indentation level of this logical line.  Once we've reached
  // the beginning of the identifier we still want to go back to the start
  // of the line.
  bool taking_identifier = true;

  // Counts nested parenthesis level relative to the starting position.  When
  // it goes below 0, we stop parsing for the identifier.
  int paren_depth = 0;

  // We were in whitespace before.  If the next thing we encounter is a
  // character from a different identifier, stop parsing.
  int whitespace_amount = 0;

  QList<pb::Type_Reference> refs;
  
  pb::Type_Reference ref;
  ref.set_kind(pb::Type_Reference_Kind_SCOPE_LOOKUP);

  for (int position = iface->position() - 1 ; position >= 0 ; --position) {
    const QChar c = iface->characterAt(position);

    if (c == '(') {
      paren_depth --;
      if (paren_depth < 0) {
        taking_identifier = false;
      }
      continue;
    } else if (c == ')') {
      paren_depth ++;
      ref.set_kind(pb::Type_Reference_Kind_SCOPE_CALL);
      continue;
    }

    if (paren_depth != 0) {
      continue;
    }

    if (c.isLetterOrNumber() || c == '_') {
      if (whitespace_amount != 0) {
        taking_identifier = false;
        whitespace_amount = 0;
      } else if (taking_identifier) {
        ref.mutable_name()->prepend(c);
      }
    } else if (c == ' ' || c == '\t') {
      whitespace_amount ++;
      if (taking_identifier && !ref.name().isEmpty()) {
        refs.prepend(ref);
        ref.Clear();
        ref.set_kind(pb::Type_Reference_Kind_SCOPE_LOOKUP);
      }
    } else if (c == '.') {
      whitespace_amount = 0;
      if (taking_identifier) {
        refs.prepend(ref);
        ref.Clear();
        ref.set_kind(pb::Type_Reference_Kind_SCOPE_LOOKUP);
      }
    } else if (c.category() == QChar::Separator_Line ||
               c.category() == QChar::Separator_Paragraph) {
      // It's hard to detect implicit line joins, so we don't bother for now.
      // Only look for explicit line joins with a trailing \ character.
      if (iface->characterAt(position-1) != '\\') {
        break;
      }
      whitespace_amount = 0;
    } else {
      taking_identifier = false;
    }
  }

  if (taking_identifier && !ref.name().isEmpty()) {
    refs.prepend(ref);
  }

  // The last section is special - it's what's being completed.
  if (!refs.isEmpty()) {
    last_section_ = refs.takeLast().name();
  }

  // Add the rest of the references to the pb.
  foreach (const pb::Type_Reference& ref, refs) {
    type_->add_reference()->CopyFrom(ref);
  }

  logical_line_indent_ = whitespace_amount;
}
