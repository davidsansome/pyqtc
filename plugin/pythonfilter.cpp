#include "codemodel.h"
#include "pythonfilter.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <texteditor/itexteditor.h>
#include <texteditor/basetexteditor.h>

using namespace Locator;

using namespace pyqtc;

Q_DECLARE_METATYPE(pyqtc::pb::Position)


PythonFilter::PythonFilter(CodeModel* model, QObject* parent)
  : ILocatorFilter(parent),
    model_(model),
    show_functions_(true),
    show_classes_(true),
    show_modules_(false),
    current_file_only_(false)
{
  setShortcutString("py");

  Core::EditorManager* editor_manager = Core::ICore::instance()->editorManager();

  connect(editor_manager, SIGNAL(currentEditorChanged(Core::IEditor*)),
          SLOT(CurrentEditorChanged(Core::IEditor*)));
  connect(editor_manager, SIGNAL(editorAboutToClose(Core::IEditor*)),
          SLOT(EditorAboutToClose(Core::IEditor*)));

  qRegisterMetaType<pyqtc::pb::Position>("pyqtc::pb::Position");
}

QList<FilterEntry> PythonFilter::matchesFor(QFutureInterface<FilterEntry>& future,
                                            const QString& entry) {
  QList<FilterEntry> ret;

  if (current_file_only_) {
    const File* file = model_->FileByFilename(current_filename_);
    if (file) {
      WalkScope(file, entry, &ret);
    }
  } else {
    CodeModel::FilesMap all_files = model_->AllFiles();

    for (CodeModel::FilesMap::const_iterator it = all_files.begin() ;
         it != all_files.end() ; ++it) {

      if (future.isCanceled()) {
        break;
      }

      WalkScope(it.value(), entry, &ret);
    }
  }

  return ret;
}

void PythonFilter::WalkScope(const Scope* scope, const QString& query,
                             QList<Locator::FilterEntry>* entries) {

  const bool matches_kind =
      scope->kind() == pb::Scope_Kind_FUNCTION && show_functions_ ||
      scope->kind() == pb::Scope_Kind_CLASS && show_classes_ ||
      scope->kind() == pb::Scope_Kind_MODULE && show_modules_;

  if (matches_kind && scope->name().contains(query, Qt::CaseInsensitive)) {
    FilterEntry entry(this, scope->name(),
                      QVariant::fromValue(scope->declaration_pos()));
    entry.extraInfo = scope->full_dotted_name();
    entry.displayIcon = scope->icon();

    entries->append(entry);
  }

  foreach (const Scope* child_scope, scope->child_scopes()) {
    WalkScope(child_scope, query, entries);
  }
}

void PythonFilter::accept(Locator::FilterEntry selection) const {
  pyqtc::pb::Position position = selection.internalData.value<pyqtc::pb::Position>();

  TextEditor::BaseTextEditorWidget::openEditorAt(
        position.filename(), position.line(), position.column(),
        Core::Id(), Core::EditorManager::ModeSwitch);
}

void PythonFilter::refresh(QFutureInterface<void>& future) {
}

void PythonFilter::CurrentEditorChanged(Core::IEditor* editor) {
  if (editor) {
    current_filename_ = editor->file()->fileName();
  } else {
    current_filename_.clear();
  }
}

void PythonFilter::EditorAboutToClose(Core::IEditor* editor) {
  if (editor && editor->file()->fileName() == current_filename_) {
    current_filename_.clear();
  }
}


PythonCurrentDocumentFilter::PythonCurrentDocumentFilter(CodeModel* model, QObject* parent)
  : PythonFilter(model, parent) {
  set_current_file_only(true);
  setShortcutString(".");
}


PythonClassFilter::PythonClassFilter(CodeModel* model, QObject* parent)
  : PythonFilter(model, parent) {
  set_show_functions(false);
  setShortcutString("c");
}


PythonFunctionFilter::PythonFunctionFilter(CodeModel* model, QObject* parent)
  : PythonFilter(model, parent) {
  set_show_classes(false);
  setShortcutString("m");
}
