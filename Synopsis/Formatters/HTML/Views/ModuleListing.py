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

class ModuleListing(View):
   """Create an index of all modules."""

   short_title = Parameter('Modules', 'short title')
   child_types = Parameter(None, 'the types of children to include')

   def register(self, frame):

      super(ModuleListing, self).register(frame)
      self._children_cache = {}

   def filename(self):

      return self.directory_layout.special('ModuleListing')

   def title(self):

      return self.short_title

   def root(self):

      return self.filename(), self.title()

   def process(self):
      """Create a view with an index of all modules."""

      # Create the file
      self.start_file()
      self.write_navigation_bar()
      self.write('<ul class="tree">')
      # FIXME: see HTML.Formatter
      module = self.processor.ir.declarations[0]
      self.index_module(module, module.name())
      self.write('</ul>')
      self.end_file()

   def _child_filter(self, child):
      """Returns true if the given child declaration is to be included"""

      if not isinstance(child, AST.Module): return 0
      if self.child_types and child.type() not in self.child_types:
         return 0
      return 1

   def _link_href(self, module):
      """Returns the link to the given declaration"""

      return rel(self.filename(),
                 self.directory_layout.module_index(module.name()))

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

   def index_module(self, module, rel_scope):
      "Write a link for this module and recursively visit child modules."

      my_scope = module.name()
      # Find children, and sort so that compound children (packages) go first
      children = self._get_children(module)
      children.sort(lambda a,b,g=self._get_children:
                    cmp(len(g(b)),len(g(a))))
      # Print link to this module
      name = Util.ccolonName(my_scope, rel_scope) or 'Global Module'
      link = self._link_href(module)
      text = href(link, name, target='detail')
      if not len(children):
         self.write('<li>%s</li>'%text)
      else:
         self.write('<li>%s<ul class="tree">'%text)
         # Add children
         for child in children:
            self.index_module(child, my_scope)
         self.write('</ul></li>')

