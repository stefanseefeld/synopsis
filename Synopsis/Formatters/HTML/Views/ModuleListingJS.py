# $Id: ModuleListingJS.py,v 1.15 2003/12/08 00:39:24 stefan Exp $
#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis import config
from Synopsis import AST, Util
from Synopsis.Formatters.HTML.Tags import *
from JSTree import JSTree

import os

class ModuleListingJS(JSTree):
   """Create an index of all modules with JS. The JS allows the user to
   expand/collapse sections of the tree!"""

   def register(self, processor):

      JSTree.register(self, processor)
      self._children_cache = {}
      filename = self.processor.file_layout.special('ModuleTree')
      self.processor.set_contents_view(filename)
      self.processor.add_root_view(filename, 'Modules', 'contents', 2)
      self._link_target = 'index'

   def filename(self): return self.processor.file_layout.special('ModuleTree')
   def title(self): return 'Module Tree'

   def process(self, start):
      """Create a view with an index of all modules"""
      # Init tree
      share = config.datadir
      self.js_init(os.path.join(share, 'syn-down.png'),
                   os.path.join(share, 'syn-right.png'),
                   os.path.join(share, 'syn-dot.png'),
                   'tree_%s.png', 0)
      self.__share = share
      # Creare the file
      self.start_file()
      self.write(self.processor.navigation_bar(self.filename(), 2))
      self.indexModule(start, start.name())
      self.end_file()

   def _child_filter(self, child):

      return isinstance(child, AST.Module)

   def _link_href(self, ns):

      return self.processor.file_layout.module_index(ns.name())

   def get_children(self, decl):

      try: return self._children_cache[decl]
      except KeyError: pass
      sorter = self.processor.sorter
      sorter.set_scope(decl)
      sorter.sort_sections()
      children = sorter.children()
      children = filter(self._child_filter, children)
      self._children_cache[decl] = children
      return children

   def indexModule(self, ns, rel_scope):
      "Write a link for this module and recursively visit child modules."

      my_scope = ns.name()
      # Find children, and sort so that compound children (packages) go first
      children = self.get_children(ns)
      children.sort(lambda a,b,g=self.get_children:
                    cmp(len(g(b)),len(g(a))))
      # Print link to this module
      name = Util.ccolonName(my_scope, rel_scope) or "Global Namespace"
      link = self._link_href(ns)
      text = href(link, name, target=self._link_target)
      if not len(children):
         self.writeLeaf(text)
      else:
         self.writeNodeStart(text)
         # Add children
         for child in children:
            self.indexModule(child, my_scope)
         self.writeNodeEnd()

