# $Id: InheritanceGraph.py,v 1.1 2001/02/01 20:09:18 chalky Exp $
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
# $Log: InheritanceGraph.py,v $
# Revision 1.1  2001/02/01 20:09:18  chalky
# Initial commit.
#
# Revision 1.3  2001/02/01 18:36:55  chalky
# Moved TOC out to Formatter/TOC.py
#
# Revision 1.2  2001/02/01 15:23:24  chalky
# Copywritten brown paper bag edition.
#
#

import os

from Synopsis.Core import Util

import core, Page
from core import config
from Tags import *

class InheritanceGraph(Page.Page):
    def __init__(self, manager):
	Page.Page.__init__(self, manager)
	self.__filename = config.files.nameOfSpecial('classgraph')
	link = href(self.__filename, 'Inheritance Graph', target='main')
	manager.addRootPage('Inheritance Graph', link, 1)
 
    def process(self, start):
	"""Creates a file with the inheritance graph"""
	self.startFile(self.__filename, "Synopsis - Class Hierarchy")
	self.write(entity('h1', "Inheritance Tree"))

	from Synopsis.Formatter import Dot
	output = self.__filename+"-dot"
	config.toc.store(output+".toc")
	args = ('-i','-f','html','-o',output,'-r',output+'.toc','-t','Synopsis')
	Dot.format(config.types, [start], args)
	dot_file = open(output+'.html', 'r')
	self.write(dot_file.read())
	dot_file.close()
	os.remove(output + ".html")
	os.remove(output + ".toc")

	self.endFile()   


    def old(self):
	from Synopsis.Formatter import Dot
	output = self.__filename
	if output[-5:] == ".html": output = output[:-5]
	input = output+".html.dot"
	config.toc.store(output+".toc")
	args = ('-i','-f','dot','-o',input,'-r',output+'.toc','-t','Synopsis')
	Dot.format(config.types, [start], args)
	Dot._format_png(input, output + ".png")
	Dot._format(input, output + ".map", "imap")
	label = 'ClassGraph'
	self.write("<img src=\"" + output + ".png\" border=\"0\" usemap=\"#")
	self.write(label + "_map\">\n")
	self.write("<map name=\"" + label + "_map\">\n")
	dotmap = open(output + ".map", "r+")
	Dot._convert_map(dotmap, self.os())
	dotmap.close()
	os.remove(output + ".map")
	self.write("</map>\n")

	self.endFile()   

htmlPageClass = InheritanceGraph
