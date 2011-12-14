import ast
import operator
import sys
import weakref

import codemodel_pb2


def WeakrefOrNone(obj):
  """
  Utility function that returns a weakref to obj if possible, or None.
  """

  if obj is None:
    return None
  return weakref.ref(obj)


class ParseContext(object):
  """
  Information about what file or section of a file is currently being parsed.
  """

  def __init__(self, filename, line_offset=0):
    self.filename = filename
    self.line_offset = line_offset

  def SetPos(self, node, pos_out):
    pos_out.filename = self.filename
    pos_out.line     = getattr(node, "lineno", 0) + self.line_offset
    pos_out.column   = getattr(node, "col_offset", 0)


class Scope(object):
  """
  Base class for things that create a new Python scope like modules, classes and
  functions.  Scopes have a weakref to their parent, and strong refs to all
  their children.

  The constructor doesn't populate the scope's children, you have to call
  Populate().
  """

  IGNORED_NODES = (
    ast.Assert,
    ast.AugAssign,
    ast.Break,
    ast.Continue,
    ast.Delete,
    ast.Expr,
    ast.Pass,
    ast.Raise,
    ast.Return,
  )

  def __init__(self, ctx, node, parent=None):
    self.members = {}
    self.global_ids = set()
    self.parent = WeakrefOrNone(parent)

    self.node = node

    # Add ourself to the parent
    if parent is None:
      self.pb = codemodel_pb2.Scope()
    else:
      self.pb = parent.pb.child_scopes.add()

    # Set the Scope position
    ctx.SetPos(node, self.pb.declaration_pos)

    # Set the Scope type
    if isinstance(node, ast.Module):
      self.pb.type = codemodel_pb2.Scope.MODULE
    elif isinstance(node, ast.ClassDef):
      self.pb.type = codemodel_pb2.Scope.CLASS
    elif isinstance(node, ast.FunctionDef):
      self.pb.type = codemodel_pb2.Scope.FUNCTION
    else:
      raise ValueError("Unhandled Scope node type %s" % str(node))

    # Set the scope name
    if hasattr(node, "name"):
      self.pb.name = node.name

  def AddVariable(self, ctx, node, name, type):
    ret = self.pb.child_variables.add()
    ctx.SetPos(node, ret.declaration_pos)
    ret.name = name
    ret.type = type

    self.members[name] = ret
    return ret

  def AddScope(self, ctx, node):
    ret = Scope(ctx, node, self)

    self.members[ret.pb.name] = ret
    return ret

  def Populate(self, ctx):
    """
    Populates the members dictionary with this scope's immediate children and
    then calls Populate() on each one.

    The two-phase initialisation is important so members from a parent scope
    that were declared after this scope are visible.
    """

    if self.node is not None:
      self.HandleChildNodes(ctx, self.node.body)
      self.node = None

      for member in self.members.values():
        if isinstance(member, Scope):
          member.Populate(ctx)

  def HandleChildNodes(self, ctx, children):
    """
    If children is a list, calls HandleChildNode on each one.
    """

    if children is None:
      return

    for child in children:
      self.HandleChildNode(ctx, child)

  def HandleChildNode(self, ctx, node):
    """
    Creates the right Thing subclass for this node, and adds it to this scope's
    member list.
    """

    if isinstance(node, ast.Import):
      for name in [x.name for x in node.names]:
        var = self.AddVariable(ctx, node, name, codemodel_pb2.Variable.MODULE_REF)
        var.possible_type_ids.append(name)

    elif isinstance(node, ast.ImportFrom):
      for name in [x.name for x in node.names]:
        var = self.AddVariable(ctx, node, name, codemodel_pb2.Variable.MODULE_REF)
        var.possible_type_ids.append("%s.%s" % (node.module, name))

    elif isinstance(node, ast.FunctionDef):
      func = self.AddScope(ctx, node)

      # Add arguments
      for index, arg_name in enumerate(node.args.args):
        arg = func.AddVariable(ctx, node, arg_name.id, codemodel_pb2.Variable.FUNCTION_ARGUMENT)

        if index == 0 and self.pb.type == codemodel_pb2.Scope.CLASS:
          # TODO: handle staticmethod and classmethod decorators
          # TODO: fully qualified type ID
          arg.possible_type_ids.append(self.pb.name)

    elif isinstance(node, ast.ClassDef):
      self.AddScope(ctx, node)

    elif isinstance(node, ast.Global):
      for name in node.names:
        self.global_ids.add(name)

    elif isinstance(node, (ast.For, ast.If, ast.While)):
      self.HandleChildNodes(ctx, node.body)
      self.HandleChildNodes(ctx, node.orelse)

    elif isinstance(node, ast.TryExcept):
      self.HandleChildNodes(ctx, node.body)
      self.HandleChildNodes(ctx, node.orelse)

      for handler in getattr(node, "handlers", []):
        self.HandleChildNodes(ctx, node.body)

    elif isinstance(node, ast.Assign):
      for target in node.targets:
        if isinstance(target, ast.Name):
          self.AddVariable(ctx, node, target.id, codemodel_pb2.Variable.SCOPE_VARIABLE)

        elif isinstance(target, ast.Attribute):
          if isinstance(target.value, ast.Name):
            # Only support one level deep attribute assignments (like self.foo)
            # for now.  In the future we should resolve whole chains like
            # self.foo.bar.baz.
            parent = self.ResolveIdentifier(target.value.id)
            if isinstance(parent, Scope):
              parent.AddVariable(ctx, node, target.attr, codemodel_pb2.Variable.SCOPE_VARIABLE)

    elif isinstance(node, self.IGNORED_NODES):
      pass

    else:
      print "Unhandled %s: %s" % (node.__class__.__name__, node._attributes)

  def ResolveIdentifier(self, name):
    """
    Tries to find a Thing subclass that is in this scope or a parent scope.
    Returns None if nothing was found.
    This is supposed to have the same behaviour as Python's variable lookup
    if you were to use "name" in this scope.
    """

    if name in self.members:
      return self.members[name]

    if self.parent is not None:
      return self.parent().ResolveIdentifier(name)

    return None


if __name__ == "__main__":
  filename = sys.argv[1]
  source = open(filename).read()

  root = ast.parse(source)

  ctx = ParseContext(filename)
  scope = Scope(ctx, root)
  scope.Populate(ctx)

  print scope.pb
