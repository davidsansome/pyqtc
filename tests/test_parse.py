import unittest

import parse


class TestParse(unittest.TestCase):
  def parse(self, source):
    # Strip spaces from the start of lines
    lines = source.split("\n")
    spaces = min([len(x) - len(x.lstrip()) for x in lines if x.strip()])
    lines = [x[spaces:] for x in lines]

    return parse.ParseSource("\n".join(lines).strip(), "dummy.py", "dummy")

  def test_declaration_pos(self):
    scope = self.parse("""
      def one():
        pass

      def two():
        pass
    """)

    self.assertEqual(0, scope.pb.declaration_pos.line)
    self.assertEqual(1, scope.pb.child_scope[0].declaration_pos.line)
    self.assertEqual(4, scope.pb.child_scope[1].declaration_pos.line)

  def test_scope_variables(self):
    scope = self.parse("""
      def func_scope():
        var1 = None
        var2 = 123
        var3 = "hello"
    """)

    self.assertEqual(0, len(scope.pb.child_variable))
    self.assertEqual(3, len(scope.pb.child_scope[0].child_variable))
    self.assertEqual("var1", scope.pb.child_scope[0].child_variable[0].name)
    self.assertEqual("var2", scope.pb.child_scope[0].child_variable[1].name)
    self.assertEqual("var3", scope.pb.child_scope[0].child_variable[2].name)
    self.assertEqual(1, scope.pb.child_scope[0].child_variable[0].kind)
    self.assertEqual(1, scope.pb.child_scope[0].child_variable[1].kind)
    self.assertEqual(1, scope.pb.child_scope[0].child_variable[2].kind)
    self.assertEqual(2, scope.pb.child_scope[0].child_variable[0].declaration_pos.line)
    self.assertEqual(3, scope.pb.child_scope[0].child_variable[1].declaration_pos.line)
    self.assertEqual(4, scope.pb.child_scope[0].child_variable[2].declaration_pos.line)

  def test_class_call_type(self):
    scope = self.parse("""
      class C():
        def f(self):
          pass
      class_var = C
      class_instance = C()
      class_instance2 = class_var()
    """)

    self.assertEqual(2, len(scope.pb.child_scope))
    self.assertEqual("C", scope.pb.child_scope[0].name)
    self.assertEqual("C<instance>", scope.pb.child_scope[1].name)

    self.assertEqual(0, len(scope.pb.child_scope[0].base_type))
    self.assertEqual(1, len(scope.pb.child_scope[1].base_type))
    self.assertEqual("dummy.C", scope.pb.child_scope[1].base_type[0].reference[0].name)

    self.assertEqual(True, scope.pb.child_scope[0].HasField("call_type"))
    self.assertEqual(False, scope.pb.child_scope[1].HasField("call_type"))
    self.assertEqual("dummy.C<instance>", scope.pb.child_scope[0].call_type.reference[0].name)

    self.assertEqual(3, len(scope.pb.child_variable))
    self.assertEqual(0, scope.pb.child_variable[0].type.reference[0].kind)
    self.assertEqual("dummy.C", scope.pb.child_variable[0].type.reference[0].name)
    self.assertEqual(0, scope.pb.child_variable[1].type.reference[0].kind)
    self.assertEqual("dummy.C<instance>", scope.pb.child_variable[1].type.reference[0].name)
    self.assertEqual(2, scope.pb.child_variable[2].type.reference[0].kind)
    self.assertEqual("class_var", scope.pb.child_variable[2].type.reference[0].name)

  def test_self(self):
    scope = self.parse("""
      class C():
        def f(self):
          pass
    """)

    f = scope.pb.child_scope[0].child_scope[0]

    self.assertEqual(1, len(f.child_variable))
    self.assertEqual("self", f.child_variable[0].name)
    self.assertEqual(0, f.child_variable[0].kind)
    self.assertEqual("dummy.C<instance>", f.child_variable[0].type.reference[0].name)

  def test_self_assign(self):
    scope = self.parse("""
      class C():
        class_variable = 123
        def __init__(self):
          self.instance_variable = 456
    """)

    self.assertEqual(1, len(scope.pb.child_scope[0].child_variable))
    self.assertEqual(1, len(scope.pb.child_scope[1].child_variable))
    self.assertEqual("class_variable", scope.pb.child_scope[0].child_variable[0].name)
    self.assertEqual("instance_variable", scope.pb.child_scope[1].child_variable[0].name)

if __name__ == "__main__":
  unittest.main()
