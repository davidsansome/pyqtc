/* This file is part of Clementine.
   Copyright 2011, David Sansome <me@davidsansome.com>

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

// Note: this file is licensed under the Apache License instead of GPL because
// it is used by the Spotify blob which links against libspotify and is not GPL
// compatible.


#include "messagehandler.h"
#include "rpc.pb.h"

#include <QBuffer>
#include <QDataStream>
#include <QLocalSocket>
#include <QtDebug>

using namespace pyqtc;


MessageHandler::MessageHandler(QLocalSocket* device, QObject* parent)
  : QObject(parent),
    device_(device),
    reading_protobuf_(false),
    expected_length_(0) {
  buffer_.open(QIODevice::ReadWrite);

  connect(device, SIGNAL(readyRead()), SLOT(DeviceReadyRead()));
}

void MessageHandler::DeviceReadyRead() {
  while (device_->bytesAvailable()) {
    if (!reading_protobuf_) {
      // Read the length of the next message
      QDataStream s(device_);
      s >> expected_length_;

      reading_protobuf_ = true;
    }

    // Read some of the message
    buffer_.write(device_->read(expected_length_ - buffer_.size()));

    // Did we get everything?
    if (buffer_.size() == expected_length_) {
      // Parse the message
      Message message;
      if (!message.ParseFromArray(buffer_.data().constData(), buffer_.size())) {
        qDebug() << "Malformed protobuf message";
        device_->close();
        return;
      }

      emit MessageArrived(message);

      // Clear the buffer
      buffer_.close();
      buffer_.setData(QByteArray());
      buffer_.open(QIODevice::ReadWrite);
      reading_protobuf_ = false;
    }
  }
}

void MessageHandler::SendMessage(const Message& message) {
  std::string data = message.SerializeAsString();
  WriteMessage(QByteArray(data.data(), data.size()));
}

void MessageHandler::SendMessageAsync(const Message& message) {
  std::string data = message.SerializeAsString();
  metaObject()->invokeMethod(this, "WriteMessage", Qt::QueuedConnection,
                             Q_ARG(QByteArray, QByteArray(data.data(), data.size())));
}

void MessageHandler::WriteMessage(const QByteArray& data) {
  QDataStream s(device_);
  s << quint32(data.length());
  s.writeRawData(data.data(), data.length());

  device_->flush();
}
