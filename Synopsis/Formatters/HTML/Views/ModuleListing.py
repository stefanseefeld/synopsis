#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Processor import Parameter
from Synopsis import ASG
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
      module = self.processor.ir.asg.declarations[0]
      self.index_module(module, module.name)
      self.write('</ul>')
      self.end_file()

   def _link_href(self, module):
      """Returns the link to the given declaration"""

      return rel(self.filename(),
                 self.directory_layout.module_index(module.name))

   def get_children(self, decl):
      """Returns the children of the given declaration"""

      try: return self._children_cache[decl]
      except KeyError: pass
      children = [c for c in decl.declarations
                  if isinstance(c, ASG.Module) and
                  (not self.child_types or child.type in self.child_types)]
      self._children_cache[decl] = children
      return children

   def index_module(self, module, rel_scope):
      "Write a link for this module and recursively visit child modules."

      my_scope = module.name
      # Find children, and sort so that compound children (packages) go first
      children = self.get_children(module)
      children.sort(lambda a,b,g=self.get_children:
                    cmp(len(g(b)),len(g(a))))
      # Print link to this module
      name = str(rel_scope.prune(my_scope)) or 'Global %s'%module.type.capitalize()
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

