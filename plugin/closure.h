#ifndef CLOSURE_H
#define CLOSURE_H

#include <tr1/functional>

#include <QMetaMethod>
#include <QObject>

#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>

namespace pyqtc {

class ClosureArgumentWrapper {
 public:
  virtual ~ClosureArgumentWrapper() {}

  virtual QGenericArgument arg() const = 0;
};

template<typename T>
class ClosureArgument : public ClosureArgumentWrapper {
 public:
  ClosureArgument(const T& data) : data_(data) {}

  virtual QGenericArgument arg() const {
    return Q_ARG(T, data_);
  }

 private:
  T data_;
};

class Closure : public QObject, boost::noncopyable {
  Q_OBJECT

 public:
  Closure(QObject* sender, const char* signal,
          QObject* receiver, const char* slot,
          const ClosureArgumentWrapper* val0 = 0,
          const ClosureArgumentWrapper* val1 = 0,
          const ClosureArgumentWrapper* val2 = 0,
          const ClosureArgumentWrapper* val3 = 0,
          const ClosureArgumentWrapper* val4 = 0,
          const ClosureArgumentWrapper* val5 = 0,
          const ClosureArgumentWrapper* val6 = 0,
          const ClosureArgumentWrapper* val7 = 0,
          const ClosureArgumentWrapper* val8 = 0,
          const ClosureArgumentWrapper* val9 = 0);

  Closure(QObject* sender, const char* signal,
          std::tr1::function<void()> callback);

  bool IsSingleShot() const { return single_shot_; }
  void SetSingleShot(bool single_shot) { single_shot_ = single_shot; }

 private slots:
  void Invoked();
  void Cleanup();

 private:
  void Connect(QObject* sender, const char* signal);

  QMetaMethod slot_;
  std::tr1::function<void()> callback_;
  bool single_shot_;

  boost::scoped_ptr<const ClosureArgumentWrapper> val0_;
  boost::scoped_ptr<const ClosureArgumentWrapper> val1_;
  boost::scoped_ptr<const ClosureArgumentWrapper> val2_;
  boost::scoped_ptr<const ClosureArgumentWrapper> val3_;
  boost::scoped_ptr<const ClosureArgumentWrapper> val4_;
  boost::scoped_ptr<const ClosureArgumentWrapper> val5_;
  boost::scoped_ptr<const ClosureArgumentWrapper> val6_;
  boost::scoped_ptr<const ClosureArgumentWrapper> val7_;
  boost::scoped_ptr<const ClosureArgumentWrapper> val8_;
  boost::scoped_ptr<const ClosureArgumentWrapper> val9_;
};


#include "closurehelpers.h"


} // namespace

#endif  // CLOSURE_H
