# $Id: ModuleIndexer.py,v 1.13 2002/11/02 06:37:37 chalky Exp $
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
# Revision 1.13  2002/11/02 06:37:37  chalky
# Allow non-frames output, some refactoring of page layout, new modules.
#
# Revision 1.12  2002/11/01 07:21:15  chalky
# More HTML formatting fixes eg: ampersands and stuff
#
# Revision 1.11  2002/11/01 03:39:21  chalky
# Cleaning up HTML after using 'htmltidy'
#
# Revision 1.10  2002/10/29 12:43:56  chalky
# Added flexible TOC support to link to things other than ScopePages
#
# Revision 1.9  2002/07/04 06:43:18  chalky
# Improved support for absolute references - pages known their full path.
#
# Revision 1.8  2002/03/14 00:19:47  chalky
# Added demo of template specializations, and fixed HTML formatter to deal with
# angle brackets in class names :)
#
# Revision 1.7  2001/07/10 02:55:51  chalky
# Better comments in some files, and more links work with the nested layout
#
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
# Revision 1.3  2001/02/01 18:36:55  chalky
# Moved TOC out to Formatter/TOC.py
#
# Revision 1.2  2001/02/01 15:23:24  chalky
# Copywritten brown paper bag edition.
#
#

import os
from Synopsis.Core import AST, Util

import core, Page
from core import config
from Tags import *

class ModuleIndexer(Page.Page):
    """A module for indexing AST.Modules. Each module gets its own page with a
    list of nested scope declarations with comments. It is intended to go in
    the left frame..."""
    def __init__(self, manager):
	Page.Page.__init__(self, manager)

    def filename(self): return self.__filename
    def title(self): return self.__title

    def register(self):
        """Register first page as index page"""
	config.set_using_module_index()
        self.__filename = config.files.nameOfModuleIndex(())
        config.set_index_page(self.__filename)

    def process(self, start):
	"""Creates indexes for all modules"""
	start_file = config.files.nameOfModuleIndex(start.name())
	config.set_index_page(start_file)
	self.__namespaces = [start]
	while self.__namespaces:
	    ns = self.__namespaces.pop(0)
	    self.processNamespaceIndex(ns)
    
    def _makePageHeading(self, ns):
	"""Creates a HTML fragment which becomes the name at the top of the
	index page. This may be overridden, but the default is (now) to make a
	linked fully scoped name, where each scope has a link to the relevant
	index."""
	name = ns.name()
	if not name: return 'Global Index'
	links = []
	for depth in range(0, len(name)):
	    url = config.files.nameOfModuleIndex(name[:depth+1])
	    label = anglebrackets(name[depth])
	    links.append(href(rel(self.__filename, url), label))
	return entity('b', string.join(links, '\n::') + ' Index')

    def processNamespaceIndex(self, ns):
	"Index one module"
	config.sorter.set_scope(ns)
	config.sorter.sort_section_names()
	config.sorter.sort_sections()

        self.__filename = config.files.nameOfModuleIndex(ns.name())
        self.__title = Util.ccolonName(ns.name()) or 'Global Namespace'
        self.__title = self.__title + ' Index'
	# Create file
	self.start_file()
	#target = rel(self.__filename, config.files.nameOfScope(ns.name()))
	#link = href(target, self.__title, target='main')
	self.write(self._makePageHeading(ns))

	# Make script to switch main frame upon load
	load_script = '<!--\n'
	if config.toc[ns.name()]:
	    target = rel(self.__filename, config.toc[ns.name()].link)
	    load_script = load_script + 'window.parent.frames["main"].location="%s";\n'%target
	load_script = load_script + 'function go(index,main) {\n'\
	    'window.parent.frames["index"].location=index;\n'\
	    'window.parent.frames["main"].location=main;\n'\
	    'return false;}\n-->'
	self.write(entity('script', load_script, type='text/javascript'))

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
		label = Util.ccolonName(child.name(), ns.name())
		label = anglebrackets(label)
		label = replace_spaces(label)
		if isinstance(child, AST.Module):
		    index_url = rel(self.__filename, config.files.nameOfModuleIndex(child.name()))
		    self.write(href(index_url, label, target='index'))
		else:
		    entry = config.toc[child.name()]
		    if entry:
			url = rel(self.__filename, entry.link)
			self.write(href(url, label, target='main'))
		    else:
			self.write(label)
		self.write('<br>')
	self.end_file()

	# Queue child namespaces
	for child in config.sorter.children():
	    if isinstance(child, AST.Module):
		self.__namespaces.append(child)
 
htmlPageClass = ModuleIndexer
