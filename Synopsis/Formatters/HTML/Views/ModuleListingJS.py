# $Id: ModuleListingJS.py,v 1.10 2003/11/11 06:01:13 stefan Exp $
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
# $Log: ModuleListingJS.py,v $
# Revision 1.10  2003/11/11 06:01:13  stefan
# adjust to directory/package layout changes
#
# Revision 1.9  2001/07/05 05:39:58  stefan
# advanced a lot in the refactoring of the HTML module.
# Page now is a truely polymorphic (abstract) class. Some derived classes
# implement the 'filename()' method as a constant, some return a variable
# dependent on what the current scope is...
#
# Revision 1.8  2001/06/28 07:22:18  stefan
# more refactoring/cleanup in the HTML formatter
#
# Revision 1.7  2001/06/26 04:32:16  stefan
# A whole slew of changes mostly to fix the HTML formatter's output generation,
# i.e. to make the output more robust towards changes in the layout of files.
#
# the rpm script now works, i.e. it generates source and binary packages.
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
from Synopsis import AST, Util

# HTML modules
import core
import JSTree
from core import config
from Tags import *


class ModuleListingJS(JSTree.JSTree): 
    """Create an index of all modules with JS. The JS allows the user to
    expand/collapse sections of the tree!"""
    def __init__(self, manager):
	JSTree.JSTree.__init__(self, manager)
	self._children_cache = {}
	self._init_page()

    def _init_page(self):
	"Sets _filename and registers the page with the manager"
	filename = config.files.nameOfSpecial('ModuleTree')
	config.set_contents_page(filename)
	self.manager.addRootPage(filename, 'Modules', 'contents', 2)
	self._link_target = 'index'

    def filename(self): return config.files.nameOfSpecial('ModuleTree')
    def title(self): return 'Module Tree'

    def process(self, start):
	"""Create a page with an index of all modules"""
	# Init tree
	share = config.datadir
	self.js_init(os.path.join(share, 'syn-down.png'),
                     os.path.join(share, 'syn-right.png'),
		     os.path.join(share, 'syn-dot.png'),
                     'tree_%s.png', 0)
	self.__share = share
	# Creare the file
	self.start_file()
	self.write(self.manager.formatHeader(filename, 2))
	self.indexModule(start, start.name())
	self.end_file()

    def _child_filter(self, child):
	return isinstance(child, AST.Module)
    def _link_href(self, ns):
	return config.files.nameOfModuleIndex(ns.name())
    def get_children(self, decl):
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
	children = self.get_children(ns)
	children.sort(lambda a,b,g=self.get_children:
	    cmp(len(g(b)),len(g(a))))
	# Print link to this module
	name = Util.ccolonName(my_scope, rel_scope) or "Global Namespace"
	link = self._link_href(ns)
	text = href(link, name, target=self._link_target)
	if not len(children):
	    self.writeLeaf(text)
	else:
	    self.writeNodeStart(text)
	    # Add children
	    for child in children:
		self.indexModule(child, my_scope)
	    self.writeNodeEnd()

htmlPageClass = ModuleListingJS

