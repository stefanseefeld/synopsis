#
# Copyright (C) 2005 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Processor import Processor, Parameter
from Synopsis import AST

class Previous(Processor, AST.Visitor):
    """A class that maps comments that begin with '<' to the previous
    declaration"""

    def process(self, ast, **kwds):
        """decorates process() to initialise last and laststack"""

        self.set_parameters(kwds)
        self.ast = self.merge_input(ast)

        self.last = None
        self.__laststack = []
        for decl in ast.declarations():
            decl.accept(self)
            self.last = decl

        return self.output_and_return_ast()

    def push(self):
        """decorates push() to also push 'last' onto 'laststack'"""

        self.__laststack.append(self.last)
        self.last = None

    def pop(self):
        """decorates pop() to also pop 'last' from 'laststack'"""

        self.last = self.__laststack.pop()

    def visitScope(self, scope):
        """overrides visitScope() to set 'last' after each declaration"""

        self.push()
        for decl in scope.declarations():
            decl.accept(self)
            self.last = decl
        self.pop()

    def visitDeclaration(self, decl):
        self.process_comments(decl)

    def visitBuiltin(self, decl):
        if decl.type() == 'EOS': # treat it if it is an 'end of scope' marker
            self.process_comments(decl)

    def visitEnum(self, enum):
        """Does the same as visitScope but for enum and enumerators"""

        self.push()
        for enumor in enum.enumerators():
            enumor.accept(self)
            self.last = enumor
        if enum.eos: enum.eos.accept(self)
        self.pop()

    def visitEnumerator(self, enumor):
        """Checks previous comment and removes dummies"""

        self.process_comments(enumor)

    def process_comments(self, decl):
        """Checks a decl to see if the comment should be moved. If the comment
        begins with a less-than sign, then it is moved to the 'last'
        declaration"""

        if len(decl.comments()) and self.last:
            first = decl.comments()[0]
            if len(first.text()) and first.text()[0] == "<" and self.last:
                if self.debug:
                    print 'found comment for previous in', decl.name()
                first.set_suspect(0) # Remove suspect flag
                first.set_text(first.text()[1:]) # Remove '<'
                self.last.comments().append(first)
                del decl.comments()[0]

