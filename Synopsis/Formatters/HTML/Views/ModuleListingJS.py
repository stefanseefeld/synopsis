# $Id: ModuleListingJS.py,v 1.4 2001/03/29 14:11:33 chalky Exp $
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
import JSTree
from core import config
from Tags import *


class ModuleListingJS(JSTree.JSTree): 
    """Create an index of all modules with JS. The JS allows the user to
    expand/collapse sections of the tree!"""
    def __init__(self, manager):
	JSTree.JSTree.__init__(self, manager)
	self._init_page()

    def _init_page(self):
	"Sets _filename and registers the page with the manager"
	self._filename = config.files.nameOfSpecial('module_listing')
	link = href(self._filename, 'Modules', target="contents")
	self.manager.addRootPage('Modules', link, 2)
	config.set_contents_page(self._filename)
	self._link_target = 'index'

    def process(self, start):
	"""Create a page with an index of all modules"""
	# Init tree
	share = os.path.split(AST.__file__)[0]+"/../share" #hack..
	self.js_init(share+'/syn-down.png', share+'/syn-right.png', 'tree_%s.png', 0)
	# Creare the file
	self.startFile(self._filename, "Module Index")
	self.write(self.manager.formatRoots('Modules', 2))
	self.write('<hr>')
	self.indexModule(start, start.name())
	self.endFile()

    def _child_filter(self, child):
	return isinstance(child, AST.Module)
    def _link_href(self, ns):
	return config.files.nameOfModuleIndex(ns.name())
    def indexModule(self, ns, rel_scope):
	"Write a link for this module and recursively visit child modules."
	my_scope = ns.name()
	# Find children
	config.sorter.set_scope(ns)
	config.sorter.sort_sections()
	children = config.sorter.children()
	children = filter(self._child_filter, children)
	# Print link to this module
	button = len(children) and self.formatButton() or ""
	name = Util.ccolonName(my_scope, rel_scope) or "Global Namespace"
	link = self._link_href(ns)
	self.write(button + href(link, name, target=self._link_target) + '<br>')
	# Add children
	if not len(children): return
	self.startSection()
	for child in children:
	    self.write('<div class="module-section">')
	    self.indexModule(child, my_scope)
	    self.write('</div>')
	self.endSection()

htmlPageClass = ModuleListingJS

