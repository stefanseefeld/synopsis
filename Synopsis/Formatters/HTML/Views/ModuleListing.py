# $Id: ModuleListing.py,v 1.13 2003/11/12 16:42:05 stefan Exp $
#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Processor import Parameter
from Synopsis import AST, Util

import core
from Page import Page
from core import config
from Tags import *

import os

class ModuleListing(Page):
   """Create an index of all modules with JS. The JS allows the user to
   expand/collapse sections of the tree!"""

   def register(self, manager):

      Page.register(self, manager)
      self.child_types = None
      self._children_cache = {}
      self.__short_title = 'Modules'
      if hasattr(config.obj, 'ModuleListing'):
         myconfig = config.obj.ModuleListing
         if hasattr(myconfig, 'short_title'):
            self.__short_title = myconfig.short_title

      filename = self.filename()
      config.set_contents_page(filename)
      self.manager.addRootPage(filename, self.__short_title, 'contents', 2)
      self._link_target = 'index'

   def filename(self): return config.files.nameOfSpecial('ModuleListing')

   def title(self): return self.__short_title + ' Listing'

   def process(self, start):
      """Create a page with an index of all modules"""
      # Init tree
      self.tree = config.treeFormatterClass(self)
      # Init list of module types to display
      try: self.child_types = config.obj.ModuleListing.child_types
      except AttributeError: pass
      # Create the file
      self.start_file()
      self.write(self.manager.formatHeader(self.filename(), 2))
      self.tree.startTree()
      self.indexModule(start, start.name())
      self.tree.endTree()
      self.end_file()

   def _child_filter(self, child):
      """Returns true if the given child declaration is to be included"""

      if not isinstance(child, AST.Module): return 0
      if self.child_types and child.type() not in self.child_types:
         return 0
      return 1

   def _link_href(self, ns):
      """Returns the link to the given declaration"""

      return rel(self.filename(), config.files.nameOfModuleIndex(ns.name()))

   def _get_children(self, decl):
      """Returns the children of the given declaration"""

      try: return self._children_cache[decl]
      except KeyError: pass
      config.sorter.set_scope(decl)
      config.sorter.sort_sections()
      children = config.sorter.children()
      children = filter(self._child_filter, children)
      self._children_cache[decl] = children
      return children

   def indexModule(self, ns, rel_scope):
      "Write a link for this module and recursively visit child modules."

      my_scope = ns.name()
      # Find children, and sort so that compound children (packages) go first
      children = self._get_children(ns)
      children.sort(lambda a,b,g=self._get_children:
                    cmp(len(g(b)),len(g(a))))
      # Print link to this module
      name = Util.ccolonName(my_scope, rel_scope) or "Global&nbsp;Namespace"
      link = self._link_href(ns)
      text = href(link, name, target=self._link_target)
      if not len(children):
         self.tree.writeLeaf(text)
      else:
         self.tree.writeNodeStart(text)
         # Add children
         for child in children:
            self.indexModule(child, my_scope)
	    self.tree.writeNodeEnd()

