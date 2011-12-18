#ifndef PYTHONFILTER_H
#define PYTHONFILTER_H

#include <plugins/locator/ilocatorfilter.h>

namespace Core {
  class IEditor;
}

namespace pyqtc {

namespace pb {
  class Scope;
}

class CodeModel;
class Scope;

class PythonFilter : public Locator::ILocatorFilter {
  Q_OBJECT

public:
  PythonFilter(CodeModel* model, QObject* parent = 0);

  void set_show_functions(bool on) { show_functions_ = on; }
  void set_show_classes(bool on) { show_classes_ = on; }
  void set_show_modules(bool on) { show_modules_ = on; }
  void set_current_file_only(bool on) { current_file_only_ = on; }

  // Locator::ILocatorFilter
  QString displayName() const { return tr("Python classes and methods"); }
  QString id() const { return "Python classes and methods"; }
  Priority priority() const { return Medium; }

  QList<Locator::FilterEntry> matchesFor(
      QFutureInterface<Locator::FilterEntry>& future,
      const QString& entry);
  void accept(Locator::FilterEntry selection) const;
  void refresh(QFutureInterface<void>& future);

private slots:
  void EditorAboutToClose(Core::IEditor* editor);
  void CurrentEditorChanged(Core::IEditor* editor);

private:
  void WalkScope(Scope* scope, const QString& query,
                 QList<Locator::FilterEntry>* entries);

private:
  CodeModel* model_;

  bool show_functions_;
  bool show_classes_;
  bool show_modules_;
  bool current_file_only_;

  QString current_filename_;
};


class PythonCurrentDocumentFilter : public PythonFilter {
public:
  PythonCurrentDocumentFilter(CodeModel* model, QObject* parent = 0);

  QString displayName() const { return tr("Python methods in Current Document"); }
  QString id() const { return "Python methods in Current Document"; }
};


class PythonClassFilter : public PythonFilter {
public:
  PythonClassFilter(CodeModel* model, QObject* parent = 0);

  QString displayName() const { return tr("Python classes"); }
  QString id() const { return "Python classes"; }
};


class PythonFunctionFilter : public PythonFilter {
public:
  PythonFunctionFilter(CodeModel* model, QObject* parent = 0);

  QString displayName() const { return tr("Python methods and functions"); }
  QString id() const { return "Python methods and functions"; }
};

} // namespace

#endif // PYTHONFILTER_H
