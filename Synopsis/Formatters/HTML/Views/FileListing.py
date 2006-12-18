#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Processor import Parameter
from Synopsis import AST, Util
from Synopsis.FileTree import FileTree
from Synopsis.Formatters.HTML.View import View
from Synopsis.Formatters.HTML.Tags import *

import os

class FileListing(View):
   """A view that creates an index of files, and an index for each file.
   First the index of files is created, intended for the top-left frame.
   Second a view is created for each file, listing the major declarations for
   that file, eg: classes, global functions, namespaces, etc."""

   def register(self, processor):

      View.register(self, processor)
      processor.set_main_view(self.filename())
      processor.set_contents_view(self.filename())

   def filename(self):

      return self.processor.file_layout.special('FileListing')

   def title(self):

      return 'Files'

   def menu_item(self):

      return self.filename(), self.title(), 'contents', 'contents'

   def register_filenames(self, start):
      """Registers a view for each file indexed"""

      self.processor.register_filename(self.filename(), self, None)
    
   def process(self, start):
      """Creates the listing using the recursive process_file_tree_node method"""

      # Init tree
      self.tree = self.processor.tree_formatter
      self.tree.register(self)
      # Start the file
      self.start_file()
      self.write(self.processor.navigation_bar(self.filename(), 'contents'))
      self.tree.start_tree()
      # recursively visit all nodes
      self.process_file_tree_node(self.processor.file_tree.root())
      self.tree.end_tree()
      self.end_file()

   def _node_sorter(self, a, b):
      """Compares file nodes a and b depending on whether they are leaves
      or not"""

      a_leaf = isinstance(a, FileTree.File)
      b_leaf = isinstance(a, FileTree.File)
      if a_leaf != b_leaf:
         return cmp(b_leaf, a_leaf)
      return cmp(string.upper(a.path), string.upper(b.path))

   def process_file_tree_node(self, node):
      """Creates a portion of the tree for the given file node. This method
      assumes that the file is already in progress, and just appends to
      it. This method is recursive, calling itself for each child of node
      (file or directory)."""

      if isinstance(node, FileTree.File):
         # Leaf node
         ref = rel(self.filename(), self.processor.file_layout.file_index(node.path))
         text = href(ref, node.filename, target='index')
         self.tree.write_leaf(text)
         return
      # Non-leaf node
      children = node.children
      children.sort(self._node_sorter)
      if len(node.path):
         self.tree.write_node_start(node.filename+os.sep)
      if len(children):
         for child in children:
            self.process_file_tree_node(child)
      if len(node.path):
         self.tree.write_node_end()
