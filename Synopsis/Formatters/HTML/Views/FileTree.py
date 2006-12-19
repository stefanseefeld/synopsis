#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Processor import Parameter
from Synopsis import AST, Util
from Tree import Tree
from Synopsis.Formatters.HTML.Tags import *

import os

class FileTree(Tree):

   link_to_views = Parameter(False, 'some docs...')

   def register(self, frame):

      super(FileTree, self).register(frame)
      self.__filename = self.directory_layout.special('FileTree')
      self.__title = 'File Tree'
   
   def filename(self):

      return self.__filename

   def title(self):

      return self.__title

   def root(self):

      return self.directory_layout.special('FileTree'), 'File Tree'

   def process(self):

      # Start the file
      self.start_file()
      self.write_navigation_bar()
      # recursively visit all nodes
      self.process_file_tree_node(self.processor.file_tree.root())
      self.end_tree()
      self.end_file()

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
         text = href(self.directory_layout.file_index(node.path),
                     filename, target='detail')
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
