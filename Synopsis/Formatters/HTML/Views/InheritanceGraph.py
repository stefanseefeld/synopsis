# $Id: InheritanceGraph.py,v 1.22 2002/10/28 16:27:22 chalky Exp $
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
# Revision 1.22  2002/10/28 16:27:22  chalky
# Support horizontal inheritance graphs
#
# Revision 1.21  2002/10/28 11:44:57  chalky
# Split graphs into groups based on common prefixes, and display each separately
#
# Revision 1.20  2002/10/28 06:13:07  chalky
# Fix typo
#
# Revision 1.19  2002/10/27 12:03:49  chalky
# Rename min_size option to min_group_size, add new min_size option
#
# Revision 1.18  2002/07/19 14:26:32  chalky
# Revert prefix in FileLayout but keep relative referencing elsewhere.
#
# Revision 1.17  2002/07/11 02:09:33  chalky
# Patch from Patrick Mauritz: Use png support in latest graphviz. If dot not
# present, don't subvert what the user asked for but instead tell them.
#
# Revision 1.16  2002/07/04 06:43:18  chalky
# Improved support for absolute references - pages known their full path.
#
# Revision 1.15  2001/07/19 04:03:05  chalky
# New .syn file format.
#
# Revision 1.14  2001/07/11 01:45:03  stefan
# fix Dot and HTML formatters to adjust the references depending on the filename of the output
#
# Revision 1.13  2001/07/05 05:39:58  stefan
# advanced a lot in the refactoring of the HTML module.
# Page now is a truely polymorphic (abstract) class. Some derived classes
# implement the 'filename()' method as a constant, some return a variable
# dependent on what the current scope is...
#
# Revision 1.12  2001/07/05 02:08:35  uid20151
# Changed the registration of pages to be part of a two-phase construction
#
# Revision 1.11  2001/06/28 07:22:18  stefan
# more refactoring/cleanup in the HTML formatter
#
# Revision 1.10  2001/06/26 04:32:16  stefan
# A whole slew of changes mostly to fix the HTML formatter's output generation,
# i.e. to make the output more robust towards changes in the layout of files.
#
# the rpm script now works, i.e. it generates source and binary packages.
#
# Revision 1.9  2001/06/06 04:44:11  uid20151
# Only create TOC once instead of for every graph
#
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
	
def find_common_name(graph):
    common_name = list(graph[0])
    for decl_name in graph[1:]:
	if len(common_name) > len(decl_name):
	    common_name = common_name[:len(decl_name)]
	for i in range(min(len(decl_name), len(common_name))):
	    if decl_name[i] != common_name[i]:
		common_name = common_name[:i]
		break
    return string.join(common_name, '::')
	    

class InheritanceGraph(Page.Page):
    def __init__(self, manager):
	Page.Page.__init__(self, manager)
	self.__todecl = ToDecl()
        self.__direction = 'vertical'
        if hasattr(config.obj,'InheritanceGraph'):
            if hasattr(config.obj.InheritanceGraph,'direction'):
                if config.obj.InheritanceGraph.direction == 'horizontal':
                    self.__direction = 'horizontal'

    def filename(self): return config.files.nameOfSpecial('InheritanceGraph')
    def title(self): return 'Synopsis - Class Hierarchy'

    def register(self):
	"""Registers this page with the manager"""
	self.manager.addRootPage(self.filename(), 'Inheritance Graph', 'main', 1)
 
    def consolidate(self, graphs):
	"""Consolidates small graphs into larger ones"""
	try:
	    min_size = config.obj.InheritanceGraph.min_size
	except:
	    if config.verbose:
		print "Error getting InheritanceGraph.min_size value. Using 1."
	    min_size = 1
	try:
	    min_group_size = config.obj.InheritanceGraph.min_group_size
	except:
	    if config.verbose:
		print "Error getting InheritanceGraph.min_group_size value. Using 5."
	    min_group_size = 5
	# Weed out the small graphs and group by common base name
	common = {}
	for graph in graphs:
	    len_graph = len(graph)
	    if len_graph < min_size:
		# Ignore the graph
		continue
	    common.setdefault(find_common_name(graph), []).append(graph)
	# Consolidate each group
	for name, graphs in common.items():
	    conned = []
	    pending = []
	    for graph in graphs:
		# Try adding to an already pending graph
		for pend in pending:
		    if len_graph + len(pend) <= min_group_size:
			pend.extend(graph)
			graph = None
			if len(pend) == min_group_size:
			    conned.append(pend)
			    pending.remove(pend)
			break
		if graph:
		    if len_graph >= min_group_size:
			# Add to final list
			conned.append(graph)
		    else:
			# Add to pending list
			pending.append(graph)
	    graphs[:] = conned + pending
	return common

    def process(self, start):
	"""Creates a file with the inheritance graph"""
	filename = self.filename()
	self.start_file()
	self.write(self.manager.formatHeader(filename))
	self.write(entity('h1', "Inheritance Graph"))

	try:
	    from Synopsis.Formatter import Dot
	except:
	    print "InheritanceGraph: Can't load the Dot formatter"
	    self.end_file()
	    return
	# Create a toc file for Dot to use
	toc_file = filename + "-dot.toc"
	config.toc.store(toc_file)
	graphs = config.classTree.graphs()
	count = 0
	# Consolidate the graphs, and sort to make the largest appear first
	lensorter = lambda a, b: cmp(len(b),len(a))
	common_graphs = self.consolidate(graphs)
	names = common_graphs.keys()
	names.sort()
	for name in names:
	    graphs = common_graphs[name]
	    graphs.sort(lensorter)
	    if name:
		self.write('<div class="inheritance-group">')
		scoped_name = string.split(name,'::')
		type_str = ''
		if core.config.types.has_key(scoped_name):
		    type = core.config.types[scoped_name]
		    if isinstance(type, Type.Declared):
			type_str = type.declaration().type() + ' '
		self.write('Graphs in '+type_str+name+':<br>')
	    for graph in graphs:
		try:
		    if core.verbose: print "Creating graph #%s - %s classes"%(count,len(graph))
		    # Find declarations
		    declarations = map(self.__todecl, graph)
		    declarations = filter(lambda x: x is not None, declarations)
		    # Call Dot formatter
		    output = os.path.join(config.basename, os.path.splitext(self.filename())[0]) + '-%s'%count
		    args = (
			'-i','-f','html','-o',output,'-r',toc_file,
			'-R',self.filename(),'-t','Synopsis %s'%count,'-n', 
			'-p',name,'-d',self.__direction)
		    temp_ast = AST.AST([''], declarations, config.types)
		    Dot.format(args, temp_ast, None)
		    dot_file = open(output + '.html', 'r')
		    self.write(dot_file.read())
		    dot_file.close()
		    os.remove(output + ".html")
		except:
		    import traceback
		    traceback.print_exc()
		    print "Graph:",graph
		    print "Declarations:",declarations
		count = count + 1
	    if name:
		self.write('</div>')

	os.remove(toc_file)

	self.end_file() 


htmlPageClass = InheritanceGraph
