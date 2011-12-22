"""
Fun script for testing the parser from the commandline.
"""

import ast
import logging
import sys

import parse


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
  scope = parse.Scope(ctx, root)
  scope.Populate(ctx)

  print scope.pb

  print "All types:"
  for name in ctx.types.keys():
    print "  %s" % name


if __name__ == "__main__":
  Main()
