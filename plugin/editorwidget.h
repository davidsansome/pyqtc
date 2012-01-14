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

#ifndef PYQTC_EDITORWIDGET_H
#define PYQTC_EDITORWIDGET_H

#include <texteditor/plaintexteditor.h>


namespace pyqtc {

class EditorWidget;

class Editor : public TextEditor::PlainTextEditor {
  Q_OBJECT

public:
  Editor(EditorWidget* editor);

  bool duplicateSupported() const { return false; }
  Core::IEditor* duplicate(QWidget* parent) { return NULL; }
  bool isTemporary() const { return false; }
  Core::Id id() const;
};


class EditorWidget : public TextEditor::PlainTextEditorWidget {
  Q_OBJECT

public:
  EditorWidget(QWidget* parent);

  void unCommentSelection();

protected:
  void contextMenuEvent(QContextMenuEvent* e);
  TextEditor::BaseTextEditor* createEditor() { return new Editor(this); }

private slots:
  void Configure();

private:
  Utils::CommentDefinition comment_definition_;
};

} // namespace pyqtc

#endif // PYQTC_EDITORWIDGET_H
