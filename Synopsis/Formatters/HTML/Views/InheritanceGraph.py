# $Id: InheritanceGraph.py,v 1.8 2001/04/06 06:26:39 chalky Exp $
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
# Revision 1.8  2001/04/06 06:26:39  chalky
# No more warning on bad types
#
# Revision 1.7  2001/03/29 14:12:42  chalky
# Consolidate small graphs to speed up processing time (graphviz is slow)
#
# Revision 1.6  2001/02/13 06:55:23  chalky
# Made synopsis -l work again
#
# Revision 1.5  2001/02/06 16:54:15  chalky
# Added -n to Dot which stops those nested classes
#
# Revision 1.4  2001/02/06 16:02:23  chalky
# Fixes
#
# Revision 1.3  2001/02/06 05:13:05  chalky
# Fixes
#
# Revision 1.2  2001/02/05 05:26:24  chalky
# Graphs are separated. Misc changes
#
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

from Synopsis.Core import AST, Type, Util

import core, Page
from core import config
from Tags import *

class ToDecl (Type.Visitor):
    def __call__(self, name):
	try:
	    typeobj = config.types[name]
	except KeyError:
	    if config.verbose: print "Warning: %s not found in types dict."%(name,)
	    return None
	self.__decl = None
	typeobj.accept(self)
	#if self.__decl is None:
	#    return None
	return self.__decl
	    
    def visitBaseType(self, type): return
    def visitUnknown(self, type): return
    def visitDeclared(self, type): self.__decl = type.declaration()
    def visitModifier(self, type): type.alias().accept(self)
    def visitArray(self, type): type.alias().accept(self)
    def visitTemplate(self, type): self.__decl = type.declaration()
    def visitParametrized(self, type): type.template().accept(self)
    def visitFunctionType(self, type): return
	

class InheritanceGraph(Page.Page):
    def __init__(self, manager):
	Page.Page.__init__(self, manager)
	self.__filename = config.files.nameOfSpecial('classgraph')
	link = href(self.__filename, 'Inheritance Graph', target='main')
	manager.addRootPage('Inheritance Graph', link, 1)
	self.__todecl = ToDecl()
 
    def consolidate(self, graphs):
	"""Consolidates small graphs into larger ones"""
	try:
	    minsize = config.obj.InheritanceGraph.min_size
	except:
	    if config.verbose:
		print "Error getting InheritanceGraph.min_size value. Using 5."
	    minsize = 5
	conned = []
	pending = []
	while len(graphs):
	    graph = graphs.pop(0)
	    len_graph = len(graph)
	    # Try adding to an already pending graph
	    for pend in pending:
		if len_graph + len(pend) <= minsize:
		    pend.extend(graph)
		    graph = None
		    if len(pend) == minsize:
			conned.append(pend)
			pending.remove(pend)
		    break
	    if graph:
		# Add to pending list
		pending.append(graph)
	conned.extend(pending)
	return conned

    def process(self, start):
	"""Creates a file with the inheritance graph"""
	self.startFile(self.__filename, "Synopsis - Class Hierarchy")
	self.write(self.manager.formatRoots('Inheritance Graph')+'<hr>')
	self.write(entity('h1', "Inheritance Graph"))

	from Synopsis.Formatter import Dot
	graphs = config.classTree.graphs()
	count = 0
	lensorter = lambda a, b: cmp(len(b),len(a))
	graphs = self.consolidate(graphs)
	graphs.sort(lensorter)
	for graph in graphs:
	    try:
		if core.verbose: print "Creating graph #%s - %s classes"%(count,len(graph))
		# Find declarations
		declarations = map(self.__todecl, graph)
		declarations = filter(lambda x: x is not None, declarations)
		# Call Dot formatter
		output = self.__filename+"-dot%s"%count
		config.toc.store(output+".toc")
		args = ('-i','-f','html','-o',output,'-r',output+'.toc','-t','Synopsis %s'%count,'-n')
		Dot.format(config.types, declarations, args, None)
		dot_file = open(output+'.html', 'r')
		self.write(dot_file.read())
		dot_file.close()
		os.remove(output + ".html")
		os.remove(output + ".toc")
	    except:
		import traceback
		traceback.print_exc()
		print "Graph:",graph
		print "Declarations:",declarations
	    count = count + 1

	self.endFile() 


htmlPageClass = InheritanceGraph
