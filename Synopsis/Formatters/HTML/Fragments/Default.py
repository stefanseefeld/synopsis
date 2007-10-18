#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Formatters.HTML.Fragment import Fragment

class Default(Fragment):
   """A base ASG strategy that calls format_declaration for all types"""

   # All these use the same method:
   def format_forward(self, decl): return self.format_declaration(decl)
   def format_group(self, decl): return self.format_declaration(decl)
   def format_scope(self, decl): return self.format_declaration(decl)
   def format_module(self, decl): return self.format_declaration(decl)
   def format_meta_module(self, decl): return self.format_declaration(decl)
   def format_class(self, decl): return self.format_declaration(decl)
   def format_typedef(self, decl): return self.format_declaration(decl)
   def format_enum(self, decl): return self.format_declaration(decl)
   def format_variable(self, decl): return self.format_declaration(decl)
   def format_const(self, decl): return self.format_declaration(decl)
   def format_function(self, decl): return self.format_declaration(decl)
   def format_operation(self, decl): return self.format_declaration(decl)
