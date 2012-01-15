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

#include "constants.h"
#include "pythoneditor.h"
#include "pythoneditorfactory.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <texteditor/texteditoractionhandler.h>
#include <texteditor/texteditorsettings.h>

#include <QtDebug>

using namespace pyqtc;


PythonEditorFactory::PythonEditorFactory(QObject* parent)
  : Core::IEditorFactory(parent)
{
  mime_types_ << "text/python"
              << "text/x-python"
              << "application/python"
              << "application/x-python";
  action_handler_ = new TextEditor::TextEditorActionHandler(
      constants::kEditorId,
      TextEditor::TextEditorActionHandler::Format |
      TextEditor::TextEditorActionHandler::UnCommentSelection |
      TextEditor::TextEditorActionHandler::UnCollapseAll);
}

PythonEditorFactory::~PythonEditorFactory() {
  delete action_handler_;
}

Core::Id PythonEditorFactory::id() const {
  return constants::kEditorId;
}

QString PythonEditorFactory::displayName() const {
  return tr(constants::kEditorDisplayName);
}

Core::IFile* PythonEditorFactory::open(const QString& file_name) {
  qDebug() << "Opening" << file_name;

  Core::IEditor* iface = Core::EditorManager::instance()->openEditor(file_name, id());
  if (!iface) {
    qWarning() << "pyqtc::EditorFactory::open: openEditor failed for " << file_name;
    return 0;
  }
  return iface->file();
}

Core::IEditor* PythonEditorFactory::createEditor(QWidget* parent) {
  PythonEditorWidget* widget = new PythonEditorWidget(parent);

  action_handler_->setupActions(widget);
  TextEditor::TextEditorSettings::instance()->initializeEditor(widget);

  return widget->editor();
}

QStringList PythonEditorFactory::mimeTypes() const {
  return mime_types_;
}

