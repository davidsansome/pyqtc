"""
Fun script for testing the parser from the commandline.
"""

import ast
import logging
import operator
import sys

import codemodel_pb2
import parse
import typedes


def Main():
  """
  Parses the python source file given on the commandline and prints the
  protobuf.
  """

  logging.basicConfig(level=logging.DEBUG)

  filename = sys.argv[1]
  source = open(filename).read()

  root = ast.parse(source)

  ctx = parse.ParseContext(filename, "__main__")
  root_scope = parse.Scope(ctx, root)
  root_scope.Populate(ctx)

  print root_scope.pb

  print "All types:"
  for name, scope in ctx.types.items():
    print "  %s" % name

    for name, member in sorted(scope().members.items(), key=operator.itemgetter(0)):
      if isinstance(member, codemodel_pb2.Variable):
        variable_type = typedes.TypeDebugString(member.type)
        if variable_type:
          variable_type = " (%s)" % variable_type

        print "    %s%s" % (member.name, variable_type)


if __name__ == "__main__":
  Main()
