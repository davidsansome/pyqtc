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

#ifndef PYQTC_PYTHONEDITOR_H
#define PYQTC_PYTHONEDITOR_H

#include <texteditor/plaintexteditor.h>

#include "config.h"

namespace pyqtc {

class PythonEditorWidget;

class PythonEditor : public TextEditor::PlainTextEditor {
  Q_OBJECT

public:
  PythonEditor(PythonEditorWidget* editor);

  bool duplicateSupported() const { return false; }
  Core::IEditor* duplicate(QWidget* parent) { return NULL; }
  bool isTemporary() const { return false; }

#ifdef QTC_HAS_CORE_ID
  Core::Id id() const;
#else
  QString id() const;
#endif
};


class PythonEditorWidget : public TextEditor::PlainTextEditorWidget {
  Q_OBJECT

public:
  PythonEditorWidget(QWidget* parent);

  void unCommentSelection();

protected:
  void contextMenuEvent(QContextMenuEvent* e);
  TextEditor::BaseTextEditor* createEditor() { return new PythonEditor(this); }

private slots:
  void Configure();

private:
  Utils::CommentDefinition comment_definition_;
};

} // namespace pyqtc

#endif // PYQTC_PYTHONEDITOR_H
