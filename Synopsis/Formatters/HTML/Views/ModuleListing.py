# $Id: ModuleListing.py,v 1.2 2001/02/01 15:23:24 chalky Exp $
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
# Revision 1.2  2001/02/01 15:23:24  chalky
# Copywritten brown paper bag edition.
#
#

# Synopsis modules
from Synopsis.Core import AST, Util

# HTML modules
import Page
import core
from core import config
from Tags import *

class ModuleListing(Page.Page): 
    """Create an index of all modules."""
    def __init__(self, manager):
	Page.Page.__init__(self, manager)
	self.__filename = config.files.nameOfSpecial('module_listing')
	link = href(self.__filename, 'Modules', target="contents")
	manager.addRootPage('Modules', link, 2)
	config.set_contents_page(self.__filename)

    def process(self, start):
	"""Create a page with an index of all modules"""
	self.startFile(self.__filename, "Module Index")
	self.write(self.manager.formatRoots('Modules', 2))
	self.write('<hr>')
	self.indexModule(start, start.name())
	self.endFile()

    def indexModule(self, ns, rel_scope):
	"Write a link for this module and recursively visit child modules."
	my_scope = ns.name()
	# Print link to this module
	name = Util.ccolonName(my_scope, rel_scope) or "Global Namespace"
	link = config.files.nameOfModuleIndex(ns.name())
	self.write(href(link, name, target='index') + '<br>')
	# Add children
	config.sorter.set_scope(ns)
	config.sorter.sort_sections()
	for child in config.sorter.children():
	    if isinstance(child, AST.Module):
		self.write('<div class="module-section">')
		self.indexModule(child, my_scope)
		self.write('</div>')

htmlPageClass = ModuleListing
