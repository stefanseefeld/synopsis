# $Id: ModuleIndexer.py,v 1.16 2003/11/14 14:51:09 stefan Exp $
#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Processor import Parameter
from Synopsis import AST, Util
from Synopsis.Formatters.HTML.Page import Page
from Synopsis.Formatters.HTML.core import config
from Synopsis.Formatters.HTML.Tags import *

import os

class ModuleIndexer(Page):
   """A module for indexing AST.Modules. Each module gets its own page with a
   list of nested scope declarations with comments. It is intended to go in
   the left frame..."""

   def register(self, processor):
      """Register first page as index page"""

      Page.register(self, processor)
      config.set_using_module_index()
      self.__filename = self.processor.file_layout.nameOfModuleIndex(())
      config.set_index_page(self.__filename)

   def filename(self): return self.__filename

   def title(self): return self.__title

   def process(self, start):
      """Creates indexes for all modules"""

      start_file = self.processor.file_layout.nameOfModuleIndex(start.name())
      config.set_index_page(start_file)
      self.__namespaces = [start]
      while self.__namespaces:
         ns = self.__namespaces.pop(0)
         self.processNamespaceIndex(ns)
    
   def _makePageHeading(self, ns):
      """Creates a HTML fragment which becomes the name at the top of the
      index page. This may be overridden, but the default is (now) to make a
      linked fully scoped name, where each scope has a link to the relevant
      index."""

      name = ns.name()
      if not name: return 'Global Index'
      links = []
      for depth in range(0, len(name)):
         url = self.processor.file_layout.nameOfModuleIndex(name[:depth+1])
         label = anglebrackets(name[depth])
         links.append(href(rel(self.__filename, url), label))
      return entity('b', string.join(links, '\n::') + ' Index')

   def processNamespaceIndex(self, ns):
      "Index one module"

      config.sorter.set_scope(ns)
      config.sorter.sort_section_names()
      config.sorter.sort_sections()

      self.__filename = self.processor.file_layout.nameOfModuleIndex(ns.name())
      self.__title = Util.ccolonName(ns.name()) or 'Global Namespace'
      self.__title = self.__title + ' Index'
      # Create file
      self.start_file()
      #target = rel(self.__filename, self.processor.file_layout.nameOfScope(ns.name()))
      #link = href(target, self.__title, target='main')
      self.write(self._makePageHeading(ns))

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
      for section in config.sorter.sections():
         if section[-1] == 's': heading = section+'es'
         else: heading = section+'s'
         heading = '<br>'+entity('i', heading)+'<br>'
         # Get a list of children of this type
         for child in config.sorter.children(section):
            # Print out summary for the child
            if not isinstance(child, AST.Scope):
               continue
            if heading:
               self.write(heading)
               heading = None
            label = Util.ccolonName(child.name(), ns.name())
            label = anglebrackets(label)
            label = replace_spaces(label)
            if isinstance(child, AST.Module):
               index_url = rel(self.__filename,
                               self.processor.file_layout.nameOfModuleIndex(child.name()))
               self.write(href(index_url, label, target='index'))
            else:
               entry = toc[child.name()]
               if entry:
                  url = rel(self.__filename, entry.link)
                  self.write(href(url, label, target='main'))
               else:
                  self.write(label)
            self.write('<br>')
      self.end_file()

      # Queue child namespaces
      for child in config.sorter.children():
         if isinstance(child, AST.Module):
            self.__namespaces.append(child)
