# $Id: FileTreeJS.py,v 1.1 2001/02/06 05:12:46 chalky Exp $
#
# This file is a part of Synopsis.
# Copyright (C) 2000, 2001 Stephen Davies
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
# $Log: FileTreeJS.py,v $
# Revision 1.1  2001/02/06 05:12:46  chalky
# Added JSTree class and FileTreeJS and modified ModuleListingJS to use JSTree
#
# Revision 1.3  2001/02/05 05:26:24  chalky
# Graphs are separated. Misc changes
#
# Revision 1.2  2001/02/01 15:23:24  chalky
# Copywritten brown paper bag edition.
#
#

# System modules
import os

# Synopsis modules
from Synopsis.Core import AST, Util

# HTML modules
import JSTree
import core
from core import config
from Tags import *

class FileTree(JSTree.JSTree):
    def __init__(self, manager):
	JSTree.JSTree.__init__(self, manager)
	self.__filename = config.files.nameOfSpecial('file_tree')
	link = href(self.__filename, 'File Tree', target="contents")
	self.manager.addRootPage('File Tree', link, 2)
   
    def process(self, start):
	# Init tree
	share = os.path.split(AST.__file__)[0]+"/../share" #hack..
	self.js_init(share+'/syn-down.png', share+'/syn-right.png', 'tree_%s.png', 0)
	# Start the file
	self.startFile(self.__filename, "File Tree")
	self.write(self.manager.formatRoots('File Tree', 2)+'<hr>')
	# recursively visit all nodes
	self.processFileTreeNode(config.fileTree.root())
	self.endFile()
	# recursively create all node pages
	self.processFileTreeNodePage(config.fileTree.root())

    def _node_sorter(self, a, b):
	a_leaf = hasattr(a, 'decls')
	b_leaf = hasattr(b, 'decls')
	if a_leaf != b_leaf:
	    return cmp(b_leaf, a_leaf)
	return cmp(string.upper(a.path[-1]), string.upper(b.path[-1]))

    def processFileTreeNode(self, node):
	if hasattr(node, 'decls'):
	    self.write(href(config.files.nameOfFile(node.path),node.path[-1],target='index'))
	    return
	children = node.children.values()
	children.sort(self._node_sorter)
	if len(node.path):
	    self.write(self.formatButton() + node.path[-1]+os.sep)
	    self.startSection()
	if len(children):
	    for child in children:
		self.write('<div class="files">')
		self.processFileTreeNode(child)
		self.write('</div>')
	if len(node.path):
	    self.endSection()
	
    def processFileTreeNodePage(self, node):
	for child in node.children.values():
	    self.processFileTreeNodePage(child)
	if not hasattr(node, 'decls'): return

	toc = config.toc
	fname = config.files.nameOfFile(node.path)
	name = list(node.path)
	while len(name) and name[0] == '..': del name[0]
	self.startFile(fname, string.join(name, os.sep))
	self.write(entity('b', string.join(name, os.sep))+'<br>')
	for name, decl in node.decls.items():
	    # TODO make this nicer :)
	    entry = config.toc[name]
	    if not entry: print "no entry for",name
	    else:
		# Print link to declaration's page
		if isinstance(decl, AST.Function):
		    self.write(div('href',href(entry.link,anglebrackets(Util.ccolonName(decl.realname())),target='main')))
		else:
		    self.write(div('href',href(entry.link,Util.ccolonName(name),target='main')))
		# Print comment
		#self.write(self.summarizer.getSummary(node.decls[name]))
	
htmlPageClass = FileTree
