# $Id: FileListing.py,v 1.6 2003/11/16 21:09:45 stefan Exp $
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
from Synopsis.Formatters.HTML.Page import Page
from Synopsis.Formatters.HTML.Tags import *

import os

class FileListing(Page):
   """A page that creates an index of files, and an index for each file.
   First the index of files is created, intended for the top-left frame.
   Second a page is created for each file, listing the major declarations for
   that file, eg: classes, global functions, namespaces, etc."""

   def register(self, processor):

      Page.register(self, processor)
      self.__filename = self.processor.file_layout.special('FileListing')
      self.__title = 'Files'

      processor.set_main_page(self.filename())
      # Reset filename in case we got main page status
      self.__filename = self.processor.file_layout.special('FileListing')
      self.processor.add_root_page(self.filename(), self.title(), "contents", 2)
      processor.set_contents_page(self.filename())

   def filename(self):
      """Returns the filename"""

      return self.__filename

   def title(self):
      """Returns the title"""

      return self.__title

   def register_filenames(self, start):
      """Registers a page for each file indexed"""

      self.processor.register_filename(self.__filename, self, None)
    
   def process(self, start):
      """Creates the listing using the recursive process_file_tree_node method"""

      # Init tree
      self.tree = self.processor.tree_formatter
      self.tree.register(self)
      # Start the file
      self.start_file()
      self.write(self.processor.navigation_bar(self.filename(), 2))
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
