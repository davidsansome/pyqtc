#include "closure.h"
#include "hoverhandler.h"
#include "rpc.pb.h"

#include <coreplugin/ifile.h>
#include <texteditor/itexteditor.h>
#include <texteditor/tooltip/tipcontents.h>
#include <texteditor/tooltip/tooltip.h>

using namespace pyqtc;

HoverHandler::HoverHandler(WorkerPool<WorkerClient>* worker_pool)
  : worker_pool_(worker_pool),
    current_reply_(NULL),
    current_editor_(NULL)
{
}

bool HoverHandler::acceptEditor(Core::IEditor* editor) {
  return true;
}

void HoverHandler::identifyMatch(TextEditor::ITextEditor* editor, int pos) {
  current_reply_ = worker_pool_->NextHandler()->Tooltip(
        editor->file()->fileName(),
        editor->contents(),
        pos);

  NewClosure(current_reply_, SIGNAL(Finished(bool)),
             this, SLOT(TooltipResponse(WorkerClient::ReplyType*)),
             current_reply_);
}

void HoverHandler::TooltipResponse(WorkerClient::ReplyType* reply) {
  reply->deleteLater();

  if (!reply->is_successful() || reply != current_reply_)
    return;

  const QString& text = reply->message().tooltip_response().rich_text();
  if (current_editor_) {
    if (text.isEmpty())
      TextEditor::ToolTip::instance()->hide();
    else
      TextEditor::ToolTip::instance()->show(
            current_point_,
            TextEditor::TextContent(text),
            current_editor_->widget());
  }

  current_reply_ = NULL;
}

void HoverHandler::operateTooltip(TextEditor::ITextEditor* editor,
                                  const QPoint& point) {
  current_editor_ = editor;
  current_point_ = point;
}
