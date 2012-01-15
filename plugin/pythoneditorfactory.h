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

#ifndef PYQTC_PYTHONEDITORFACTORY_H
#define PYQTC_PYTHONEDITORFACTORY_H

#include <coreplugin/editormanager/ieditorfactory.h>

#include <QStringList>


namespace TextEditor {
  class TextEditorActionHandler;
}


namespace pyqtc {

class PythonEditorFactory : public Core::IEditorFactory {
public:
  PythonEditorFactory(QObject* parent = NULL);
  ~PythonEditorFactory();

  // IEditorFactory
  QStringList mimeTypes() const;
  Core::Id id() const;
  QString displayName() const;
  Core::IFile* open(const QString& file_name);
  Core::IEditor* createEditor(QWidget* parent);

private:
  QStringList mime_types_;
  TextEditor::TextEditorActionHandler* action_handler_;
};

} // namespace pyqtc

#endif // PYQTC_PYTHONEDITORFACTORY_H
