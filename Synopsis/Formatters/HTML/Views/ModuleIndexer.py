# $Id: ModuleIndexer.py,v 1.2 2001/02/01 15:23:24 chalky Exp $
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
# $Log: ModuleIndexer.py,v $
# Revision 1.2  2001/02/01 15:23:24  chalky
# Copywritten brown paper bag edition.
#
#

# Synopsis Modules
from Synopsis.Core import AST, Util

# HTML Modules
import Page
from core import config
from Tags import *

class ModuleIndexer(Page.Page):
    """A module for indexing AST.Modules. Each module gets its own page with a
    list of nested scope declarations with comments. It is intended to go in
    the left frame..."""
    def __init__(self, manager):
	Page.Page.__init__(self, manager)
    def process(self, start):
	"""Creates indexes for all modules"""
	start_file = config.files.nameOfModuleIndex(start.name())
	config.set_index_page(start_file)
	self.__namespaces = [start]
	while self.__namespaces:
	    ns = self.__namespaces.pop(0)
	    self.processNamespaceIndex(ns)

    def processNamespaceIndex(self, ns):
	"Index one module"
	config.sorter.set_scope(ns)
	config.sorter.sort_section_names()
	config.sorter.sort_sections()

	# Create file
	name = Util.ccolonName(ns.name()) or "Global Namespace"
	fname = config.files.nameOfModuleIndex(ns.name())
	self.startFile(fname, name+" Index")
	link = href(config.files.nameOfScope(ns.name()), name, target='main')
	self.write(entity('b', link+" Index"))

	# Make script to switch main frame upon load
	link_script = "javascript:return go('%s','%s');"
	load_script = '<!--\nwindow.parent.frames["main"].location="%s";\n'\
	    'function go(index,main) {\n'\
	    'window.parent.frames["index"].location=index;\n'\
	    'window.parent.frames["main"].location=main;\n'\
	    'return false;}\n-->'
	load_script = load_script%config.files.nameOfScope(ns.name())
	self.write(entity('script', load_script, language='Javascript'))

	# Loop throught all the types of children
	for section in config.sorter.sections():
	    if section[-1] == 's': heading = section+'es'
	    else: heading = section+'s'
	    heading = '<br>'+entity('i', heading)+'<br>'
	    # Get a list of children of this type
	    for child in config.sorter.children(section):
		# Print out summary for the child
		if not isinstance(child, AST.Scope):
		    continue
		if heading:
		    self.write(heading)
		    heading = None
		if isinstance(child, AST.Module):
		    script = link_script%(config.files.nameOfModuleIndex(child.name()),config.files.nameOfScope(child.name()))
		    self.write(config.toc.referenceName(child.name(), ns.name(), target='main', onClick=script))
		else:
		    self.write(config.toc.referenceName(child.name(), ns.name(), target='main'))
		self.write('<br>')
	self.endFile()

	# Queue child namespaces
	for child in config.sorter.children():
	    if isinstance(child, AST.Scope):
		self.__namespaces.append(child)
 
htmlPageClass = ModuleIndexer
