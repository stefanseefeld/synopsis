#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis import AST, Type
from Tags import *
import string

class Fragment(object):
   """Generates HTML fragment for a declaration. Multiple strategies are
   combined to generate the output for a single declaration, allowing the
   user to customise the output by choosing a set of strategies. This follows
   the Strategy design pattern.
   
   The key concept of this class is the format* methods. Any
   class derived from Strategy that overrides one of the format methods
   will have that method called by the Summary and Detail formatters when
   they visit that AST type. Summary and Detail maintain a list of
   Strategies, and a list for each AST type.
    
   For example, when Strategy.Summary visits a Function object, it calls
   the formatFunction method on all Strategys registed with
   SummaryFormatter that implemented that method. Each of these format
   methods returns a string, which may contain a TD tag to create a new
   column.

   An important point to note is that only Strategies which override a
   particular format method are called - if that format method is not
   overridden then it is not called for that declaration type.
   """

   def register(self, formatter):
      """Store formatter as self.formatter. The formatter is either a
      SummaryFormatter or DetailFormatter, and is used for things like
      reference() and label() calls. Local references to the formatter's
      reference and label methods are stored in self for more efficient use
      of them."""

      self.processor = formatter.processor
      self.formatter = formatter
      self.label = formatter.label
      self.reference = formatter.reference
      self.format_type = formatter.format_type
      self.view = formatter.view()

   #
   # Utility methods
   #
   def format_modifiers(self, modifiers):
      """Returns a HTML string from the given list of string modifiers. The
      modifiers are enclosed in 'keyword' spans."""

      def keyword(m):
         if m == '&': return span('keyword', '&amp;')
         return span('keyword', m)
      return string.join(map(keyword, modifiers))


   #
   # AST Formatters
   #
   def format_declaration(self, decl): pass
   def format_forward(self, decl): pass
   def format_group(self, decl): pass
   def format_scope(self, decl): pass
   def format_module(self, decl): pass
   def format_meta_module(self, decl): pass
   def format_class(self, decl): pass
   def format_typedef(self, decl): pass
   def format_enum(self, decl): pass
   def format_variable(self, decl): pass
   def format_const(self, decl): pass
   def format_function(self, decl): pass
   def format_operation(self, decl): pass
