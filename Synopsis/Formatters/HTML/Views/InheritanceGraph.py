# $Id: InheritanceGraph.py,v 1.26 2003/11/12 16:42:05 stefan Exp $
#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Processor import Parameter
from Synopsis import AST, Type, Util

import core
from Page import Page
from core import config
from Tags import *

import os

class ToDecl(Type.Visitor):
   def __call__(self, name):
      try:
         typeobj = config.types[name]
      except KeyError:
         # Eg: Unknown parent which has been NameMapped
         #if config.verbose: print "Warning: %s not found in types dict."%(name,)
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

class InheritanceGraph(Page):

   def register(self, manager):

      Page.register(self, manager)
      self.__todecl = ToDecl()
      self.__direction = 'vertical'
      if hasattr(config.obj,'InheritanceGraph'):
         if hasattr(config.obj.InheritanceGraph,'direction'):
            if config.obj.InheritanceGraph.direction == 'horizontal':
               self.__direction = 'horizontal'
      self.manager.addRootPage(self.filename(), 'Inheritance Graph', 'main', 1)

   def filename(self): return config.files.nameOfSpecial('InheritanceGraph')
   def title(self): return 'Synopsis - Class Hierarchy'

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
         from Synopsis.Formatters import Dot
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
               temp_ast = AST.AST({}, declarations, config.types)
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
