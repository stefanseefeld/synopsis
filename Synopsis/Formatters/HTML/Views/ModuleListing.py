# $Id: ModuleListing.py,v 1.6 2001/06/28 07:22:18 stefan Exp $
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
# $Log: ModuleListing.py,v $
# Revision 1.6  2001/06/28 07:22:18  stefan
# more refactoring/cleanup in the HTML formatter
#
# Revision 1.5  2001/06/26 04:32:16  stefan
# A whole slew of changes mostly to fix the HTML formatter's output generation,
# i.e. to make the output more robust towards changes in the layout of files.
#
# the rpm script now works, i.e. it generates source and binary packages.
#
# Revision 1.4  2001/06/05 10:04:36  chalky
# Can filter modules based on type, eg: 'Package'
#
# Revision 1.3  2001/06/05 05:28:34  chalky
# Some old tree abstraction
#
# Revision 1.6  2001/04/18 04:08:03  chalky
# Sort modules so that packages come first
#
# Revision 1.5  2001/04/17 13:36:10  chalky
# Slight enhancement to JSTree and derivatives, to use a dot graphic for leaves
#
# Revision 1.4  2001/03/29 14:11:33  chalky
# Refactoring for use by RefManual's own derived module
#
# Revision 1.3  2001/02/06 05:12:46  chalky
# Added JSTree class and FileTreeJS and modified ModuleListingJS to use JSTree
#
# Revision 1.2  2001/02/05 07:58:39  chalky
# Cleaned up image copying for *JS. Added synopsis logo to ScopePages.
#
# Revision 1.1  2001/02/05 05:31:52  chalky
# Initial commit. Image finding is a hack
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
import core
import Page
from core import config
from Tags import *


class ModuleListing(Page.Page): 
    """Create an index of all modules with JS. The JS allows the user to
    expand/collapse sections of the tree!"""
    def __init__(self, manager):
	Page.Page.__init__(self, manager)
	self.child_types = None
	self._children_cache = {}
	self._init_page()

    def _init_page(self):
	"Sets _filename and registers the page with the manager"
	filename = config.files.nameOfSpecial('ModuleTree')
	config.set_contents_page(filename)
	self.manager.addRootPage(filename, 'Modules', 'contents', 2)
	self._link_target = 'index'

    def process(self, start):
	"""Create a page with an index of all modules"""
	# Init tree
	self.tree = config.treeFormatterClass(self)
	# Init list of module types to display
	try: self.child_types = config.obj.ModuleListing.child_types
	except AttributeError: pass
	# Create the file
	filename = config.files.nameOfSpecial('ModuleTree')
	self.startFile(filename, 'Module Tree')
	self.write(self.manager.formatHeader(filename, 2))
	self.tree.startTree()
	self.indexModule(start, start.name())
	self.tree.endTree()
	self.endFile()

    def _child_filter(self, child):
	if not isinstance(child, AST.Module): return 0
	if self.child_types and child.type() not in self.child_types:
	    return 0
	return 1
    def _link_href(self, ns):
	return config.files.nameOfModuleIndex(ns.name())
    def _get_children(self, decl):
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
	name = Util.ccolonName(my_scope, rel_scope) or "Global Namespace"
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

htmlPageClass = ModuleListing

