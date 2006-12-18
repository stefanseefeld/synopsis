#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis import config
from Synopsis.Processor import Parameter
from Synopsis import AST, Util
from Tree import Tree
from Synopsis.Formatters.HTML.Tags import *

import os

class FileTree(Tree):

   link_to_views = Parameter(False, 'some docs...')

   def register(self, processor):

      Tree.register(self, processor)
      self.__filename = self.processor.file_layout.special('FileTree')
      self.__title = 'File Tree'
      processor.set_main_view(self.filename())
      processor.set_contents_view(self.filename())
   
   def filename(self):

      return self.__filename

   def title(self):

      return self.__title

   def menu_item(self):

      return self.processor.file_layout.special('FileTree'), 'File Tree', 'contents', 'contents'

   def process(self, start):

      # Init tree
      share = config.datadir
      self.js_init(os.path.join(share, 'syn-down.png'),
                   os.path.join(share, 'syn-right.png'),
                   os.path.join(share, 'syn-dot.png'),
                   'tree_%s.png', 0)
      # Start the file
      self.start_file()
      self.write(self.processor.navigation_bar(self.filename(), 'contents'))
      # recursively visit all nodes
      self.process_file_tree_node(self.processor.file_tree.root())
      self.end_file()
      # recursively create all node views
      self.process_file_tree_node_view(self.processor.file_tree.root())

   def _node_sorter(self, a, b):
      a_leaf = hasattr(a, 'decls')
      b_leaf = hasattr(b, 'decls')
      if a_leaf != b_leaf:
         return cmp(b_leaf, a_leaf)
      return cmp(a.path[-1].upper(), b.path[-1].upper())

   def process_file_tree_node(self, node):

      dirname, filename = os.path.split(node.path)
      if hasattr(node, 'decls'):
         # Leaf node
         text = href(self.processor.file_layout.file_index(node.path),
                     filename, target='index')
         self.write_leaf(text)
         return
      # Non-leaf node
      children = node.children[:]
      children.sort(self._node_sorter)
      if node.path:
         self.write_node_start(filename + os.sep)
      if len(children):
         for child in children:
            #self.write('<div class="files">')
            self.process_file_tree_node(child)
            #self.write('</div>')
      if node.path:
         self.write_node_end()
	
   def process_file_tree_node_view(self, node):

      if hasattr(node, 'children'): # a Directory
         for child in node.children[:]:
            self.process_file_tree_node_view(child)
         return

      # set up filename and title for the current view
      self.__filename = self.processor.file_layout.file_index(node.path)
      name = node.path.split(os.sep)
      while len(name) and name[0] == '..': del name[0]
      self.__title = os.sep.join(name)

      self.start_file()
      self.write(entity('b', os.sep.join(name))+'<br/>')
      if self.link_to_views:
         link = self.processor.file_layout.scoped_special('view', name)
         self.write(href(link, '[Source]', target="main")+'<br/>')
      for decl in node.decls:
         # TODO make this nicer :)
         entry = self.processor.toc[decl.name()]
         if not entry: pass #print "no entry for",decl.name()
         else:
            # Print link to declaration's view
            if isinstance(decl, AST.Function):
               self.write(div('href',href(entry.link,escape(Util.ccolonName(decl.realname())),target='main')))
            else:
               self.write(div('href',href(entry.link,Util.ccolonName(decl.name()),target='main')))
               # Print comment
               #self.write(self.summarizer.getSummary(node.decls[name]))
      self.end_file()
