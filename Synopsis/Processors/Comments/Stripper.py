#
# Copyright (C) 2005 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis import AST
from Filter import Filter

class Stripper(Filter):
    """Strip off all but the last comment."""

    def visitDeclaration(self, decl):

        self.strip(decl)

    def visitBuiltin(self, decl):

        if decl.type() == 'EOS': # treat it if it is an 'end of scope' marker
            self.strip(decl)

    def strip(self, decl):
        """Summarize the comment of this declaration."""

        if not len(decl.comments()):
            return
        if decl.comments()[-1].is_suspect():
            decl.comments()[:] = []
        else:
            decl.comments()[:] = [decl.comments()[-1]]

