#ifndef PYQTC_COMPLETIONASSIST_H
#define PYQTC_COMPLETIONASSIST_H

#include <cplusplus/Icons.h>
#include <texteditor/codeassist/completionassistprovider.h>
#include <texteditor/codeassist/functionhintproposal.h>
#include <texteditor/codeassist/iassistprocessor.h>
#include <texteditor/codeassist/ifunctionhintproposalmodel.h>
#include <texteditor/codeassist/igenericproposalmodel.h>

#include "config.h"
#include "rpc.pb.h"
#include "workerclient.h"
#include "workerpool.h"

namespace TextEditor {
  class IAssistInterface;
}

namespace pyqtc {

class PythonIcons;

class CompletionAssistProvider : public TextEditor::CompletionAssistProvider {
public:
  CompletionAssistProvider(WorkerPool<WorkerClient>* worker_pool,
                           const PythonIcons* icons);

#ifdef QTC_HAS_CORE_ID
  bool supportsEditor(const Core::Id& editorId) const;
#else
  bool supportsEditor(const QString& editorId) const;
#endif

  int activationCharSequenceLength() const;
  bool isActivationCharSequence(const QString& sequence) const;
  TextEditor::IAssistProcessor* createProcessor() const;

private:
  WorkerPool<WorkerClient>* worker_pool_;
  const PythonIcons* icons_;
};


class CompletionAssistProcessor : public TextEditor::IAssistProcessor {
public:
  CompletionAssistProcessor(WorkerPool<WorkerClient>* worker_pool,
                            const PythonIcons* icons);

  TextEditor::IAssistProposal* perform(const TextEditor::IAssistInterface* interface);

private:
  TextEditor::IAssistProposal* CreateCalltipProposal(
      int position, const QString& text);
  TextEditor::IAssistProposal* CreateCompletionProposal(
      const pb::CompletionResponse* response);

private:
  WorkerPool<WorkerClient>* worker_pool_;
  const PythonIcons* icons_;
};


class FunctionHintProposalModel : public TextEditor::IFunctionHintProposalModel {
public:
  FunctionHintProposalModel(const QString& text);

  void reset() {}
  int size() const { return 1; }
  QString text(int index) const;
  int activeArgument(const QString& prefix) const;

private:
  QString text_;
  mutable int current_arg_;
};

} // namespace pyqtc

#endif // PYQTC_COMPLETIONASSIST_H
