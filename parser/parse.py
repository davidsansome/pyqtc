"""
Converts an AST to a CodeModel protobuf.
"""

import ast
import logging
import weakref

import codemodel_pb2
import typedes

LOGGER = logging.getLogger("parse")


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

  def __init__(self, filename, module_name, line_offset=0):
    self.filename = filename
    self.module_name = module_name
    self.line_offset = line_offset

    self.types = {}

  def SetPos(self, node, pos_out):
    """
    Fills the Position protobuf pos_out with the code location of the AST node.
    """

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
    ast.Exec,
    ast.Expr,
    ast.Pass,
    ast.Print,
    ast.Raise,
    ast.Return,
  )

  def __init__(self, ctx, node, parent=None, pb=None, is_instance=False):
    self.members = {}
    self.member_variable_names = set()
    self.global_ids = set()
    self.parent = WeakrefOrNone(parent)
    self.instance_type = None

    self.node = node

    # Add ourself to the parent
    if parent is None:
      if pb is None:
        self.pb = codemodel_pb2.Scope()
      else:
        self.pb = pb
    else:
      self.pb = parent.pb.child_scope.add()

    # Set the Scope position
    ctx.SetPos(node, self.pb.declaration_pos)

    # Set the Scope kind
    if isinstance(node, ast.Module):
      self.pb.kind = codemodel_pb2.Scope.MODULE

    elif isinstance(node, ast.ClassDef):
      self.pb.kind = codemodel_pb2.Scope.CLASS

      # Get the type descriptors of any base classes.
      for base in node.bases:
        typedes.BuildTypeDescriptor(base, self.pb.base_type.add())

    elif isinstance(node, ast.FunctionDef):
      self.pb.kind = codemodel_pb2.Scope.FUNCTION

    else:
      raise ValueError("Unhandled Scope node type %s" % str(node))

    # Set the scope name
    if isinstance(node, ast.Module):
      self.pb.name = ctx.module_name
    else:
      self.pb.name = node.name

    # Classes get a special instance scope as well
    if not is_instance and isinstance(node, ast.ClassDef):
      # Create the instance type
      self.instance_type = Scope(ctx, node, parent, is_instance=True)

      # Set the class type as the only base of the instance type
      del self.instance_type.pb.base_type[:]
      typedes.SetAbsoluteType(self.FullName(),
                              self.instance_type.pb.base_type.add())

      # Calling the class type should give you the instance type
      typedes.SetAbsoluteType(self.instance_type.FullName(),
                              self.pb.call_type)

    if is_instance:
      self.node = None
      self.pb.name += "<instance>"

    # Register this type with the context
    ctx.types[self.FullName()] = weakref.ref(self)

  def FullName(self):
    """
    Returns the full dotted type name of this Scope.
    """

    if not self.parent:
      return self.pb.name
    return self.parent().FullName() + "." + self.pb.name

  def __str__(self):
    return "<%s>" % self.FullName()

  def Module(self):
    """
    Returns the highest ancestor of this Scope.
    """

    if not self.parent:
      return self
    return self.parent().Module()

  def AddVariable(self, ctx, node, name, var_kind):
    """
    Adds a new variable to this scope.  var_kind is one of the protobuf Variable
    types.  Returns the Variable protobuf.
    """

    try:
      existing_member = self.members[name]
    except KeyError:
      pass
    else:
      if isinstance(existing_member, codemodel_pb2.Variable):
        return existing_member
      return None

    ret = self.pb.child_variable.add()
    ctx.SetPos(node, ret.declaration_pos)
    ret.name = name
    ret.kind = var_kind

    self.members[name] = ret
    return ret

  def AddScope(self, ctx, node):
    """
    Adds a new scope (either a function or a class) to this scope.  Returns the
    Scope instance.
    """

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

  def _GetArgNames(self, thing):
    """
    Returns a generator that yields an ast.Name for each argument in "thing",
    which should be an ast.arguments instance.
    Copes with nested tuples as arguments.
    """

    if isinstance(thing, ast.arguments):
      for arg in thing.args:
        for name in self._GetArgNames(arg):
          yield name

    elif isinstance(thing, ast.Name):
      yield thing

    elif isinstance(thing, ast.Tuple):
      for elt in thing.elts:
        for name in self._GetArgNames(elt):
          yield name

  def HandleChildNode(self, ctx, node):
    """
    Creates the right Thing subclass for this node, and adds it to this scope's
    member list.
    """

    if isinstance(node, ast.Import):
      for name in node.names:
        asname = name.asname
        if not asname:
          asname = name.name

        var = self.AddVariable(ctx, node, asname, codemodel_pb2.Variable.MODULE_REF)
        typedes.SetAbsoluteType(name.name, var.type)

    elif isinstance(node, ast.ImportFrom):
      for name in node.names:
        asname = name.asname
        if not asname:
          asname = name.name

        var = self.AddVariable(ctx, node, asname, codemodel_pb2.Variable.MODULE_REF)
        typedes.SetAbsoluteType("%s.%s" % (node.module, name.name), var.type)

    elif isinstance(node, ast.FunctionDef):
      func = self.AddScope(ctx, node)

      # Add arguments
      for index, arg_name in enumerate(self._GetArgNames(node.args)):
        arg = func.AddVariable(ctx, node, arg_name.id, codemodel_pb2.Variable.FUNCTION_ARGUMENT)

        if index == 0 and self.pb.kind == codemodel_pb2.Scope.CLASS:
          # TODO: handle staticmethod and classmethod decorators
          # TODO: fully qualified type ID
          typedes.SetAbsoluteType(self.instance_type.FullName(), arg.type)

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

      for _handler in node.handlers:
        self.HandleChildNodes(ctx, node.body)

    elif isinstance(node, ast.TryFinally):
      self.HandleChildNodes(ctx, node.body)
      self.HandleChildNodes(ctx, node.finalbody)

    elif isinstance(node, ast.Assign):
      for target in node.targets:
        self.HandleVariableAssignment(ctx, target, node.value)

    elif isinstance(node, ast.With):
      if node.optional_vars is not None:
        self.HandleVariableAssignment(ctx, node.optional_vars, node.context_expr)

      self.HandleChildNodes(ctx, node.body)

    elif isinstance(node, self.IGNORED_NODES):
      pass

    else:
      logging.warning("Unhandled %s", node.__class__.__name__)

  def HandleVariableAssignment(self, ctx, target, value):
    """
    Adds a scope variable to this scope for "target", which can be a name or an
    attribute access.  The type is inferred from value, which is an Expr.
    """

    var = None

    if isinstance(target, ast.Name):
      var = self.AddVariable(ctx, target, target.id, codemodel_pb2.Variable.SCOPE_VARIABLE)

    elif isinstance(target, ast.Attribute):
      # We have <parent>.attribute.  Try to find a scope for <parent> and
      # create an attribute inside it.

      # Try to parse a type descriptor out of <parent>
      parent_type = codemodel_pb2.Type()
      try:
        typedes.BuildTypeDescriptor(target.value, parent_type)
      except typedes.TypeDescriptorError:
        # Too complicated - never mind
        return

      # Resolve that to a type
      (parent_scope, _, _) = typedes.ResolveType(ctx, parent_type, self, self.Module())

      if parent_scope is not None:
        # Create the variable in that scope
        var = parent_scope.AddVariable(ctx, target, target.attr,
                                       codemodel_pb2.Variable.SCOPE_VARIABLE)

    if var is not None:
      var.ClearField("type")
      try:
        typedes.BuildTypeDescriptor(value, var.type)
      except typedes.TypeDescriptorError:
        return

      # Resolve the value type now if we can
      (_, _, value_type_id) = typedes.ResolveType(ctx, var.type, self, self.Module())
      if value_type_id is not None:
        typedes.SetAbsoluteType(value_type_id, var.type)

  def LookupName(self, name):
    """
    Looks for a child with the (non-dotted) name in this immediate scope.
    Doesn't look in parent or base scopes.
    """

    return self.members.get(name, None)
