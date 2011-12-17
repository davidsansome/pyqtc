#include "codemodel.h"
#include "pythonfilter.h"

#include <texteditor/itexteditor.h>
#include <texteditor/basetexteditor.h>

using namespace Locator;

using namespace pyqtc;

Q_DECLARE_METATYPE(pyqtc::pb::Position)


PythonFilter::PythonFilter(CodeModel* model, QObject* parent)
  : ILocatorFilter(parent),
    model_(model)
{
  setIncludedByDefault(true);

  qRegisterMetaType<pyqtc::pb::Position>("pyqtc::pb::Position");
}

QList<FilterEntry> PythonFilter::matchesFor(QFutureInterface<FilterEntry>& future,
                                            const QString& entry) {
  QList<FilterEntry> ret;

  for (CodeModel::FilesMap::const_iterator it = model_->files().begin() ;
       it != model_->files().end() ; ++it) {
    WalkScope(it.value(), entry, &ret);
  }

  return ret;
}

void PythonFilter::WalkScope(Scope* scope, const QString& query,
                             QList<Locator::FilterEntry>* entries) {
  if (scope->type() != pb::Scope_Type_MODULE &&
      scope->name().contains(query, Qt::CaseInsensitive)) {
    FilterEntry entry(this, scope->name(),
                      QVariant::fromValue(scope->declaration_pos()));
    entry.extraInfo = scope->ParentDottedName();
    entry.displayIcon = scope->icon();

    entries->append(entry);
  }

  foreach (Scope* child_scope, scope->children()) {
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
