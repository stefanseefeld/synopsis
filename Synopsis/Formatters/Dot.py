#
# Copyright (C) 2000 Stefan Seefeld
# Copyright (C) 2000 Stephen Davies
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

"""
Uses 'dot' from graphviz to generate various graphs.
"""

from Synopsis.Processor import Processor, Parameter
from Synopsis import AST, Type, Util
from Synopsis.Formatters import TOC

import sys, tempfile, getopt, os, os.path, string, types, errno, re

verbose = False

class SystemError:
   """Error thrown by the system() function. Attributes are 'retval', encoded
   as per os.wait(): low-byte is killing signal number, high-byte is return
   value of command."""

   def __init__(self, retval, command):

      self.retval = retval
      self.command = command

   def __repr__(self):

      return "SystemError: %(retval)x\"%(command)s\" failed."%self.__dict__

def system(command):
   """Run the command. If the command fails, an exception SystemError is
   thrown."""

   ret = os.system(command)
   if (ret>>8) != 0:
      raise SystemError(ret, command)

class DotFileGenerator:
   """A class that encapsulates the dot file generation"""
   def __init__(self, os, direction):

      self.__os = os
      self.direction = direction
      self.nodes = {}

   def write(self, text): self.__os.write(text)

   def write_node(self, ref, name, label, **attr):
      """helper method to generate output for a given node"""

      if self.nodes.has_key(name): return
      self.nodes[name] = len(self.nodes)
      number = self.nodes[name]

      # Quote to remove characters that dot can't handle
      label = re.sub('<',r'\<',label)
      label = re.sub('>',r'\>',label)
      label = re.sub('{',r'\{',label)
      label = re.sub('}',r'\}',label)

      self.write("Node" + str(number) + " [shape=\"record\", label=\"{" + label + "}\"")
      self.write(", fontSize = 10, height = 0.2, width = 0.4")
      self.write(string.join(map(lambda item:', %s="%s"'%item, attr.items())))
      if ref: self.write(", URL=\"" + ref + "\"")
      self.write("];\n")

   def write_edge(self, parent, child, **attr):

      self.write("Node" + str(self.nodes[parent]) + " -> ")
      self.write("Node" + str(self.nodes[child]))
      self.write("[ color=\"black\", fontsize=10, dir=back" + string.join(map(lambda item:', %s="%s"'%item, attr.items())) + "];\n")

class InheritanceGenerator(DotFileGenerator, AST.Visitor, Type.Visitor):
   """A Formatter that generates an inheritance graph. If the 'toc' argument is not None,
   it is used to generate URLs. If no reference could be found in the toc, the node will
   be grayed out."""
   def __init__(self, os, direction, operations, attributes, aggregation,
                toc, prefix, no_descend):
      
      DotFileGenerator.__init__(self, os, direction)
      if operations: self.__operations = []
      else: self.__operations = None
      if attributes: self.__attributes = []
      else: self.__attributes = None
      self.aggregation = aggregation
      self.toc = toc
      self.__scope = []
      if prefix: self.__scope = string.split(prefix, '::')
      self.__type_ref = None
      self.__type_label = ''
      self.__no_descend = no_descend
      self.nodes = {}

   def scope(self): return self.__scope
   def type_ref(self): return self.__type_ref
   def type_label(self): return self.__type_label
   def parameter(self): return self.__parameter

   def format_type(self, typeObj):
      "Returns a reference string for the given type object"

      if typeObj is None: return "(unknown)"
      typeObj.accept(self)
      return self.type_label()

   def clear_type(self):

      self.__type_ref = None
      self.__type_label = ''
      
   def get_class_name(self, node):
      """Returns the name of the given class node, relative to all its
      parents. This makes the graph simpler by making the names shorter"""

      base = node.name()
      for i in node.parents():
         try:
            parent = i.parent()
            pname = parent.name()
            for j in range(len(base)):
               if j > len(pname) or pname[j] != base[j]:
                  # Base is longer than parent name, or found a difference
                  base[j:] = []
                  break
         except: pass # typedefs etc may cause errors here.. ignore
      if not node.parents():
         base = self.scope()
      return Util.ccolonName(node.name(), base)

   #################### Type Visitor ##########################################

   def visitModifier(self, type):

      self.format_type(type.alias())
      self.__type_label = string.join(type.premod()) + self.__type_label
      self.__type_label = self.__type_label + string.join(type.postmod())

   def visitUnknown(self, type):

      self.__type_ref = self.toc and self.toc[type.link()] or None
      self.__type_label = Util.ccolonName(type.name(), self.scope())
        
   def visitBase(self, type):

      self.__type_ref = None
      self.__type_label = type.name()[-1]

   def visitDependent(self, type):

      self.__type_ref = None
      self.__type_label = type.name()[-1]
        
   def visitDeclared(self, type):

      self.__type_ref = self.toc and self.toc[type.declaration().name()] or None
      if isinstance(type.declaration(), AST.Class):
         self.__type_label = self.get_class_name(type.declaration())
      else:
         self.__type_label = Util.ccolonName(type.declaration().name(), self.scope())

   def visitParametrized(self, type):

      if type.template():
         type_ref = self.toc and self.toc[type.template().name()] or None
         type_label = Util.ccolonName(type.template().name(), self.scope())
      else:
         type_ref = None
         type_label = "(unknown)"
      parameters_label = []
      for p in type.parameters():
         parameters_label.append(self.format_type(p))
      self.__type_ref = type_ref
      self.__type_label = type_label + "<" + string.join(parameters_label, ",") + ">"

   def visitTemplate(self, type):
      self.__type_ref = None
      def clip(x, max=20):
         if len(x) > max: return '...'
         return x
      self.__type_label = "template<%s>"%(clip(string.join(map(clip, map(self.format_type, type.parameters())), ","),40))

   #################### AST Visitor ###########################################

   def visitInheritance(self, node):

      self.format_type(node.parent())
      if self.type_ref():
         self.write_node(self.type_ref().link, self.type_label(), self.type_label())
      elif self.toc:
         self.write_node('', self.type_label(), self.type_label(), color='gray75', fontcolor='gray75')
      else:
         self.write_node('', self.type_label(), self.type_label())
        
   def visitClass(self, node):

      if self.__operations is not None: self.__operations.append([])
      if self.__attributes is not None: self.__attributes.append([])
      name = self.get_class_name(node)
      ref = self.toc and self.toc[node.name()] or None
      for d in node.declarations(): d.accept(self)
      # NB: old version of dot needed the label surrounded in {}'s (?)
      label = name
      if node.template():
         if self.direction == 'vertical':
            label = self.format_type(node.template()) + '\\n' + label
         else:
            label = self.format_type(node.template()) + ' ' + label
      if self.__operations or self.__attributes:
         label = label + '\\n'
         if self.__operations:
            label += '|' + string.join(map(lambda x:x[-1] + '()\\l', self.__operations[-1]), '')
         if self.__attributes:
            label += '|' + string.join(map(lambda x:x[-1] + '\\l', self.__attributes[-1]), '')
      if ref:
         self.write_node(ref.link, name, label)
      elif self.toc:
         self.write_node('', name, label, color='gray75', fontcolor='gray75')
      else:
         self.write_node('', name, label)

      if self.aggregation:
         #FIXME: we shouldn't only be looking for variables of the exact type,
         #       but also derived types such as pointers, references, STL containers, etc.
         #
         # find attributes of type 'Class' so we can link to it
         for a in filter(lambda a:isinstance(a, AST.Variable), node.declarations()):
            if isinstance(a.vtype(), Type.Declared):
               d = a.vtype().declaration()
               if isinstance(d, AST.Class) and self.nodes.has_key(self.get_class_name(d)):
                  self.write_edge(self.get_class_name(node), self.get_class_name(d),
                                  arrowtail='ediamond')

      for inheritance in node.parents():
         inheritance.accept(self)
         self.write_edge(self.type_label(), name, arrowtail='empty')
      if self.__no_descend: return
      if self.__operations: self.__operations.pop()
      if self.__attributes: self.__attributes.pop()

   def visitOperation(self, operation):

      if self.__operations:
         self.__operations[-1].append(operation.realname())

   def visitVariable(self, variable):

      if self.__attributes:
         self.__attributes[-1].append(variable.name())

class SingleInheritanceGenerator(InheritanceGenerator):
   """A Formatter that generates an inheritance graph for a specific class.
   This Visitor visits the AST upwards, i.e. following the inheritance links, instead of
   the declarations contained in a given scope."""

   def __init__(self, os, direction, operations, attributes, levels, types,
                toc, prefix, no_descend):
      InheritanceGenerator.__init__(self, os, direction, operations, attributes, False,
                                    toc, prefix, no_descend)
      self.__levels = levels
      self.__types = types
      self.__current = 1
      self.__visited_classes = {} # classes already visited, to prevent recursion

   #################### Type Visitor ##########################################

   def visitDeclared(self, type):
      if self.__current < self.__levels or self.__levels == -1:
         self.__current = self.__current + 1
         type.declaration().accept(self)
         self.__current = self.__current - 1
      # to restore the ref/label...
      InheritanceGenerator.visitDeclared(self, type)

   #################### AST Visitor ###########################################
        
   def visitInheritance(self, node):

      node.parent().accept(self)
      if self.type_label():
         if self.type_ref():
            self.write_node(self.type_ref().link, self.type_label(), self.type_label())
         elif self.toc:
            self.write_node('', self.type_label(), self.type_label(), color='gray75', fontcolor='gray75')
         else:
            self.write_node('', self.type_label(), self.type_label())
        
   def visitClass(self, node):

      # Prevent recursion
      if self.__visited_classes.has_key(id(node)): return
      self.__visited_classes[id(node)] = None

      name = self.get_class_name(node)
      if self.__current == 1:
         self.write_node('', name, name, style='filled', color='lightgrey')
      else:
         ref = self.toc and self.toc[node.name()] or None
         if ref:
            self.write_node(ref.link, name, name)
         elif self.toc:
            self.write_node('', name, name, color='gray75', fontcolor='gray75')
         else:
            self.write_node('', name, name)

      for inheritance in node.parents():
         inheritance.accept(self)
         if self.nodes.has_key(self.type_label()):
            self.write_edge(self.type_label(), name, arrowtail='empty')
      # if this is the main class and if there is a type dictionary,
      # look for classes that are derived from this class

      # if this is the main class
      if self.__current == 1 and self.__types:
         # fool the visitDeclared method to stop walking upwards
         self.__levels = 0
         for t in self.__types.values():
            if isinstance(t, Type.Declared):
               child = t.declaration()
               if isinstance(child, AST.Class):
                  for inheritance in child.parents():
                     type = inheritance.parent()
                     type.accept(self)
                     if self.type_ref():
                        if self.type_ref().name == node.name():
                           child_label = self.get_class_name(child)
                           ref = self.toc and self.toc[child.name()] or None
                           if ref:
                              self.write_node(ref.link, child_label, child_label)
                           elif self.toc:
                              self.write_node('', child_label, child_label, color='gray75', fontcolor='gray75')
                           else:
                              self.write_node('', child_label, child_label)

                           self.write_edge(name, child_label, arrowtail='empty')

class FileDependencyGenerator(DotFileGenerator, AST.Visitor, Type.Visitor):
   """A Formatter that generates a file dependency graph"""

   def visit_file(self, file):
      if file.is_main():
         self.write_node('', file.filename(), file.filename())
      for i in file.includes():
         target = i.target()
         if target.is_main():
            self.write_node('', target.filename(), target.filename())
            name = i.name()
            name = name.replace('"', '\\"')
            self.write_edge(target.filename(), file.filename(),
                            label=name, style='dashed')

def _rel(frm, to):
   "Find link to to relative to frm"

   frm = string.split(frm, '/'); to = string.split(to, '/')
   for l in range((len(frm)<len(to)) and len(frm)-1 or len(to)-1):
      if to[0] == frm[0]: del to[0]; del frm[0]
      else: break
   if frm: to = ['..'] * (len(frm) - 1) + to
   return string.join(to,'/')

def _convert_map(input, output, base_url):
   """convert map generated from Dot to a html region map.
   input and output are (open) streams"""

   line = input.readline()
   while line:
      line = line[:-1]
      if line[0:4] == "rect":
         url, x1y1, x2y2 = string.split(line[4:])
         x1, y1 = string.split(x1y1, ",")
         x2, y2 = string.split(x2y2, ",")
         output.write('<area alt="'+url+'" href="' + _rel(base_url, url) + '" shape="rect" coords="')
         output.write(str(x1) + ", " + str(y1) + ", " + str(x2) + ", " + str(y2) + '" />\n')
      line = input.readline()

def _format(input, output, format):

   command = 'dot -T%s -o "%s" "%s"'%(format, output, input)
   if verbose: print "Dot Formatter: running command '" + command + "'"
   system(command)

def _format_png(input, output): _format(input, output, "png")

def _format_html(input, output, base_url):
   """generate (active) image for html.
   input and output are file names. If output ends
   in '.html', its stem is used with an '.png' suffix for the
   actual image."""

   if output[-5:] == ".html": output = output[:-5]
   _format_png(input, output + ".png")
   _format(input, output + ".map", "imap")
   prefix, name = os.path.split(output)
   reference = name + ".png"
   html = Util.open(output + ".html")
   html.write('<img alt="'+name+'" src="' + reference + '" hspace="8" vspace="8" border="0" usemap="#')
   html.write(name + "_map\" />\n")
   html.write("<map name=\"" + name + "_map\">")
   dotmap = open(output + ".map", "r+")
   _convert_map(dotmap, html, base_url)
   dotmap.close()
   os.remove(output + ".map")
   html.write("</map>\n")

class Formatter(Processor):
   """The Formatter class acts merely as a frontend to
   the various InheritanceGenerators"""

   title = Parameter('Inheritance Graph', 'the title of the graph')
   type = Parameter('class', 'type of graph (one of \'file\', \'class\', \'single\'')
   hide_operations = Parameter(True, 'hide operations')
   hide_attributes = Parameter(True, 'hide attributes')
   show_aggregation = Parameter(False, 'show aggregation')
   format = Parameter('ps', "Generate output in format 'dot', 'ps', 'png', 'gif', 'map', 'html'")
   layout = Parameter('vertical', 'Direction of graph')
   prefix = Parameter(None, 'Prefix to strip from all class names')
   toc_in = Parameter([], 'list of table of content files to use for symbol lookup')
   base_url = Parameter(None, 'base url to use for generated links')

   "-n                    Don't descend AST (for internal use)"

   def process(self, ast, **kwds):
      global verbose
      
      self.set_parameters(kwds)
      self.ast = self.merge_input(ast)
      verbose = self.verbose

      formats = {'dot' : 'dot',
                 'ps' : 'ps',
                 'png' : 'png',
                 'gif' : 'gif',
                 'map' : 'imap',
                 'html' : 'html'}

      if formats.has_key(self.format): format = formats[self.format]
      else:
         print "Error: Unknown format. Available formats are:",
         print string.join(formats.keys(), ', ')
         return self.ast

      # we only need the toc if format=='html'
      if format == 'html':
         # beware: HTML.Fragments.ClassHierarchyGraph sets self.toc !!
         toc = getattr(self, 'toc', TOC.TOC(TOC.Linker()))
         for t in self.toc_in: toc.load(t)
      else:
         toc = None

      head, tail = os.path.split(self.output)
      tmpfile = os.path.join(head, Util.quote(tail)) + ".dot"
      if self.verbose: print "Dot Formatter: Writing dot file..."
      dotfile = Util.open(tmpfile)
      dotfile.write("digraph \"%s\" {\n"%(self.title))
      if self.layout == 'horizontal':
         dotfile.write('rankdir="LR";\n')
         dotfile.write('ranksep="1.0";\n')
      dotfile.write("node[shape=record, fontsize=10, height=0.2, width=0.4, color=black]\n")
      if self.type == 'single':
         generator = SingleInheritanceGenerator(dotfile, self.layout,
                                                not self.hide_operations,
                                                not self.hide_attributes,
                                                -1, self.ast.types(),
                                                toc, self.prefix, False)
      elif self.type == 'class':
         generator = InheritanceGenerator(dotfile, self.layout,
                                          not self.hide_operations,
                                          not self.hide_attributes,
                                          self.show_aggregation,
                                          toc, self.prefix, False)
      elif self.type == 'file':
         generator = FileDependencyGenerator(dotfile, self.layout)
      else:
         sys.stderr.write("Dot: unknown type\n");
         

      if self.type == 'file':
         for f in self.ast.files().values():
            generator.visit_file(f)
      else:
         for d in self.ast.declarations():
            d.accept(generator)
      dotfile.write("}\n")
      dotfile.close()
      if format == "dot":
         os.rename(tmpfile, self.output)
      elif format == "png":
         _format_png(tmpfile, self.output)
         os.remove(tmpfile)
      elif format == "html":
         _format_html(tmpfile, self.output, self.base_url)
         os.remove(tmpfile)
      else:
         _format(tmpfile, self.output, format)
         os.remove(tmpfile)

      return self.ast

