# $Id: InheritanceFormatter.py,v 1.1 2003/12/05 22:31:53 stefan Exp $
#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Formatters.HTML.Tags import *
from Default import Default
from Synopsis import Util

class InheritanceFormatter(Default):
   """Prints just the name of each declaration, with a link to its doc"""

   def format_declaration(self, decl, label=None):

      if not label: label = decl.name()[-1]
      fullname = Util.ccolonName(decl.name(), self.formatter.scope())
      title = decl.type() + " " + anglebrackets(fullname)
      return self.reference(decl.name(), label=label, title=title) + ' '

   def format_function(self, decl):

      return self.format_declaration(decl, label=decl.realname()[-1])

   def format_operation(self, decl): return self.format_function(decl)
