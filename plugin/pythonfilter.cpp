/* This file is part of Clementine.
   Copyright 2011, David Sansome <me@davidsansome.com>
   
   Clementine is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   Clementine is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with Clementine.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "pythonfilter.h"
#include "pythonicons.h"

#include <texteditor/basetexteditor.h>

using namespace pyqtc;

PythonFilter::PythonFilter(WorkerPool<WorkerClient>* worker_pool,
                           const PythonIcons* icons)
  : Locator::ILocatorFilter(NULL),
    worker_pool_(worker_pool),
    icons_(icons)
{
  setShortcutString("py");
}

QList<Locator::FilterEntry> PythonFilter::matchesFor(
    QFutureInterface<Locator::FilterEntry>& future, const QString& entry) {
  QScopedPointer<WorkerClient::ReplyType> reply(
        worker_pool_->NextHandler()->Search(entry));
  reply->WaitForFinished();

  QList<Locator::FilterEntry> ret;
  if (!reply->is_successful() || future.isCanceled()) {
    return ret;
  }

  const pb::SearchResponse* response = &reply->message().search_response();

  for (int i=0 ; i<response->result_size() ; ++i) {
    const pb::SearchResponse_Result* result = &response->result(i);

    EntryInternalData internal_data(result->file_path(), result->line_number());

    Locator::FilterEntry entry(this, result->symbol_name(),
                               QVariant::fromValue(internal_data));
    entry.extraInfo = result->module_name();
    entry.displayIcon = icons_->IconForSearchResult(*result);

    ret << entry;
  }

  return ret;
}

void PythonFilter::accept(Locator::FilterEntry selection) const {
  const EntryInternalData& data =
      selection.internalData.value<pyqtc::PythonFilter::EntryInternalData>();

  TextEditor::BaseTextEditorWidget::openEditorAt(
        data.file_path_, data.line_number_, 0,
        Core::Id(), Core::EditorManager::ModeSwitch);
}

void PythonFilter::refresh(QFutureInterface<void>& future) {
}
