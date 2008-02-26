#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Processor import *
from Synopsis import IR, ASG
from Synopsis.Formatters.HTML.View import View
from Synopsis.Formatters.HTML.Tags import *

import os

class DeclarationFinder(ASG.Visitor):
   def __init__(self, types, verbose):

      self.types = types
      self.verbose = verbose

   def __call__(self, name):
      try:
         typeobj = self.types[name]
      except KeyError:
         # Eg: Unknown parent which has been NameMapped
         if self.verbose: print "Warning: %s not found in type dict."%(name,)
         return None
      self.__decl = None
      typeobj.accept(self)
      #if self.__decl is None:
      #    return None
      return self.__decl
	    
   def visit_base_type(self, type): return
   def visit_unknown_type(self, type): return
   def visit_declared_type(self, type): self.__decl = type.declaration
   def visit_modifier_type(self, type): type.alias.accept(self)
   def visit_array(self, type): type.alias.accept(self)
   def visit_template(self, type): self.__decl = type.declaration
   def visit_parametrized(self, type): type.template.accept(self)
   def visit_function_type(self, type): return
	
def find_common_name(graph):
   common_name = list(graph[0])
   for decl_name in graph[1:]:
      if len(common_name) > len(decl_name):
         common_name = common_name[:len(decl_name)]
      for i in range(min(len(decl_name), len(common_name))):
         if decl_name[i] != common_name[i]:
            common_name = common_name[:i]
            break
   return '::'.join(common_name)

class InheritanceGraph(View):

   min_size = Parameter(2, 'minimum number of nodes for a graph to be displayed')
   min_group_size = Parameter(5, 'how many nodes to put into a group')
   direction = Parameter('vertical', 'layout of the graph')

   def register(self, frame):

      super(InheritanceGraph, self).register(frame)
      self.decl_finder = DeclarationFinder(self.processor.ir.types,
                                           self.processor.verbose)

   def filename(self):

      if self.main:
         return self.directory_layout.index()
      else:
         return self.directory_layout.special('InheritanceGraph')

   def title(self):

      return 'Inheritance Graph'

   def root(self):

      return self.filename(), self.title()

   def consolidate(self, graphs):
      """Consolidates small graphs into larger ones"""

      # Weed out the small graphs and group by common base name
      common = {}
      for graph in graphs:
         len_graph = len(graph)
         if len_graph < self.min_size:
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
               if len_graph + len(pend) <= self.min_group_size:
                  pend.extend(graph)
                  graph = None
                  if len(pend) == self.min_group_size:
                     conned.append(pend)
                     pending.remove(pend)
                  break
            if graph:
               if len_graph >= self.min_group_size:
                  # Add to final list
                  conned.append(graph)
               else:
                  # Add to pending list
                  pending.append(graph)
         graphs[:] = conned + pending
      return common

   def process(self):
      """Creates a file with the inheritance graph"""

      filename = self.filename()
      self.start_file()
      self.write_navigation_bar()
      self.write(element('h1', "Inheritance Graph"))

      from Synopsis.Formatters import Dot
      # Create a toc file for Dot to use
      toc_file = filename + "-dot.toc"
      self.processor.toc.store(toc_file)
      graphs = self.processor.class_tree.graphs()
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
            type_str = ''
            types = self.processor.ir.types
            type = types.get(name, None)
            if isinstance(type, ASG.Declared):
               type_str = type.declaration.type + ' '
            self.write('Graphs in '+type_str+name+':<br/>')
         for graph in graphs:
            if self.processor.verbose: print "Creating graph #%s - %s classes"%(count,len(graph))
            # Find declarations
            declarations = map(self.decl_finder, graph)
            declarations = filter(lambda x: x is not None, declarations)
            #output = os.path.join(self.processor.output, 'InheritanceGraph', str(count))
            output = os.path.join(self.processor.output,
                                  os.path.splitext(self.filename())[0]) + '-%s'%count
            dot = Dot.Formatter(bgcolor=self.processor.graph_color)
            ir = IR.IR({}, declarations, self.processor.ir.types)
            try:
               dot.process(ir,
                           output=output,
                           format='html',
                           toc_in=[toc_file],
                           base_url=self.filename(),
                           title='Synopsis %s'%count,
                           layout=self.direction)
               dot_file = open(output + '.html', 'r')
               self.write(dot_file.read())
               dot_file.close()
               os.remove(output + ".html")
            except InvalidCommand, e:
               print 'Warning : %s'%str(e)
            count = count + 1
         if name:
            self.write('</div>')

      os.remove(toc_file)

      self.end_file() 
