"""
Reads uint32 length-encoded protobufs from stdin, handles requests and writes
responses to stdout.
"""

import ast
import logging
import socket
import struct
import sys

import parse
import rpc_pb2


def ParseFile(request, response):
  """
  Handler for rpc_pb2.ParseFileRequest.
  """

  with open(request.filename) as handle:
    source = handle.read()

  try:
    root = ast.parse(source, request.filename)
  except SyntaxError, ex:
    response.syntax_error.position.filename = ex.filename
    response.syntax_error.position.line     = ex.lineno
    response.syntax_error.position.column   = ex.offset
    response.syntax_error.text              = ex.text
  else:
    ctx = parse.ParseContext(request.filename)
    scope = parse.Scope(ctx, root, pb=response.module)
    scope.Populate(ctx)


class ShortReadError(Exception):
  """
  An EOF was read from the input handle.
  """

  pass


class UnknownRequestType(Exception):
  """
  The request protobuf did not contain any recognised request types.
  """
  
  pass


def ReadMessage(handle):
  """
  Reads a uint32 length-encoded protobuf from the file handle and returns it.
  """
  
  # Read the length
  encoded_length = handle.read(4)
  if len(encoded_length) != 4:
    raise ShortReadError()

  # Decode the length
  (length,) = struct.unpack(">I", encoded_length)

  # Read the protobuf
  data = handle.read(length)
  if len(data) != length:
    raise ShortReadError()

  return rpc_pb2.Message.FromString(data)


def WriteMessage(handle, message):
  """
  uint32 length-encodes the given protobuf and writes it to the file handle.
  """

  data = message.SerializeToString()
  handle.write(struct.pack(">I", len(data)) + data)
  handle.flush()
      

def Main(socket_filename):
  """
  Connects to the given local socket and listens for incoming request protobufs.
  Handles the requests and writes the responses back to the socket.
  """

  sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
  sock.connect(socket_filename)

  input_handle = output_handle = sock.makefile()

  logging.basicConfig()

  while True:
    try:
      request = ReadMessage(input_handle)
    except ShortReadError:
      break

    response = rpc_pb2.Message()
    response.id = request.id

    try:
      if request.HasField("parse_file_request"):
        ParseFile(request.parse_file_request, response.parse_file_response)
      else:
        raise UnknownRequestType()
    except Exception, ex:
      logging.exception("Error handling request %s", request)
      response.error_response.message = \
        "%s: %s" % (ex.__class__.__name__, str(ex))

    WriteMessage(output_handle, response)


if __name__ == "__main__":
  Main(sys.argv[1])
