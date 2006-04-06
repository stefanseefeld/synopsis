#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Processor import Parameter
from Synopsis import AST, Util
from Synopsis.Formatters.HTML.View import View
from Synopsis.Formatters.HTML.Tags import *

import os

class ModuleIndexer(View):
   """A module for indexing AST.Modules. Each module gets its own view with a
   list of nested scope declarations with documentation. It is intended to go in
   the left frame..."""

   def register(self, processor):
      """Register first view as index view"""

      View.register(self, processor)
      processor.set_using_module_index()
      self.__filename = self.processor.file_layout.module_index(())
      processor.set_index_view(self.__filename)

   def filename(self): return self.__filename

   def title(self): return self.__title

   def process(self, start):
      """Creates indexes for all modules"""

      start_file = self.processor.file_layout.module_index(start.name())
      self.processor.set_index_view(start_file)
      self.__namespaces = [start]
      while self.__namespaces:
         ns = self.__namespaces.pop(0)
         self.process_namespace_index(ns)
    
   def make_view_heading(self, ns):
      """Creates a HTML fragment which becomes the name at the top of the
      index view. This may be overridden, but the default is (now) to make a
      linked fully scoped name, where each scope has a link to the relevant
      index."""

      name = ns.name()
      if not name: return 'Global Index'
      links = []
      for depth in range(0, len(name)):
         url = self.processor.file_layout.module_index(name[:depth+1])
         label = escape(name[depth])
         links.append(href(rel(self.__filename, url), label))
      return entity('b', string.join(links, '\n::') + ' Index')

   def process_namespace_index(self, ns):
      "Index one module"

      sorter = self.processor.sorter
      sorter.set_scope(ns)
      sorter.sort_section_names()
      sorter.sort_sections()

      self.__filename = self.processor.file_layout.module_index(ns.name())
      self.__title = Util.ccolonName(ns.name()) or 'Global Namespace'
      self.__title = self.__title + ' Index'
      # Create file
      self.start_file()
      #target = rel(self.__filename, self.processor.file_layout.scope(ns.name()))
      #link = href(target, self.__title, target='main')
      self.write(self.make_view_heading(ns))

      toc = self.processor.toc

      # Make script to switch main frame upon load
      load_script = '<!--\n'
      if toc[ns.name()]:
         target = rel(self.__filename, toc[ns.name()].link)
         load_script = load_script + 'window.parent.frames["main"].location="%s";\n'%target
      load_script = load_script + 'function go(index,main) {\n'\
                    'window.parent.frames["index"].location=index;\n'\
                    'window.parent.frames["main"].location=main;\n'\
                    'return false;}\n-->'
      self.write(entity('script', load_script, type='text/javascript'))

      # Loop throught all the types of children
      for section in sorter.sections():
         if section[-1] == 's': heading = section+'es'
         else: heading = section+'s'
         heading = '<br/>\n'+entity('i', escape(heading))+'<br/>\n'
         # Get a list of children of this type
         for child in sorter.children(section):
            # Print out summary for the child
            if not isinstance(child, AST.Scope):
               continue
            if heading:
               self.write(heading)
               heading = None
            label = Util.ccolonName(child.name(), ns.name())
            label = escape(label)
            label = replace_spaces(label)
            if isinstance(child, AST.Module):
               index_url = rel(self.__filename,
                               self.processor.file_layout.module_index(child.name()))
               self.write(href(index_url, label, target='index'))
            else:
               entry = toc[child.name()]
               if entry:
                  url = rel(self.__filename, entry.link)
                  self.write(href(url, label, target='main'))
               else:
                  self.write(label)
            self.write('<br/>\n')
      self.end_file()

      # Queue child namespaces
      for child in sorter.children():
         if isinstance(child, AST.Module):
            self.__namespaces.append(child)
