# $Id: FileListing.py,v 1.2 2003/11/11 06:01:13 stefan Exp $
#
# This file is a part of Synopsis.
# Copyright (C) 2000-2003 Stephen Davies
# Copyright (C) 2000, 2001 Stefan Seefeld
#
# Synopsis is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.
#
# $Log: FileListing.py,v $
# Revision 1.2  2003/11/11 06:01:13  stefan
# adjust to directory/package layout changes
#
# Revision 1.1  2003/01/16 12:46:46  chalky
# Renamed FilePages to FileSource, FileTree to FileListing. Added FileIndexer
# (used to be part of FileTree) and FileDetails.
#
# Revision 1.18  2003/01/02 07:00:58  chalky
# Only use Core.FileTree, refactored FileTree to be quicker and better.
#
# Revision 1.17  2002/12/12 17:25:33  chalky
# Implemented Include support for C++ parser. A few other minor fixes.
#
# Revision 1.16  2002/11/02 06:37:37  chalky
# Allow non-frames output, some refactoring of page layout, new modules.
#
# Revision 1.15  2002/11/01 07:21:15  chalky
# More HTML formatting fixes eg: ampersands and stuff
#
# Revision 1.14  2002/10/29 12:43:56  chalky
# Added flexible TOC support to link to things other than ScopePages
#

# System modules
import os

# Synopsis modules
from Synopsis import AST, Util
from Synopsis.FileTree import FileTree

# HTML modules
import Page
import core
from core import config
from Tags import *

class FileListing (Page.Page):
    """A page that creates an index of files, and an index for each file.
    First the index of files is created, intended for the top-left frame.
    Second a page is created for each file, listing the major declarations for
    that file, eg: classes, global functions, namespaces, etc."""
    def __init__(self, manager):
	Page.Page.__init__(self, manager)
        self.__filename = config.files.nameOfSpecial('FileListing')
        self.__title = 'Files'

    def filename(self):
        """Returns the filename"""
        return self.__filename
    def title(self):
        """Returns the title"""
        return self.__title

    def register(self):
	"""Registers this page for the top-left frame"""
	config.set_main_page(self.filename())
	# Reset filename in case we got main page status
        self.__filename = config.files.nameOfSpecial('FileListing')
	self.manager.addRootPage(self.filename(), self.title(), "contents", 2)
	config.set_contents_page(self.filename())
   
    def register_filenames(self, start):
        """Registers a page for each file indexed"""
	self.manager.register_filename(self.__filename, self, None)
    
    def process(self, start):
	"""Creates the listing using the recursive processFileTreeNode method"""
	# Init tree
	self.tree = config.treeFormatterClass(self)
	# Start the file
	self.start_file()
	self.write(self.manager.formatHeader(self.filename(), 2))
	self.tree.startTree()
	# recursively visit all nodes
	self.processFileTreeNode(config.fileTree.root())
	self.tree.endTree()
	self.end_file()

    def _node_sorter(self, a, b):
	"""Compares file nodes a and b depending on whether they are leaves
	or not"""
	a_leaf = isinstance(a, FileTree.File)
	b_leaf = isinstance(a, FileTree.File)
	if a_leaf != b_leaf:
	    return cmp(b_leaf, a_leaf)
	return cmp(string.upper(a.path), string.upper(b.path))

    def processFileTreeNode(self, node):
	"""Creates a portion of the tree for the given file node. This method
	assumes that the file is already in progress, and just appends to
	it. This method is recursive, calling itself for each child of node
	(file or directory)."""
	if isinstance(node, FileTree.File):
	    # Leaf node
	    ref = rel(self.filename(), config.files.nameOfFileIndex(node.path))
	    text = href(ref, node.filename, target='index')
	    self.tree.writeLeaf(text)
	    return
	# Non-leaf node
	children = node.children
	children.sort(self._node_sorter)
	if len(node.path):
	    self.tree.writeNodeStart(node.filename+os.sep)
	if len(children):
	    for child in children:
		self.processFileTreeNode(child)
	if len(node.path):
	    self.tree.writeNodeEnd()
 	
htmlPageClass = FileListing
