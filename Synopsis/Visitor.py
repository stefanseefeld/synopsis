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
    def visitForward(self, node): self.visitDeclaration(node)
    def visitDeclarator(self, node): self.visitDeclaration(node)
    def visitScope(self, node): self.visitDeclaration(node)
    def visitModule(self, node): self.visitScope(node)
    def visitMetaModule(self, node): self.visitModule(node)
    def visitClass(self, node): self.visitScope(node)
    def visitTypedef(self, node): self.visitDeclaration(node)
    def visitEnumerator(self, node): self.visitDeclaration(node)
    def visitEnum(self, node): self.visitDeclaration(node)
    def visitVariable(self, node): self.visitDeclaration(node)
    def visitConst(self, node): self.visitDeclaration(node)
    def visitFunction(self, node): self.visitDeclaration(node)
    def visitOperation(self, node): self.visitDeclaration(node)
    def visitParameter(self, node): return
    def visitComment(self, node): return
    def visitInheritance(self, node): return

class TypeVisitor:
    """Visitor for Type objects

Functions:

  visitBaseType(type)
  visitDeclared(type)
  visitModifier(node)
  visitTemplate(type)
  visitParametrized(type)"""

    def visitBaseType(self, type): return
    def visitDeclared(self, type): return
    def visitModifier(self, type): return
    def visitTemplate(self, type): return
    def visitParametrized(self, type): return
    def visitFunctionType(self, type): return
