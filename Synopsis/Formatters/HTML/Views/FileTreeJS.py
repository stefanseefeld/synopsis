# $Id: FileTreeJS.py,v 1.6 2001/07/05 05:39:58 stefan Exp $
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
# Revision 1.6  2001/07/05 05:39:58  stefan
# advanced a lot in the refactoring of the HTML module.
# Page now is a truely polymorphic (abstract) class. Some derived classes
# implement the 'filename()' method as a constant, some return a variable
# dependent on what the current scope is...
#
# Revision 1.5  2001/06/28 07:22:18  stefan
# more refactoring/cleanup in the HTML formatter
#
# Revision 1.4  2001/06/26 04:32:16  stefan
# A whole slew of changes mostly to fix the HTML formatter's output generation,
# i.e. to make the output more robust towards changes in the layout of files.
#
# the rpm script now works, i.e. it generates source and binary packages.
#
# Revision 1.3  2001/04/17 13:36:10  chalky
# Slight enhancement to JSTree and derivatives, to use a dot graphic for leaves
#
# Revision 1.2  2001/02/16 02:29:55  chalky
# Initial work on SXR and HTML integration
#
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
	filename = config.files.nameOfSpecial('FileTree')
	self.manager.addRootPage(filename, 'File Tree', 'contents', 2)
	myconfig = config.obj.FileTree
	self._link_pages = myconfig.link_to_pages
   
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
	filename = config.files.nameOfSpecial('FileTree')
	self.start_file(filename, 'File Tree')
	self.write(self.manager.formatHeader(filename, 2))
	# recursively visit all nodes
	self.processFileTreeNode(config.fileTree.root())
	self.end_file()
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
	    # Leaf node
	    text = href(config.files.nameOfFile(node.path), node.path[-1], target='index')
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

	toc = config.toc
        # set up filename and title for the current page
	self.__filename = config.files.nameOfFile(node.path)
	name = list(node.path)
	while len(name) and name[0] == '..': del name[0]
        self.__title = string.join(name, os.sep)

	self.start_file()
	self.write(entity('b', string.join(name, os.sep))+'<br>')
	if self._link_pages:
	    link = config.files.nameOfScopedSpecial('page', name)
	    self.write(href(link, '[Source]', target="main")+'<br>')
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
	self.end_file()
	
htmlPageClass = FileTree
