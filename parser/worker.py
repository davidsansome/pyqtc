#!/usr/bin/env python

"""
Entry point for the pyqtc worker.
"""

import logging
import os
import rope.base.project
from rope.base import worder
from rope.contrib import codeassist
import sys

import messagehandler
import rpc_pb2


class ProjectNotFoundError(Exception):
  """
  An action was requested on a file that was not found in a project.
  """


class Handler(messagehandler.MessageHandler):
  """
  Handles rpc requests.
  """

  PROPOSAL_TYPES = {
    "instance": rpc_pb2.CompletionResponse.Proposal.INSTANCE,
    "class":    rpc_pb2.CompletionResponse.Proposal.CLASS,
    "function": rpc_pb2.CompletionResponse.Proposal.FUNCTION,
    "module":   rpc_pb2.CompletionResponse.Proposal.MODULE,
  }

  PROPOSAL_SCOPES = {
    "local":             rpc_pb2.CompletionResponse.Proposal.LOCAL,
    "global":            rpc_pb2.CompletionResponse.Proposal.GLOBAL,
    "builtin":           rpc_pb2.CompletionResponse.Proposal.BUILTIN,
    "attribute":         rpc_pb2.CompletionResponse.Proposal.ATTRIBUTE,
    "imported":          rpc_pb2.CompletionResponse.Proposal.IMPORTED,
    "keyword":           rpc_pb2.CompletionResponse.Proposal.KEYWORD,
    "parameter_keyword": rpc_pb2.CompletionResponse.Proposal.PARAMETER_KEYWORD,
  }

  MAXFIXES = 10

  def __init__(self):
    super(Handler, self).__init__(rpc_pb2.Message)

    self.projects = {}

  def CreateProjectRequest(self, request, _response):
    """
    Creates a new rope project and stores it away for later.
    """

    root = os.path.normpath(request.create_project_request.project_root)
    project = rope.base.project.Project(root)

    self.projects[root] = project
  
  def DestroyProjectRequest(self, request, _response):
    """
    Cleans up a rope project when it is closed by the user in Qt Creator.
    """

    root = os.path.normpath(request.create_project_request.project_root)
    project = self.projects[root]

    project.close()
    del self.projects[root]

  def _Context(self, context):
    """
    Returns a (project, source, offset) tuple for the context.
    """

    return (
      self._ProjectForFile(context.file_path),
      context.source_text,
      context.cursor_position,
    )
  
  def _ProjectForFile(self, file_path):
    """
    Tries to find the project that contains the given file.
    """

    while file_path:
      try:
        return self.projects[file_path]
      except KeyError:
        pass

      file_path = os.path.dirname(file_path)
    
    raise ProjectNotFoundError
  
  def CompletionRequest(self, request, response):
    """
    Finds completion proposals for the given location in the given source file.
    """

    # Ignore access to protected members pylint: disable=W0212

    # Guess at the root directory for this project by walking up the path
    project, source, offset = self._Context(request.completion_request.context)
    starting_offset = codeassist.starting_offset(source, offset)

    response.completion_response.insertion_position = starting_offset

    # Is the cursor immediately after a comma or open paren?
    word_finder = worder.Worder(source)
    non_space_offset = word_finder.code_finder._find_last_non_space_char(offset)
    if word_finder.code_finder.code[non_space_offset] in "(,":
      paren_start = word_finder.find_parens_start_from_inside(offset)

      # Get a calltip now
      calltip = codeassist.get_calltip(project, source, paren_start-1,
                                       maxfixes=self.MAXFIXES,
                                       remove_self=True)
      
      if calltip is not None:
        response.completion_response.calltip = calltip
        return
    
    # Do normal completion if a calltip couldn't be found
    proposals = codeassist.code_assist(project, source, offset,
                                       maxfixes=self.MAXFIXES)
    proposals = codeassist.sorted_proposals(proposals)
    
    # Construct the response protobuf
    for proposal in proposals:
      proposal_pb = response.completion_response.proposal.add()
      proposal_pb.name = proposal.name

      docstring = proposal.get_doc()

      if proposal.type in self.PROPOSAL_TYPES:
        proposal_pb.type = self.PROPOSAL_TYPES[proposal.type]

      if proposal.scope in self.PROPOSAL_SCOPES:
        proposal_pb.scope = self.PROPOSAL_SCOPES[proposal.scope]

      if docstring is not None:
        proposal_pb.docstring = docstring

  def TooltipRequest(self, request, response):
    """
    Finds and returns a tooltip for the given location in the given source file.
    """

    project, source, offset = self._Context(request.tooltip_request.context)
    docstring = codeassist.get_doc(project, source, offset, maxfixes=10)

    if docstring is not None:
      response.tooltip_response.rich_text = docstring


def Main(args):
  """
  Connects to the socket passed on the commandline and listens for requests.
  """

  logging.basicConfig()

  handler = Handler()
  handler.ServeForever(args[0])


if __name__ == "__main__":
  Main(sys.argv[1:])
