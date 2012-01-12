"""
Reads uint32 length-encoded protobufs from stdin, handles requests and writes
responses to stdout.
"""

import logging
import re
import socket
import struct
import sys

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


class MessageHandler(object):
  """
  Abstract subclass for handling messages and sending responses.  Your subclass
  should implement a method for each request you want to handle.  The fields in
  the request protobuf are searched for a field ending with "_request".  That
  field name is converted to CamelCase and the method with that name is called
  on this class.
  """

  handlers = None

  UNDER_LETTER = re.compile(r'_([a-z])')

  def __init__(self, message_class):
    self.message_class = message_class

  def ReadMessage(self, handle):
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

    return self.message_class.FromString(data)

  @staticmethod
  def WriteMessage(handle, message):
    """
    uint32 length-encodes the given protobuf and writes it to the file handle.
    """

    data = message.SerializeToString()
    handle.write(struct.pack(">I", len(data)) + data)
    handle.flush()

  def FunctionForRequest(self, request):
    """
    Returns a callable to handle this request.  Raises UnknownRequestType if
    no handler was found, or no x_request field was found in the request
    protobuf.
    """

    for descriptor, _value in request.ListFields():
      name = descriptor.name
      if not name.endswith("_request"):
        continue
      
      # Convert some_thing_request to SomeThingRequest
      name = name[0].upper() + name[1:]
      name = self.UNDER_LETTER.sub(lambda m: m.group(1).upper(), name)
      
      try:
        return getattr(self, name)
      except AttributeError:
        raise UnknownRequestType
    
    raise UnknownRequestType

  def ServeForever(self, socket_filename):
    """
    Connects to the given local socket and listens for incoming request
    protobufs.  Handles the requests and writes the responses back to the 
    socket.
    """

    sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    sock.connect(socket_filename)

    input_handle = output_handle = sock.makefile()

    while True:
      try:
        request = self.ReadMessage(input_handle)
      except ShortReadError:
        break

      print >> sys.stderr, ">" * 80
      print >> sys.stderr, request

      # Create a response and fill its ID
      response = self.message_class()
      response.id = request.id

      try:
        # Find a function to handle the request and call it
        self.FunctionForRequest(request)(request, response)
      except Exception, ex:
        logging.exception("Error handling request %s", request)
        response.error_response.message = \
          "%s: %s" % (ex.__class__.__name__, str(ex))

      print >> sys.stderr, "<" * 80
      print >> sys.stderr, response

      self.WriteMessage(output_handle, response)
