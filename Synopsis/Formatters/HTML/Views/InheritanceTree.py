# $Id: InheritanceTree.py,v 1.11 2003/11/11 06:01:13 stefan Exp $
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
# $Log: InheritanceTree.py,v $
# Revision 1.11  2003/11/11 06:01:13  stefan
# adjust to directory/package layout changes
#
# Revision 1.10  2002/07/04 06:43:18  chalky
# Improved support for absolute references - pages known their full path.
#
# Revision 1.9  2001/07/05 05:39:58  stefan
# advanced a lot in the refactoring of the HTML module.
# Page now is a truely polymorphic (abstract) class. Some derived classes
# implement the 'filename()' method as a constant, some return a variable
# dependent on what the current scope is...
#
# Revision 1.8  2001/07/05 02:08:35  uid20151
# Changed the registration of pages to be part of a two-phase construction
#
# Revision 1.7  2001/06/28 07:22:18  stefan
# more refactoring/cleanup in the HTML formatter
#
# Revision 1.6  2001/06/26 04:32:16  stefan
# A whole slew of changes mostly to fix the HTML formatter's output generation,
# i.e. to make the output more robust towards changes in the layout of files.
#
# the rpm script now works, i.e. it generates source and binary packages.
#
# Revision 1.5  2001/02/06 05:13:05  chalky
# Fixes
#
# Revision 1.4  2001/02/05 05:26:24  chalky
# Graphs are separated. Misc changes
#
# Revision 1.3  2001/02/01 18:36:55  chalky
# Moved TOC out to Formatter/TOC.py
#
# Revision 1.2  2001/02/01 15:23:24  chalky
# Copywritten brown paper bag edition.
#
#

import os
from Synopsis import Util

import core, Page
from core import config
from Tags import *

class InheritanceTree(Page.Page):
    def __init__(self, manager):
	Page.Page.__init__(self, manager)

    def filename(self): return config.files.nameOfSpecial('InheritanceTree')
    def title(self): return 'Synopsis - Class Hierarchy'

    def register(self):
	self.manager.addRootPage(self.filename(), 'Inheritance Tree', 'main', 1)
 
    def process(self, start):
	"""Creates a file with the inheritance tree"""
	roots = config.classTree.roots()
	self.start_file()
	self.write(self.manager.formatHeader(self.filename()))
	self.write(entity('h1', "Inheritance Tree"))
	self.write('<ul>')
	map(self.processClassInheritance, map(lambda a,b=start.name():(a,b), roots))
	self.write('</ul>')
	self.end_file()   

    def processClassInheritance(self, args):
	name, rel_name = args
	self.write('<li>')
	self.write(self.reference(name, rel_name))
	parents = config.classTree.superclasses(name)
	if parents:
	    self.write(' <i>(%s)</i>'%string.join(map(Util.ccolonName, parents), ", "))
	subs = config.classTree.subclasses(name)
	if subs:
	    self.write('<ul>')
	    map(self.processClassInheritance, map(lambda a,b=name:(a,b), subs))
	    self.write('</ul>\n')
	self.write('</li>')

htmlPageClass = InheritanceTree
