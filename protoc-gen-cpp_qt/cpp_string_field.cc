// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include "cpp_string_field.h"
#include "cpp_helpers.h"
#include <google/protobuf/io/printer.h>
#include <google/protobuf/descriptor.pb.h>
#include "strutil.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp_qt {

namespace {

void SetStringVariables(const FieldDescriptor* descriptor,
                        map<string, string>* variables) {
  SetCommonFieldVariables(descriptor, variables);
  (*variables)["default"] = DefaultValue(descriptor);
  (*variables)["default_variable"] = descriptor->default_value_string().empty()
      ? "QString()"
      : "_default_" + FieldName(descriptor) + "_";
}

}  // namespace

// ===================================================================

StringFieldGenerator::
StringFieldGenerator(const FieldDescriptor* descriptor)
  : descriptor_(descriptor) {
  SetStringVariables(descriptor, &variables_);
}

StringFieldGenerator::~StringFieldGenerator() {}

void StringFieldGenerator::
GeneratePrivateMembers(io::Printer* printer) const {
  printer->Print(variables_, "QString $name$_;\n");
  if (!descriptor_->default_value_string().empty()) {
    printer->Print(variables_, "static const char* $default_variable$;\n");
  }
}

void StringFieldGenerator::
GenerateAccessorDeclarations(io::Printer* printer) const {
  // If we're using StringFieldGenerator for a field with a ctype, it's
  // because that ctype isn't actually implemented.  In particular, this is
  // true of ctype=CORD and ctype=STRING_PIECE in the open source release.
  // We aren't releasing Cord because it has too many Google-specific
  // dependencies and we aren't releasing StringPiece because it's hardly
  // useful outside of Google and because it would get confusing to have
  // multiple instances of the StringPiece class in different libraries (PCRE
  // already includes it for their C++ bindings, which came from Google).
  //
  // In any case, we make all the accessors private while still actually
  // using a string to represent the field internally.  This way, we can
  // guarantee that if we do ever implement the ctype, it won't break any
  // existing users who might be -- for whatever reason -- already using .proto
  // files that applied the ctype.  The field can still be accessed via the
  // reflection interface since the reflection interface is independent of
  // the string's underlying representation.
  if (descriptor_->options().ctype() != FieldOptions::STRING) {
    printer->Outdent();
    printer->Print(
      " private:\n"
      "  // Hidden due to unknown ctype option.\n");
    printer->Indent();
  }

  printer->Print(variables_,
    "inline const QString& $name$() const$deprecation$;\n"
    "inline void set_$name$(const QString& value)$deprecation$;\n"
    "inline QString* mutable_$name$()$deprecation$;\n"
    "inline QString* release_$name$()$deprecation$;\n");

  if (descriptor_->options().ctype() != FieldOptions::STRING) {
    printer->Outdent();
    printer->Print(" public:\n");
    printer->Indent();
  }
}

void StringFieldGenerator::
GenerateInlineAccessorDefinitions(io::Printer* printer) const {
  printer->Print(variables_,
    "inline const QString& $classname$::$name$() const {\n"
    "  return $name$_;\n"
    "}\n"
    "inline void $classname$::set_$name$(const QString& value) {\n"
    "  set_has_$name$();\n"
    "  $name$_ = value;\n"
    "}\n"
    "inline QString* $classname$::mutable_$name$() {\n"
    "  set_has_$name$();\n"
    "  return &$name$_;\n"
    "}\n"
    "inline QString* $classname$::release_$name$() {\n"
    "  clear_has_$name$();\n"
    "  return &$name$_;\n"
    "}\n");
}

void StringFieldGenerator::
GenerateNonInlineAccessorDefinitions(io::Printer* printer) const {
  if (!descriptor_->default_value_string().empty()) {
    printer->Print(variables_,
      "const char* $classname$::$default_variable$ = $default$;\n");
  }
}

void StringFieldGenerator::
GenerateClearingCode(io::Printer* printer) const {
  if (descriptor_->default_value_string().empty()) {
    printer->Print(variables_,
      "$name$_ = QString();\n");
  } else {
    printer->Print(variables_,
      "$name$_ = $default_variable$;\n");
  }
}

void StringFieldGenerator::
GenerateMergingCode(io::Printer* printer) const {
  printer->Print(variables_, "set_$name$(from.$name$());\n");
}

void StringFieldGenerator::
GenerateSwappingCode(io::Printer* printer) const {
  printer->Print(variables_, "std::swap($name$_, other->$name$_);\n");
}

void StringFieldGenerator::
GenerateConstructorCode(io::Printer* printer) const {
  printer->Print(variables_,
    "$name$_ = $default_variable$;\n");
}

void StringFieldGenerator::
GenerateMergeFromCodedStream(io::Printer* printer) const {
  printer->Print(variables_,
    "std::string temp;\n"
    "DO_(::google::protobuf::internal::WireFormatLite::Read$declared_type$(\n"
    "      input, &temp));\n"
    "*(this->mutable_$name$()) = QString::fromUtf8(temp.data(), temp.size());\n"
  );
}

void StringFieldGenerator::
GenerateSerializeWithCachedSizes(io::Printer* printer) const {
  printer->Print(variables_,
    "QByteArray temp(this->$name$().toUtf8());\n"
    "::google::protobuf::internal::WireFormatLite::Write$declared_type$(\n"
    "  $number$, std::string(temp.constData(), temp.size()), output);\n");
}

void StringFieldGenerator::
GenerateSerializeWithCachedSizesToArray(io::Printer* printer) const {
  printer->Print(variables_,
    "QByteArray temp(this->$name$().toUtf8());\n"
    "target =\n"
    "  ::google::protobuf::internal::WireFormatLite::Write$declared_type$ToArray(\n"
    "    $number$, std::string(temp.constData(), temp.size()), target);\n");
}

void StringFieldGenerator::
GenerateByteSize(io::Printer* printer) const {
  printer->Print(variables_,
    "QByteArray temp(this->$name$().toUtf8());\n"
    "total_size += $tag_size$ +\n"
    "  ::google::protobuf::internal::WireFormatLite::$declared_type$Size(\n"
    "    std::string(temp.constData(), temp.size()));\n");
}

// ===================================================================

RepeatedStringFieldGenerator::
RepeatedStringFieldGenerator(const FieldDescriptor* descriptor)
  : descriptor_(descriptor) {
  SetStringVariables(descriptor, &variables_);
}

RepeatedStringFieldGenerator::~RepeatedStringFieldGenerator() {}

void RepeatedStringFieldGenerator::
GeneratePrivateMembers(io::Printer* printer) const {
  printer->Print(variables_,
    "QStringList $name$_;\n");
}

void RepeatedStringFieldGenerator::
GenerateAccessorDeclarations(io::Printer* printer) const {
  // See comment above about unknown ctypes.
  if (descriptor_->options().ctype() != FieldOptions::STRING) {
    printer->Outdent();
    printer->Print(
      " private:\n"
      "  // Hidden due to unknown ctype option.\n");
    printer->Indent();
  }

  printer->Print(variables_,
    "inline const QString& $name$(int index) const$deprecation$;\n"
    "inline QString* mutable_$name$(int index)$deprecation$;\n"
    "inline void set_$name$(int index, const QString& value)$deprecation$;\n"
    "inline QString* add_$name$()$deprecation$;\n"
    "inline void add_$name$(const QString& value)$deprecation$;\n");

  printer->Print(variables_,
    "inline const QStringList& $name$() const$deprecation$;\n"
    "inline QStringList* mutable_$name$()$deprecation$;\n");

  if (descriptor_->options().ctype() != FieldOptions::STRING) {
    printer->Outdent();
    printer->Print(" public:\n");
    printer->Indent();
  }
}

void RepeatedStringFieldGenerator::
GenerateInlineAccessorDefinitions(io::Printer* printer) const {
  printer->Print(variables_,
    "inline const QString& $classname$::$name$(int index) const {\n"
    "  return $name$_[index];\n"
    "}\n"
    "inline QString* $classname$::mutable_$name$(int index) {\n"
    "  return &$name$_[index];\n"
    "}\n"
    "inline void $classname$::set_$name$(int index, const QString& value) {\n"
    "  $name$_[index] = value;\n"
    "}\n"
    "inline QString* $classname$::add_$name$() {\n"
    "  $name$_.append(QString());"
    "  return &$name$_.last();\n"
    "}\n"
    "inline void $classname$::add_$name$(const QString& value) {\n"
    "  $name$_.append(value);\n"
    "}\n");
  printer->Print(variables_,
    "inline const QStringList&\n"
    "$classname$::$name$() const {\n"
    "  return $name$_;\n"
    "}\n"
    "inline QStringList*\n"
    "$classname$::mutable_$name$() {\n"
    "  return &$name$_;\n"
    "}\n");
}

void RepeatedStringFieldGenerator::
GenerateClearingCode(io::Printer* printer) const {
  printer->Print(variables_, "$name$_.clear();\n");
}

void RepeatedStringFieldGenerator::
GenerateMergingCode(io::Printer* printer) const {
  printer->Print(variables_, "$name$_ = from.$name$_;\n");
}

void RepeatedStringFieldGenerator::
GenerateSwappingCode(io::Printer* printer) const {
  printer->Print(variables_, "std::swap($name$_, other->$name$_);\n");
}

void RepeatedStringFieldGenerator::
GenerateConstructorCode(io::Printer* printer) const {
  // Not needed for repeated fields.
}

void RepeatedStringFieldGenerator::
GenerateMergeFromCodedStream(io::Printer* printer) const {
  printer->Print(variables_,
    "std::string temp;\n"
    "DO_(::google::protobuf::internal::WireFormatLite::Read$declared_type$(\n"
    "      input, &temp));\n"
    "this->$name$_.append(QString::fromUtf8(temp.data(), temp.size()));\n"
  );
}

void RepeatedStringFieldGenerator::
GenerateSerializeWithCachedSizes(io::Printer* printer) const {
  printer->Print(variables_,
    "for (int i = 0; i < this->$name$_size(); i++) {\n");
  printer->Print(variables_,
    "  QByteArray temp(this->$name$(i).toUtf8());\n"
    "  ::google::protobuf::internal::WireFormatLite::Write$declared_type$(\n"
    "    $number$, std::string(temp.constData(), temp.size()), output);\n"
    "}\n");
}

void RepeatedStringFieldGenerator::
GenerateSerializeWithCachedSizesToArray(io::Printer* printer) const {
  printer->Print(variables_,
    "for (int i = 0; i < this->$name$_size(); i++) {\n");
  printer->Print(variables_,
    "  QByteArray temp(this->$name$(i).toUtf8());\n"
    "  target = ::google::protobuf::internal::WireFormatLite::\n"
    "    Write$declared_type$ToArray($number$, std::string(temp.constData(), temp.size()), target);\n"
    "}\n");
}

void RepeatedStringFieldGenerator::
GenerateByteSize(io::Printer* printer) const {
  printer->Print(variables_,
    "total_size += $tag_size$ * this->$name$_size();\n"
    "for (int i = 0; i < this->$name$_size(); i++) {\n"
    "  QByteArray temp(this->$name$(i).toUtf8());\n"
    "  total_size += ::google::protobuf::internal::WireFormatLite::$declared_type$Size(\n"
    "    std::string(temp.constData(), temp.size()));\n"
    "}\n");
}

// ===================================================================

BytesFieldGenerator::
BytesFieldGenerator(const FieldDescriptor* descriptor)
  : descriptor_(descriptor) {
  SetStringVariables(descriptor, &variables_);
}

BytesFieldGenerator::~BytesFieldGenerator() {}

void BytesFieldGenerator::
GeneratePrivateMembers(io::Printer* printer) const {
  printer->Print(variables_, "QByteArray $name$_;\n");
  if (!descriptor_->default_value_string().empty()) {
    printer->Print(variables_, "static const char* $default_variable$;\n");
  }
}

void BytesFieldGenerator::
GenerateAccessorDeclarations(io::Printer* printer) const {
  // If we're using BytesFieldGenerator for a field with a ctype, it's
  // because that ctype isn't actually implemented.  In particular, this is
  // true of ctype=CORD and ctype=STRING_PIECE in the open source release.
  // We aren't releasing Cord because it has too many Google-specific
  // dependencies and we aren't releasing StringPiece because it's hardly
  // useful outside of Google and because it would get confusing to have
  // multiple instances of the StringPiece class in different libraries (PCRE
  // already includes it for their C++ bindings, which came from Google).
  //
  // In any case, we make all the accessors private while still actually
  // using a string to represent the field internally.  This way, we can
  // guarantee that if we do ever implement the ctype, it won't break any
  // existing users who might be -- for whatever reason -- already using .proto
  // files that applied the ctype.  The field can still be accessed via the
  // reflection interface since the reflection interface is independent of
  // the string's underlying representation.
  if (descriptor_->options().ctype() != FieldOptions::STRING) {
    printer->Outdent();
    printer->Print(
      " private:\n"
      "  // Hidden due to unknown ctype option.\n");
    printer->Indent();
  }

  printer->Print(variables_,
    "inline const QByteArray& $name$() const$deprecation$;\n"
    "inline void set_$name$(const QByteArray& value)$deprecation$;\n"
    "inline QByteArray* mutable_$name$()$deprecation$;\n"
    "inline QByteArray* release_$name$()$deprecation$;\n");

  if (descriptor_->options().ctype() != FieldOptions::STRING) {
    printer->Outdent();
    printer->Print(" public:\n");
    printer->Indent();
  }
}

void BytesFieldGenerator::
GenerateInlineAccessorDefinitions(io::Printer* printer) const {
  printer->Print(variables_,
    "inline const QByteArray& $classname$::$name$() const {\n"
    "  return $name$_;\n"
    "}\n"
    "inline void $classname$::set_$name$(const QByteArray& value) {\n"
    "  set_has_$name$();\n"
    "  $name$_ = value;\n"
    "}\n"
    "inline QByteArray* $classname$::mutable_$name$() {\n"
    "  set_has_$name$();\n"
    "  return &$name$_;\n"
    "}\n"
    "inline QByteArray* $classname$::release_$name$() {\n"
    "  clear_has_$name$();\n"
    "  return &$name$_;\n"
    "}\n");
}

void BytesFieldGenerator::
GenerateNonInlineAccessorDefinitions(io::Printer* printer) const {
  if (!descriptor_->default_value_string().empty()) {
    printer->Print(variables_,
      "const char* $classname$::$default_variable$ = $default$;\n");
  }
}

void BytesFieldGenerator::
GenerateClearingCode(io::Printer* printer) const {
  if (descriptor_->default_value_string().empty()) {
    printer->Print(variables_,
      "$name$_ = QByteArray();\n");
  } else {
    printer->Print(variables_,
      "$name$_ = $default_variable$;\n");
  }
}

void BytesFieldGenerator::
GenerateMergingCode(io::Printer* printer) const {
  printer->Print(variables_, "set_$name$(from.$name$());\n");
}

void BytesFieldGenerator::
GenerateSwappingCode(io::Printer* printer) const {
  printer->Print(variables_, "std::swap($name$_, other->$name$_);\n");
}

void BytesFieldGenerator::
GenerateConstructorCode(io::Printer* printer) const {
  printer->Print(variables_,
    "$name$_ = $default_variable$;\n");
}

void BytesFieldGenerator::
GenerateMergeFromCodedStream(io::Printer* printer) const {
  printer->Print(variables_,
    "std::string temp;\n"
    "DO_(::google::protobuf::internal::WireFormatLite::Read$declared_type$(\n"
    "      input, &temp));\n"
    "*(this->mutable_$name$()) = QByteArray(temp.data(), temp.size());\n"
  );
}

void BytesFieldGenerator::
GenerateSerializeWithCachedSizes(io::Printer* printer) const {
  printer->Print(variables_,
    "::google::protobuf::internal::WireFormatLite::Write$declared_type$(\n"
    "  $number$, std::string(this->$name$().constData(), this->$name$().size()), output);\n");
}

void BytesFieldGenerator::
GenerateSerializeWithCachedSizesToArray(io::Printer* printer) const {
  printer->Print(variables_,
    "target =\n"
    "  ::google::protobuf::internal::WireFormatLite::Write$declared_type$ToArray(\n"
    "    $number$, std::string(this->$name$().constData(), this->$name$().size()), target);\n");
}

void BytesFieldGenerator::
GenerateByteSize(io::Printer* printer) const {
  printer->Print(variables_,
    "total_size += $tag_size$ +\n"
    "  ::google::protobuf::internal::WireFormatLite::$declared_type$Size(\n"
    "    std::string(this->$name$().constData(), this->$name$().size()));\n");
}

// ===================================================================

RepeatedBytesFieldGenerator::
RepeatedBytesFieldGenerator(const FieldDescriptor* descriptor)
  : descriptor_(descriptor) {
  SetStringVariables(descriptor, &variables_);
}

RepeatedBytesFieldGenerator::~RepeatedBytesFieldGenerator() {}

void RepeatedBytesFieldGenerator::
GeneratePrivateMembers(io::Printer* printer) const {
  printer->Print(variables_,
    "QList<QByteArray> $name$_;\n");
}

void RepeatedBytesFieldGenerator::
GenerateAccessorDeclarations(io::Printer* printer) const {
  // See comment above about unknown ctypes.
  if (descriptor_->options().ctype() != FieldOptions::STRING) {
    printer->Outdent();
    printer->Print(
      " private:\n"
      "  // Hidden due to unknown ctype option.\n");
    printer->Indent();
  }

  printer->Print(variables_,
    "inline const QByteArray& $name$(int index) const$deprecation$;\n"
    "inline QByteArray* mutable_$name$(int index)$deprecation$;\n"
    "inline void set_$name$(int index, const QByteArray& value)$deprecation$;\n"
    "inline QByteArray* add_$name$()$deprecation$;\n"
    "inline void add_$name$(const QByteArray& value)$deprecation$;\n");

  printer->Print(variables_,
    "inline const QList<QByteArray>& $name$() const$deprecation$;\n"
    "inline QList<QByteArray>* mutable_$name$()$deprecation$;\n");

  if (descriptor_->options().ctype() != FieldOptions::STRING) {
    printer->Outdent();
    printer->Print(" public:\n");
    printer->Indent();
  }
}

void RepeatedBytesFieldGenerator::
GenerateInlineAccessorDefinitions(io::Printer* printer) const {
  printer->Print(variables_,
    "inline const QByteArray& $classname$::$name$(int index) const {\n"
    "  return $name$_[index];\n"
    "}\n"
    "inline QByteArray* $classname$::mutable_$name$(int index) {\n"
    "  return &$name$_[index];\n"
    "}\n"
    "inline void $classname$::set_$name$(int index, const QByteArray& value) {\n"
    "  $name$_[index] = value;\n"
    "}\n"
    "inline QByteArray* $classname$::add_$name$() {\n"
    "  $name$_.append(QByteArray());"
    "  return &$name$_.last();\n"
    "}\n"
    "inline void $classname$::add_$name$(const QByteArray& value) {\n"
    "  $name$_.append(value);\n"
    "}\n");
  printer->Print(variables_,
    "inline const QList<QByteArray>&\n"
    "$classname$::$name$() const {\n"
    "  return $name$_;\n"
    "}\n"
    "inline QList<QByteArray>*\n"
    "$classname$::mutable_$name$() {\n"
    "  return &$name$_;\n"
    "}\n");
}

void RepeatedBytesFieldGenerator::
GenerateClearingCode(io::Printer* printer) const {
  printer->Print(variables_, "$name$_.clear();\n");
}

void RepeatedBytesFieldGenerator::
GenerateMergingCode(io::Printer* printer) const {
  printer->Print(variables_, "$name$_ = from.$name$_;\n");
}

void RepeatedBytesFieldGenerator::
GenerateSwappingCode(io::Printer* printer) const {
  printer->Print(variables_, "std::swap($name$_, other->$name$_);\n");
}

void RepeatedBytesFieldGenerator::
GenerateConstructorCode(io::Printer* printer) const {
  // Not needed for repeated fields.
}

void RepeatedBytesFieldGenerator::
GenerateMergeFromCodedStream(io::Printer* printer) const {
  printer->Print(variables_,
    "DO_(::google::protobuf::internal::WireFormatLite::Read$declared_type$(\n"
    "      input, &temp));\n"
    "this->$name$_.append(QByteArray(temp.data(), temp.size()));\n"
  );
}

void RepeatedBytesFieldGenerator::
GenerateSerializeWithCachedSizes(io::Printer* printer) const {
  printer->Print(variables_,
    "for (int i = 0; i < this->$name$_size(); i++) {\n");
  printer->Print(variables_,
    "  ::google::protobuf::internal::WireFormatLite::Write$declared_type$(\n"
    "    $number$, std::string(this->$name$(i).constData(), this->$name$(i).size()), output);\n"
    "}\n");
}

void RepeatedBytesFieldGenerator::
GenerateSerializeWithCachedSizesToArray(io::Printer* printer) const {
  printer->Print(variables_,
    "for (int i = 0; i < this->$name$_size(); i++) {\n");
  printer->Print(variables_,
    "  target = ::google::protobuf::internal::WireFormatLite::\n"
    "    Write$declared_type$ToArray($number$, std::string(this->$name$(i).constData(), this->$name$(i).size()), target);\n"
    "}\n");
}

void RepeatedBytesFieldGenerator::
GenerateByteSize(io::Printer* printer) const {
  printer->Print(variables_,
    "total_size += $tag_size$ * this->$name$_size();\n"
    "for (int i = 0; i < this->$name$_size(); i++) {\n"
    "  total_size += ::google::protobuf::internal::WireFormatLite::$declared_type$Size(\n"
    "    std::string(this->$name$(i).constData(), this->$name$(i).size()));\n"
    "}\n");
}

}  // namespace cpp_qt
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
