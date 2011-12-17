#include "codemodel.h"
#include "pythonfilter.h"

using namespace Locator;

using namespace pyqtc;


PythonFilter::PythonFilter(CodeModel* model, QObject* parent)
  : ILocatorFilter(parent),
    model_(model)
{
  setIncludedByDefault(true);
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
    FilterEntry entry(this, scope->name(), QVariant());
    entry.extraInfo = scope->ParentDottedName();
    entry.displayIcon = scope->icon();

    entries->append(entry);
  }

  foreach (Scope* child_scope, scope->children()) {
    WalkScope(child_scope, query, entries);
  }
}

void PythonFilter::accept(Locator::FilterEntry selection) const {
}

void PythonFilter::refresh(QFutureInterface<void>& future) {
}
