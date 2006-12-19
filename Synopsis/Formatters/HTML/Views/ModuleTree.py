#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis import AST, Util
from Synopsis.Formatters.HTML.Tags import *
from Tree import Tree

import os

class ModuleTree(Tree):
   """Create an index of all modules with JS. The JS allows the user to
   expand/collapse sections of the tree!"""

   def register(self, frame):

      super(ModuleTree, self).register(frame)
      self._children_cache = {}

   def filename(self):

      return self.directory_layout.special('ModuleTree')

   def title(self):

      return 'Module Tree'

   def root(self):

      return self.filename(), self.title()

   def process(self):

      # Creare the file
      self.start_file()
      self.write_navigation_bar()
      # FIXME: see HTML.Formatter
      module = self.processor.ast.declarations()[0]
      self.index_module(module, module.name())
      self.end_tree()
      self.end_file()

   def _child_filter(self, child):

      return isinstance(child, AST.Module)

   def _link_href(self, module):

      return self.directory_layout.module_index(module.name())

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

   def index_module(self, module, rel_scope):
      "Write a link for this module and recursively visit child modules."

      my_scope = module.name()
      # Find children, and sort so that compound children (packages) go first
      children = self.get_children(module)
      children.sort(lambda a,b,g=self.get_children:
                    cmp(len(g(b)),len(g(a))))
      # Print link to this module
      name = Util.ccolonName(my_scope, rel_scope) or 'Global Module'
      link = self._link_href(module)
      text = href(link, name, target='detail')
      if not len(children):
         self.write_leaf(text)
      else:
         self.write_node_start(text)
         # Add children
         for child in children:
            self.index_module(child, my_scope)
         self.write_node_end()

