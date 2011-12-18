#ifndef MINIPARSER_H
#define MINIPARSER_H

#include <QList>
#include <QString>

namespace TextEditor {
  class IAssistInterface;
}

namespace pyqtc {

class MiniParser {
public:
  MiniParser();

  void Parse(const TextEditor::IAssistInterface* iface);

  struct Part {
    Part() : type_(Type_Member) {}

    enum Type {
      Type_FunctionCall,
      Type_Member
    };

    QString name_;
    Type type_;
  };
  typedef QList<Part> PartList;

  const PartList& parts() const { return parts_; }
  int logical_line_indent() const { return logical_line_indent_; }

  QString DottedName() const;

private:
  PartList parts_;
  int logical_line_indent_;
};

} // namespace

#endif // MINIPARSER_H
