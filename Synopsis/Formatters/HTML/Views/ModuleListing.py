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

class ModuleListing(View):
   """Create an index of all modules."""

   short_title = Parameter('Modules', 'short title')
   child_types = Parameter(None, 'the types of children to include')

   def register(self, processor):

      View.register(self, processor)
      self._children_cache = {}
      filename = self.filename()
      processor.set_contents_view(filename)
      self._link_target = 'index'

   def filename(self):

      return self.processor.file_layout.special('ModuleListing')

   def title(self):

      return self.short_title

   def menu_item(self):
      """Return a pair (url, title, target, frame) to be used as navigation bar item."""

      return self.filename(), self.title(), 'contents', 'contents'

   def process(self, start):
      """Create a view with an index of all modules"""
      # Init tree
      self.tree = self.processor.tree_formatter
      self.tree.register(self)
      # Create the file
      self.start_file()
      self.write(self.processor.navigation_bar(self.filename(), 'contents'))
      self.tree.start_tree()
      self.index_module(start, start.name())
      self.tree.end_tree()
      self.end_file()

   def _child_filter(self, child):
      """Returns true if the given child declaration is to be included"""

      if not isinstance(child, AST.Module): return 0
      if self.child_types and child.type() not in self.child_types:
         return 0
      return 1

   def _link_href(self, ns):
      """Returns the link to the given declaration"""

      return rel(self.filename(),
                 self.processor.file_layout.module_index(ns.name()))

   def _get_children(self, decl):
      """Returns the children of the given declaration"""

      try: return self._children_cache[decl]
      except KeyError: pass

      sorter = self.processor.sorter
      sorter.set_scope(decl)
      sorter.sort_sections()
      children = sorter.children()
      children = filter(self._child_filter, children)
      self._children_cache[decl] = children
      return children

   def index_module(self, ns, rel_scope):
      "Write a link for this module and recursively visit child modules."

      my_scope = ns.name()
      # Find children, and sort so that compound children (packages) go first
      children = self._get_children(ns)
      children.sort(lambda a,b,g=self._get_children:
                    cmp(len(g(b)),len(g(a))))
      # Print link to this module
      name = Util.ccolonName(my_scope, rel_scope) or "Global&#160;Namespace"
      link = self._link_href(ns)
      text = href(link, name, target=self._link_target)
      if not len(children):
         self.tree.write_leaf(text)
      else:
         self.tree.write_node_start(text)
         # Add children
         for child in children:
            self.index_module(child, my_scope)
         self.tree.write_node_end()

