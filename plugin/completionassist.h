#ifndef PYQTC_COMPLETIONASSIST_H
#define PYQTC_COMPLETIONASSIST_H

#include <texteditor/codeassist/completionassistprovider.h>
#include <texteditor/codeassist/iassistprocessor.h>
#include <texteditor/codeassist/igenericproposalmodel.h>

#include "workerclient.h"
#include "workerpool.h"

namespace TextEditor {
  class IAssistInterface;
}

namespace pyqtc {

class CompletionAssistProvider : public TextEditor::CompletionAssistProvider {
public:
  CompletionAssistProvider(WorkerPool<WorkerClient>* worker_pool);

  bool supportsEditor(const QString& editorId) const;
  int activationCharSequenceLength() const;
  bool isActivationCharSequence(const QString& sequence) const;
  TextEditor::IAssistProcessor* createProcessor() const;

private:
  WorkerPool<WorkerClient>* worker_pool_;
};


class CompletionAssistProcessor : public TextEditor::IAssistProcessor {
public:
  CompletionAssistProcessor(WorkerPool<WorkerClient>* worker_pool);

  TextEditor::IAssistProposal* perform(const TextEditor::IAssistInterface* interface);

private:
  WorkerPool<WorkerClient>* worker_pool_;
};

} // namespace pyqtc

#endif // PYQTC_COMPLETIONASSIST_H
