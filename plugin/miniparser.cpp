#include "miniparser.h"

#include <texteditor/codeassist/iassistinterface.h>

#include <QStringList>
#include <QtDebug>


using namespace pyqtc;

MiniParser::MiniParser()
  : logical_line_indent_(0)
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

  Part part;

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
      part.type_ = Part::Type_FunctionCall;
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
        part.name_.prepend(c);
      }
    } else if (c == ' ' || c == '\t') {
      whitespace_amount ++;
      if (taking_identifier && !part.name_.isEmpty()) {
        parts_.prepend(part);
        part = Part();
      }
    } else if (c == '.') {
      whitespace_amount = 0;
      if (taking_identifier) {
        parts_.prepend(part);
        part = Part();
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

  if (taking_identifier && !part.name_.isEmpty()) {
    parts_.prepend(part);
  }

  logical_line_indent_ = whitespace_amount;
}

QString MiniParser::DottedName() const {
  QStringList ret;

  foreach (const Part& part, parts_) {
    QString name = part.name_;
    if (part.type_ == Part::Type_FunctionCall) {
      name += "()";
    }
    ret << name;
  }

  return ret.join(".");
}
