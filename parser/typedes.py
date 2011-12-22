"""
Utility functions for dealing with type descriptors.

A type descriptor is a set of instructions that can be followed to get a
concrete type ID.  A descriptor only makes sense within a given pair of
(locals, globals) scopes.  This is usually the local and global scope at the
point the type descriptor was created.
"""

import ast
import logging

import codemodel_pb2
import parse

LOGGER = logging.getLogger("typedes")

LITERAL_AST_NODES = {
  ast.Dict: "dict",
  ast.Num:  "int",
  ast.Str:  "str",
  ast.List: "list",
}

if hasattr(ast, "Set"):
  # Python 2.7
  LITERAL_AST_NODES[ast.Set] = "set"


class TypeDescriptorError(Exception):
  """
  I don't know how to build a type descriptor from this AST.
  """

  pass


def BuildTypeDescriptor(node, type_pb):
  """
  node is an AST expression.  Fills type_pb (which is a codemodel_pb2.Type
  instance) with Reference objects that describe the final type of the
  expression.

  Raises TypeDescriptorError if the expression was too complicated.
  """

  if isinstance(node, ast.Name):
    if node.id == "None":
      SetAbsoluteType("NoneType", type_pb)
    else:
      ref = type_pb.reference.add()
      ref.kind = codemodel_pb2.Type.Reference.SCOPE_LOOKUP
      ref.name = node.id

  elif isinstance(node, ast.Attribute):
    BuildTypeDescriptor(node.value, type_pb)

    ref = type_pb.reference.add()
    ref.kind = codemodel_pb2.Type.Reference.SCOPE_LOOKUP
    ref.name = node.attr

  elif isinstance(node, ast.Call):
    BuildTypeDescriptor(node.func, type_pb)
    type_pb.reference[-1].kind = codemodel_pb2.Type.Reference.SCOPE_CALL

  else:
    if type(node) in LITERAL_AST_NODES:
      SetAbsoluteType(LITERAL_AST_NODES[type(node)], type_pb)
    else:
      raise TypeDescriptorError(node)


def TypeDebugString(type_pb):
  """
  Returns a short string describing the type.  This is useful for debugging.
  """

  parts = []

  for ref in type_pb.reference:
    parts.append(ref.name)
    if ref.kind == codemodel_pb2.Type.Reference.SCOPE_CALL:
      parts[-1] += "()"

  return ".".join(parts)


def ScopeLookupGenerator(ctx, local_scope, global_scope, recursion_guard=None):
  """
  Returns a generator that yields locals, base classes, and globals.
  """

  if local_scope is not None:
    # Look in locals first
    yield local_scope

    # Next look in base classes
    for base_type in local_scope.pb.base_type:
      (new_local, new_global, _) = \
          ResolveType(ctx, base_type, None, global_scope, recursion_guard)
      for child in ScopeLookupGenerator(ctx, new_local, new_global, recursion_guard):
        yield child

  # Finally look in the global scope
  if global_scope is not None:
    yield global_scope


def ResolveTypeReference(ctx, ref, local_scope, global_scope, recursion_guard=None):
  """
  Given a type reference, returns (new_locals, new_globals, full_type_id).
  new_locals and new_globals are the appropriate scopes for the returned type.
  """

  LOGGER.debug("Resolving '%s' (%s) in %s, %s",
               ref.name, ref.kind, local_scope, global_scope)

  ret = None

  if ref.kind == codemodel_pb2.Type.Reference.ABSOLUTE_TYPE_ID:
    ret = ref.name

  elif ref.kind == codemodel_pb2.Type.Reference.SCOPE_LOOKUP:
    for scope in ScopeLookupGenerator(ctx, local_scope, global_scope, recursion_guard):
      LOGGER.debug("Scope lookup for '%s' now in %s", ref.name, scope)
      member = scope.LookupName(ref.name)
      if isinstance(member, parse.Scope):
        ret = member.FullName()
        break

      elif isinstance(member, codemodel_pb2.Variable):
        ret = member.type
        break

  elif ref.kind == codemodel_pb2.Type.Reference.SCOPE_CALL:
    for scope in ScopeLookupGenerator(ctx, local_scope, global_scope, recursion_guard):
      LOGGER.debug("Scope lookup for '%s' now in %s", ref.name, scope)
      member = scope.LookupName(ref.name)
      if isinstance(member, parse.Scope):
        ret = member.pb.call_type
        break

  new_locals = None
  new_globals = None
  if isinstance(ret, str):
    try:
      new_locals = ctx.types[ret]()
    except KeyError:
      pass
    else:
      new_globals = new_locals.Module()

  elif isinstance(ret, codemodel_pb2.Type):
    (new_locals, new_globals, ret) = \
        ResolveType(ctx, ret, local_scope, global_scope, recursion_guard)

  return (new_locals, new_globals, ret)


def ResolveType(ctx, type_pb, local_scope, global_scope, recursion_guard=None):
  """
  Given a type, returns (new_locals, new_globals, full_type_id).
  """

  if not type_pb.reference:
    return (None, None, None)

  if not recursion_guard:
    recursion_guard = set()

  recursion_key = (TypeDebugString(type_pb), str(local_scope), str(global_scope))
  if recursion_key in recursion_guard:
    LOGGER.debug("Recursion detected, giving up")
    return (None, None, None)

  recursion_guard.add(recursion_key)


  LOGGER.debug("Resolving full type '%s' in %s, %s",
               TypeDebugString(type_pb), local_scope, global_scope)

  ret = None

  for ref in type_pb.reference:
    (local_scope, global_scope, ret) = \
      ResolveTypeReference(ctx, ref, local_scope, global_scope, recursion_guard)

  LOGGER.debug("Result: '%s' in %s, %s", ret, local_scope, global_scope)
  return (local_scope, global_scope, ret)


def SetAbsoluteType(dotted_name, type_pb):
  """
  Convenience function for setting a Type protobuf to an absolute type ID.
  """

  del type_pb.reference[:]
  ref = type_pb.reference.add()
  ref.kind = codemodel_pb2.Type.Reference.ABSOLUTE_TYPE_ID
  ref.name = dotted_name
