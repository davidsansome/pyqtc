#ifndef PYTHONFILTER_H
#define PYTHONFILTER_H

#include <plugins/locator/ilocatorfilter.h>

namespace pyqtc {

namespace pb {
  class Scope;
}

class CodeModel;

class PythonFilter : public Locator::ILocatorFilter {
public:
  PythonFilter(CodeModel* model, QObject* parent = 0);

  // Locator::ILocatorFilter
  QString displayName() const { return tr("Python classes and methods"); }
  QString id() const { return "Python classes and methods"; }
  Priority priority() const { return Medium; }

  QList<Locator::FilterEntry> matchesFor(
      QFutureInterface<Locator::FilterEntry>& future,
      const QString& entry);
  void accept(Locator::FilterEntry selection) const;
  void refresh(QFutureInterface<void>& future);

private:
  void WalkScope(Scope* scope, const QString& query,
                 QList<Locator::FilterEntry>* entries);

private:
  CodeModel* model_;
};

} // namespace

#endif // PYTHONFILTER_H
