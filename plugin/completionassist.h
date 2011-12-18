#ifndef PYQTC_COMPLETIONASSIST_H
#define PYQTC_COMPLETIONASSIST_H

#include <texteditor/codeassist/completionassistprovider.h>
#include <texteditor/codeassist/iassistprocessor.h>
#include <texteditor/codeassist/igenericproposalmodel.h>

namespace TextEditor {
  class IAssistInterface;
}


namespace pyqtc {

class CodeModel;

class CompletionAssistProvider : public TextEditor::CompletionAssistProvider {
public:
  CompletionAssistProvider(CodeModel* model);

  bool supportsEditor(const Core::Id& editorId) const;
  int activationCharSequenceLength() const;
  bool isActivationCharSequence(const QString& sequence) const;
  TextEditor::IAssistProcessor* createProcessor() const;

private:
  CodeModel* model_;
};


class CompletionAssistProcessor : public TextEditor::IAssistProcessor {
public:
  CompletionAssistProcessor(CodeModel* model);

  TextEditor::IAssistProposal* perform(const TextEditor::IAssistInterface* interface);

private:
  CodeModel* model_;
};

} // namespace pyqtc

#endif // PYQTC_COMPLETIONASSIST_H
