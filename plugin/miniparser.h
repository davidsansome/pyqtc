#ifndef MINIPARSER_H
#define MINIPARSER_H

#include <QList>
#include <QString>

namespace TextEditor {
  class IAssistInterface;
}

namespace pyqtc {

namespace pb {
  class Type;
}

class MiniParser {
public:
  MiniParser(pb::Type* type);

  void Parse(const TextEditor::IAssistInterface* iface);

  const QString& last_section() const { return last_section_; }
  int logical_line_indent() const { return logical_line_indent_; }

private:
  pb::Type* type_;
  QString last_section_;
  int logical_line_indent_;
};

} // namespace

#endif // MINIPARSER_H
