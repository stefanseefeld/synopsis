# $Id: FileTreeJS.py,v 1.11 2003/11/15 19:01:53 stefan Exp $
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
from JSTree import JSTree
from Synopsis.Formatters.HTML.Tags import *

import os

class FileTree(JSTree):

   link_to_pages = Parameter(False, 'some docs...')

   def register(self, processor):

      JSTree.register(self, processor)
      filename = self.processor.file_layout.nameOfSpecial('FileTree')
      self.processor.addRootPage(filename, 'File Tree', 'contents', 2)
   
   def filename(self):
      """since FileTree generates a whole file hierarchy, this method returns the current filename,
      which may change over the lifetime of this object"""

      return self.__filename

   def title(self):
      """since FileTree generates a while file hierarchy, this method returns the current title,
      which may change over the lifetime of this object"""

      return self.__title

   def process(self, start):

      # Init tree
      share = config.datadir
      self.js_init(os.path.join(share, 'syn-down.png'),
                   os.path.join(share, 'syn-right.png'),
                   os.path.join(share, 'syn-dot.png'),
                   'tree_%s.png', 0)
      # Start the file
      filename = self.processor.file_layout.nameOfSpecial('FileTree')
      self.start_file(filename, 'File Tree')
      self.write(self.processor.formatHeader(filename, 2))
      # recursively visit all nodes
      self.processFileTreeNode(self.processor.fileTree.root())
      self.end_file()
      # recursively create all node pages
      self.processFileTreeNodePage(self.processor.fileTree.root())

   def _node_sorter(self, a, b):
      a_leaf = hasattr(a, 'decls')
      b_leaf = hasattr(b, 'decls')
      if a_leaf != b_leaf:
         return cmp(b_leaf, a_leaf)
      return cmp(string.upper(a.path[-1]), string.upper(b.path[-1]))

   def processFileTreeNode(self, node):

      if hasattr(node, 'decls'):
         # Leaf node
         text = href(self.processor.file_layout.nameOfFileIndex(string.join(node.path,
                                                                            os.sep)),
                     node.path[-1], target='index')
         self.writeLeaf(text)
         return
      # Non-leaf node
      children = node.children.values()
      children.sort(self._node_sorter)
      if len(node.path):
         self.writeNodeStart(node.path[-1]+os.sep)
      if len(children):
         for child in children:
            #self.write('<div class="files">')
            self.processFileTreeNode(child)
            #self.write('</div>')
      if len(node.path):
         self.writeNodeEnd()
	
   def processFileTreeNodePage(self, node):

      for child in node.children.values():
         self.processFileTreeNodePage(child)
      if not hasattr(node, 'decls'): return

      # set up filename and title for the current page
      self.__filename = self.processor.file_layout.nameOfFileIndex(string.join(node.path,
                                                                               os.sep))
      name = list(node.path)
      while len(name) and name[0] == '..': del name[0]
      self.__title = string.join(name, os.sep)

      self.start_file()
      self.write(entity('b', string.join(name, os.sep))+'<br>')
      if self.link_to_pages:
         link = self.processor.file_layout.nameOfScopedSpecial('page', name)
         self.write(href(link, '[Source]', target="main")+'<br>')
      for name, decl in node.decls.items():
         # TODO make this nicer :)
         entry = self.processor.toc[name]
         if not entry: print "no entry for",name
         else:
            # Print link to declaration's page
            if isinstance(decl, AST.Function):
               self.write(div('href',href(entry.link,anglebrackets(Util.ccolonName(decl.realname())),target='main')))
            else:
               self.write(div('href',href(entry.link,Util.ccolonName(name),target='main')))
               # Print comment
               #self.write(self.summarizer.getSummary(node.decls[name]))
      self.end_file()
