class AstVisitor :
    """Visitor for AST nodes

Functions:

  visitAST(node)
  visitDeclaration(node)
  visitForward(node)
  visitDeclarator(node)
  visitModule(node)
  visitClass(node)
  visitTypedef(node)
  visitEnumerator(node)
  visitEnum(node)
  visitVariable(node)
  visitConst(node)
  visitParameter(node)
  visitFunction(node)
  visitOperation(node)
  """

    def visitAST(self, node): return
    def visitDeclaration(self, node): return
    def visitForward(self, node): return
    def visitDeclarator(self, node): return
    def visitModule(self, node): return
    def visitMetaModule(self, node): return
    def visitClass(self, node): return
    def visitTypedef(self, node): return
    def visitEnumerator(self, node): return
    def visitEnum(self, node): return
    def visitVariable(self, node): return
    def visitConst(self, node): return
    def visitParameter(self, node): return
    def visitFunction(self, node): return
    def visitOperation(self, node): return

class TypeVisitor:
    """Visitor for Type objects

Functions:

  visitBaseType(type)
  visitDeclared(type)
  visitModifier(node)
  visitTemplate(type)
  visitParametrized(type)"""

    def visitBaseType(self, type):     return
    def visitDeclared(self, type): return
    def visitModifier(self, type): return
    def visitTemplate(self, type): return
    def visitParametrized(self, type): return
