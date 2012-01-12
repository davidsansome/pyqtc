#ifndef HOVERHANDLER_H
#define HOVERHANDLER_H

#include "workerclient.h"
#include "workerpool.h"

#include <texteditor/basehoverhandler.h>

#include <QPoint>

namespace pyqtc {

class HoverHandler : public TextEditor::BaseHoverHandler {
  Q_OBJECT

public:
  HoverHandler(WorkerPool<WorkerClient>* worker_pool);

private slots:
  void TooltipResponse(WorkerClient::ReplyType* reply);

private:
  bool acceptEditor(Core::IEditor* editor);
  void identifyMatch(TextEditor::ITextEditor* editor, int pos);
  void operateTooltip(TextEditor::ITextEditor* editor, const QPoint& point);

private:
  WorkerPool<WorkerClient>* worker_pool_;

  WorkerClient::ReplyType* current_reply_;
  TextEditor::ITextEditor* current_editor_;
  QPoint current_point_;
};

} // namespace

#endif // HOVERHANDLER_H
