#include "closure.h"

#include <QtDebug>

using namespace pyqtc;


Closure::Closure(QObject* sender,
                 const char* signal,
                 QObject* receiver,
                 const char* slot,
                 const ClosureArgumentWrapper* val0,
                 const ClosureArgumentWrapper* val1,
                 const ClosureArgumentWrapper* val2,
                 const ClosureArgumentWrapper* val3,
                 const ClosureArgumentWrapper* val4,
                 const ClosureArgumentWrapper* val5,
                 const ClosureArgumentWrapper* val6,
                 const ClosureArgumentWrapper* val7,
                 const ClosureArgumentWrapper* val8,
                 const ClosureArgumentWrapper* val9)
    : QObject(receiver),
      callback_(NULL),
      single_shot_(true),
      val0_(val0),
      val1_(val1),
      val2_(val2),
      val3_(val3),
      val4_(val4),
      val5_(val5),
      val6_(val6),
      val7_(val7),
      val8_(val8),
      val9_(val9) {
  const QMetaObject* meta_receiver = receiver->metaObject();

  QByteArray normalised_slot = QMetaObject::normalizedSignature(slot + 1);
  const int index = meta_receiver->indexOfSlot(normalised_slot.constData());

  if (index == -1) {
    qWarning() << "Closure receiver not found" << receiver << normalised_slot;
    return;
  }

  slot_ = meta_receiver->method(index);

  Connect(sender, signal);
}

Closure::Closure(QObject* sender,
                 const char* signal,
                 std::tr1::function<void()> callback)
    : callback_(callback) {
  Connect(sender, signal);
}

void Closure::Connect(QObject* sender, const char* signal) {
  bool success = connect(sender, signal, SLOT(Invoked()));
  Q_ASSERT(success);
  success = connect(sender, SIGNAL(destroyed()), SLOT(Cleanup()));
  Q_ASSERT(success);
  Q_UNUSED(success);
}

void Closure::Invoked() {
  if (callback_) {
    callback_();
  } else {
    slot_.invoke(
        parent(),
        val0_ ? val0_->arg() : QGenericArgument(),
        val1_ ? val1_->arg() : QGenericArgument(),
        val2_ ? val2_->arg() : QGenericArgument(),
        val3_ ? val3_->arg() : QGenericArgument(),
        val4_ ? val4_->arg() : QGenericArgument(),
        val5_ ? val5_->arg() : QGenericArgument(),
        val6_ ? val6_->arg() : QGenericArgument(),
        val7_ ? val7_->arg() : QGenericArgument(),
        val8_ ? val8_->arg() : QGenericArgument(),
        val9_ ? val9_->arg() : QGenericArgument());
  }

  if (IsSingleShot())
    deleteLater();
}

void Closure::Cleanup() {
  disconnect();
  deleteLater();
}

Closure* NewClosure(
    QObject* sender, const char* signal,
    QObject* receiver, const char* slot) {
  return new Closure(sender, signal, receiver, slot);
}
