# $Id: InheritanceTree.py,v 1.2 2001/02/01 15:23:24 chalky Exp $
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
# Revision 1.2  2001/02/01 15:23:24  chalky
# Copywritten brown paper bag edition.
#
#

from Synopsis.Core import Util

import Page
from core import config
from Tags import *

class InheritanceTree(Page.Page):
    def __init__(self, manager):
	Page.Page.__init__(self, manager)
	self.__filename = config.files.nameOfSpecial('tree')
	link = href(self.__filename, 'Inheritance Tree', target='main')
	manager.addRootPage('Inheritance Tree', link, 1)
 
    def process(self, start):
	"""Creates a file with the inheritance tree"""
	classes = config.classTree.classes()
	def root(name): return not config.classTree.superclasses(name)
	roots = filter(root, classes)

	self.startFile(self.__filename, "Synopsis - Class Hierarchy")
	self.write(entity('h1', "Inheritance Tree"))
	self.write('<ul>')
	map(self.processClassInheritance, map(lambda a,b=start.name():(a,b), roots))
	self.write('</ul>')
	self.endFile()   

    def processClassInheritance(self, args):
	name, rel_name = args
	self.write('<li>')
	self.write(config.toc.referenceName(name, rel_name))
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
