#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Formatters.HTML.Tags import *
from Default import Default

class InheritanceFormatter(Default):
   """Prints just the name of each declaration, with a link to its doc"""

   def format_declaration(self, decl, label=None):

      if not label: label = decl.name[-1]
      qname = self.formatter.scope().prune(decl.name)
      title = decl.type + ' ' + escape(str(qname))
      return self.reference(decl.name, label=label, title=title) + ' '

   def format_function(self, decl):

      return self.format_declaration(decl, label=decl.real_name[-1])

   def format_operation(self, decl): return self.format_function(decl)
