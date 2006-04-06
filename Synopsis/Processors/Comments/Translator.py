#
# Copyright (C) 2006 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis import AST
from Synopsis.DocString import DocString
from Synopsis.Processor import Processor, Parameter

class Translator(Processor, AST.Visitor):
    """"""

    markup = Parameter('', 'The markup type for this declaration.')
    concatenate = Parameter(False, 'Whether or not to concatenate adjacent comments.')
    primary_only = Parameter(True, 'Whether or not to preserve secondary comments.')

    def process(self, ast, **kwds):
      
        self.set_parameters(kwds)
        self.ast = self.merge_input(ast)

        for decl in ast.declarations():
            decl.accept(self)

        return self.output_and_return_ast()


    def visitDeclaration(self, decl):

        comments = decl.annotations.get('comments')
        if comments:
            text = None
            if self.primary_only:
                text = comments[-1]
            elif self.combine:
                text = ''.join([c for c in comments if c])
            else:
                comments = comments[:]
                comments.reverse()
                for c in comments:
                    if c:
                        text = c
                        break
            doc = DocString(text, self.markup)
            decl.annotations['doc'] = doc
