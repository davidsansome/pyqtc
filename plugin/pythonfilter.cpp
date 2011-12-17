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
    WalkScope(it.value().scope_, entry, &ret);
  }

  return ret;
}

void PythonFilter::WalkScope(const Scope& scope, const QString& query,
                             QList<Locator::FilterEntry>* entries) {
  if (scope.name().contains(query, Qt::CaseInsensitive)) {
    FilterEntry entry(this, scope.name(), QVariant());
    entries->append(entry);
  }

  for (int i=0 ; i<scope.child_scope_size() ; ++i) {
    WalkScope(scope.child_scope(i), query, entries);
  }
}

void PythonFilter::accept(Locator::FilterEntry selection) const {
}

void PythonFilter::refresh(QFutureInterface<void>& future) {
}
