#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Processor import Parameter
from Synopsis import AST, Type
from Synopsis.Formatters.HTML.View import View
from Synopsis.Formatters.HTML.Tags import *

import os

class NameIndex(View):
   """Creates an index of all names on one view in alphabetical order"""

   columns = Parameter(2, 'the number of columns for the listing')

   def filename(self):

      return self.directory_layout.special('NameIndex')

   def title(self):

      return 'Name Index'

   def root(self):

      return self.filename(), self.title()

   def process(self):

      self.start_file()
      self.write_navigation_bar()
      self.write(entity('h1', "Name Index"))
      self.write('<i>Hold the mouse over a link to see the scope of each name</i>\n')

      dict = self.make_dictionary()
      keys = dict.keys()
      keys.sort()
      linker = lambda key: '<a href="#%s">%s</a>'%(ord(key),key)
      self.write(div('nameindex-index', ''.join([linker(k) for k in keys])) + '\n')
      for key in keys:
         self.write('<a name="%s">'%ord(key)+'</a>')
         self.write(entity('h2', key) + '\n')
         self.write('<table border="0" width="100%" summary="table of names">\n')
         self.write('<col width="*"/>'*self.columns + '\n')
         self.write('<tr>\n')
         items = dict[key]
         numitems = len(items)
         start = 0
         for column in range(self.columns):
            end = numitems * (column + 1) / self.columns
            self.write('<td valign="top">\n')
            for item in items[start:end]:
               self._process_item(item)
            self.write('</td>\n')
            start = end
         self.write('</tr>\n</table>\n')
	
      self.end_file()

   def make_dictionary(self):
      """Returns a dictionary of items. The keys of the dictionary are the
      headings - the first letter of the name. The values are each a sorted
      list of items with that first letter."""

      decl_filter = lambda type: (isinstance(type, Type.Declared)
                                  and not isinstance(type.declaration(), AST.Builtin))
      def name_cmp(a,b):
         a, b = a.name(), b.name()
         res = cmp(a[-1],b[-1])
         if res == 0: res = cmp(a,b)
         return res
      dict = {}
      def hasher(type, dict=dict):
         name = type.name()
         try: key = name[-1][0]
         except:
            print 'name:',name, 'type:',repr(type)
            raise
         if key >= 'a' and key <= 'z': key = chr(ord(key) - 32)
         if dict.has_key(key): dict[key].append(type)
         else: dict[key] = [type]
      # Fill the dict
      map(hasher, filter(decl_filter, self.processor.ast.types().values()))
      # Now sort the dict
      for items in dict.values():
         items.sort(name_cmp)
      return dict

   def _process_item(self, type):
      """Process the given name for output"""

      name = type.name()
      decl = type.declaration() # non-declared types are filtered out
      if isinstance(decl, AST.Function):
         realname = escape(decl.realname()[-1]) + '()'
      else:
         realname = escape(name[-1])
      self.write('\n')
      title = escape(string.join(name, '::'))
      type = decl.type()
      name = self.reference(name, (), realname, title=title)+' '+type
      self.write(div('nameindex-item', name))
