# $Id: TreeFormatter.py,v 1.4 2003/11/14 14:51:09 stefan Exp $
#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

"""Tree formatter interface.

This module contains the class which defines the interface for all tree views.
A tree is a structure of leaves and nodes, where each leave has one node, each
node can have many child leaves or nodes, and there is one root node. Trees
are used to describe the relationship between modules, and also between files.
The user can select between different ways of representing trees, for example
as simple nested lists or as complex javascript trees that the user can expand
and collapse individual branches of. This module contains the tree interface
which is common to all implementations."""

from Synopsis.Processor import Parametrized, Parameter

class TreeFormatter(Parametrized):
   """Interface for all tree implementations. This tree class provides
   default implementations of its methods for example and basic usage. The
   implementation provided outputs a nested tree using the UL and LI html
   elements."""

   def register(self, page):
      """A tree is a strategy, so it must be passed the page instance to
      display to."""

      self.page = page
      self.write = page.write
      self.os = page.os()

   def start_tree(self):
      """Writes anything to the file that needs to be written at the start.
      For example a script section for the global scripts used by a
      javascript tree."""

      self.write('<ul class="tree">')

   def end_tree(self):
      """Writes anything that needs to be written at the end."""

      self.write('</ul>')
    
   def write_leaf(self, text):
      """Writes a leaf to the output. A leaf is a node with no children, for
      example a module (not package) or a file (not directory). The text is
      output verbatim in the appropriate html tags, which in the default
      instance is LI"""

      self.write('<li>%s</li>'%text)

   def write_node_start(self, text):
      """Starts a node with children. The text is written, and then a block
      of children is started. This method call must be followed by a
      corresponding writeNodeEnd() call. Invididual leaves inside the block
      may be written out using the writeLeaf() method."""

      self.write('<li>%s<ul class="tree">'%text)

   def write_node_end(self):
      """Ends a node with children. This method just closes any tags opened
      by the corresponding writeNodeStart() method for a node."""

      self.write('</ul></li>')
