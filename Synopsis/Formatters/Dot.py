# $Id: Dot.py,v 1.36 2003/11/13 20:40:09 stefan Exp $
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
toc = None
nodes = {}

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

class InheritanceGenerator(AST.Visitor, Type.Visitor):
   """A Formatter that generates an inheritance graph"""
   def __init__(self, os, direction, operations, attributes,
                toc, prefix, no_descend):

      self.__os = os
      self.__direction = direction
      if operations: self.__operations = []
      else: self.__operations = None
      if attributes: self.__attributes = []
      else: self.__attributes = None
      self.toc = toc
      self.__scope = []
      if prefix: self.__scope = string.split(prefix, '::')
      self.__type_ref = None
      self.__type_label = ''
      self.__no_descend = no_descend

   def scope(self): return self.__scope
   def write(self, text): self.__os.write(text)
   def type_ref(self): return self.__type_ref
   def type_label(self): return self.__type_label
   def parameter(self): return self.__parameter

   def formatType(self, typeObj):
      "Returns a reference string for the given type object"

      if typeObj is None: return "(unknown)"
      typeObj.accept(self)
      return self.type_label()

   def clearType(self):

      self.__type_ref = None
      self.__type_label = ''
      
   def writeNode(self, ref, name, label, **attr):
      """helper method to generate output for a given node"""

      if nodes.has_key(name): return
      nodes[name] = len(nodes)
      number = nodes[name]

      # Quote to remove characters that dot can't handle
      label = re.sub('<',r'\<',label)
      label = re.sub('>',r'\>',label)
      label = re.sub('{',r'\{',label)
      label = re.sub('}',r'\}',label)

      self.write("Node" + str(number) + " [shape=\"record\", label=\"" + label + "\"")
      self.write(", fontSize = 10, height = 0.2, width = 0.4")
      self.write(string.join(map(lambda item:', %s="%s"'%item, attr.items())))
      if ref: self.write(", URL=\"" + ref + "\"")
      self.write("];\n")

   def writeEdge(self, parent, child, label, **attr):

      self.write("Node" + str(nodes[parent]) + " -> ")
      self.write("Node" + str(nodes[child]))
      self.write("[ color=\"black\", fontsize=10, dir=back, arrowtail=empty, " + string.join(map(lambda item:', %s="%s"'%item, attr.items())) + "];\n")

   def getClassName(self, node):
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

      self.formatType(type.alias())
      self.__type_label = string.join(type.premod()) + self.__type_label
      self.__type_label = self.__type_label + string.join(type.postmod())

   def visitUnknown(self, type):

      self.__type_ref = self.toc[type.link()]
      self.__type_label = Util.ccolonName(type.name(), self.scope())
        
   def visitBase(self, type):

      self.__type_ref = None
      self.__type_label = type.name()[-1]

   def visitDependent(self, type):

      self.__type_ref = None
      self.__type_label = type.name()[-1]
        
   def visitDeclared(self, type):

      self.__type_ref = self.toc[type.declaration().name()]
      if isinstance(type.declaration(), AST.Class):
         self.__type_label = self.getClassName(type.declaration())
      else:
         self.__type_label = Util.ccolonName(type.declaration().name(), self.scope())

   def visitParametrized(self, type):

      if type.template():
         type_ref = self.toc[type.template().name()]
         type_label = Util.ccolonName(type.template().name(), self.scope())
      else:
         type_ref = None
         type_label = "(unknown)"
      parameters_label = []
      for p in type.parameters():
         parameters_label.append(self.formatType(p))
      self.__type_ref = type_ref
      self.__type_label = type_label + "<" + string.join(parameters_label, ",") + ">"

   def visitTemplate(self, type):
      self.__type_ref = None
      def clip(x, max=20):
         if len(x) > max: return '...'
         return x
      self.__type_label = "template<%s>"%(clip(string.join(map(clip, map(self.formatType, type.parameters())), ","),40))

   #################### AST Visitor ###########################################

   def visitInheritance(self, node):

      self.formatType(node.parent())
      if self.type_ref():
         self.writeNode(self.type_ref().link, self.type_label(), self.type_label())
      else:
         self.writeNode('', self.type_label(), self.type_label(), color='gray75', fontcolor='gray75')
        
   def visitClass(self, node):

      if self.__operations is not None: self.__operations.append([])
      if self.__attributes is not None: self.__attributes.append([])
      name = self.getClassName(node)
      ref = self.toc[node.name()]
      for d in node.declarations(): d.accept(self)
      # NB: old version of dot needed the label surrounded in {}'s (?)
      label = name
      if node.template():
         if self.__direction == 'vertical':
            label = self.formatType(node.template()) + '\\n' + label
         else:
            label = self.formatType(node.template()) + ' ' + label
      if self.__operations or self.__attributes:
         label = label + '\\n'
         if self.__operations:
            label = label + '|' + string.join(map(lambda x:x[-1] + '()\\l', self.__operations[-1]))
         if self.__attributes:
            label = label + '|' + string.join(map(lambda x:x[-1] + '\\l', self.__attributes[-1]))
      if ref:
         self.writeNode(ref.link, name, label)
      else:
         self.writeNode('', name, label, color='gray75', fontcolor='gray75')
      for inheritance in node.parents():
         inheritance.accept(self)
         self.writeEdge(self.type_label(), name, None)
      if self.__no_descend: return
      if self.__operations: self.__operations = self.__operations[:-1]
      if self.__attributes: self.__attributes = self.__attributes[:-1]

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
      InheritanceFormatter.__init__(self, os, direction, operations, attributes,
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
      InheritanceFormatter.visitDeclared(self, type)

   #################### AST Visitor ###########################################
        
   def visitInheritance(self, node):

      node.parent().accept(self)
      if self.type_label():
         if self.type_ref():
            self.writeNode(self.type_ref().link, self.type_label(), self.type_label())
         else:
            self.writeNode('', self.type_label(), self.type_label(), color='gray75', fontcolor='gray75')
        
   def visitClass(self, node):

      # Prevent recursion
      if self.__visited_classes.has_key(id(node)): return
      self.__visited_classes[id(node)] = None

      name = self.getClassName(node)
      if self.__current == 1:
         self.writeNode('', name, name, style='filled', color='lightgrey')
      else:
         ref = self.toc[node.name()]
         if ref:
            self.writeNode(ref.link, name, name)
         else:
            self.writeNode('', name, name, color='gray75', fontcolor='gray75')
      for inheritance in node.parents():
         inheritance.accept(self)
         if nodes.has_key(self.type_label()):
            self.writeEdge(self.type_label(), name, None)
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
                           child_label = self.getClassName(child)
                           ref = self.toc[child.name()]
                           if ref:
                              self.writeNode(ref.link, child_label, child_label)
                           else:
                              self.writeNode('', child_label, child_label, color='gray75', fontcolor='gray75')
                              self.writeEdge(name, child_label, None)

class CollaborationGenerator(AST.Visitor, Type.Visitor):
   """A Formatter that generates a collaboration graph"""

   def __init__(self, output):
      self.__output = output

   def write(self, text): self.__output.write(text)
   def visitClass(self, node):

      name = node.name()
      for inheritance in node.parents():
         parent = inheritance.parent()
         if hasattr(parent, 'name'):
            self.write(parent.name()[-1] + " -> " + name[-1] + ";\n")
      for d in node.declarations(): d.accept(self)

   def visitVariable(self, node):

      node.vtype().accept(self)
      print node.name(), self.__type_label

   def visitBaseType(self, type):

      self.__type_label = type.name()

   def visitUnknown(self, type):

      self.__type_label = type.name()

   def visitDeclared(self, type):

      self.__type_label = type.name()

   def visitModifier(self, type):

      alias = type.alias()
      pre = string.join(map(lambda x:x+"&nbsp;", type.premod()), '')
      post = string.join(type.postmod(), '')
      self.__type_label = "%s%s%s"%(pre,alias,post)

   def visitTemplate(self, type):

      self.__type_label = "<template>"

   def visitParametrized(self, type):

      self.__type_label = "<parametrized>"

   def visitFunctionType(self, type):

      self.__type_label = "<function>"

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
         output.write(str(x1) + ", " + str(y1) + ", " + str(x2) + ", " + str(y2) + '">\n')
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
   html.write(name + "_map\">\n")
   html.write("<map name=\"" + name + "_map\">")
   dotmap = open(output + ".map", "r+")
   _convert_map(dotmap, html, base_url)
   dotmap.close()
   os.remove(output + ".map")
   html.write("</map>\n")

class Formatter(Processor):
   """The Formatter class acts merely as a frontend to
   the various InheritanceFormatters"""

   title = Parameter('Inheritance Graph', 'the title of the graph')
   inheritance = Parameter(True, 'Generate an inheritance graph')
   single = Parameter(False, 'Generate an inheritance graph for a single class')
   collaboration = Parameter(False, 'Generate a collaboration graph')
   hide_operations = Parameter(True, 'hide operations')
   hide_attributes = Parameter(True, 'hide attributes')
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

      if formats.has_key(self.format): oformat = formats[self.format]
      else:
         print "Error: Unknown format. Available formats are:",
         print string.join(formats.keys(), ', ')
         return self.ast

      toc = TOC.TOC(TOC.Linker())
      for t in self.toc_in: toc.load(t)

      head, tail = os.path.split(self.output)
      tmpfile = os.path.join(head, Util.quote(tail)) + ".dot"
      if self.verbose: print "Dot Formatter: Writing dot file..."
      dotfile = Util.open(tmpfile)
      dotfile.write("digraph \"%s\" {\n"%(self.title))
      if self.layout == 'horizontal':
         dotfile.write('rankdir="LR";\n')
         dotfile.write('ranksep="1.0";\n')
      dotfile.write("node[shape=record, fontsize=10, height=0.2, width=0.4, color=black]\n")
      if self.single:
         generator = SingleInheritanceGenerator(dotfile, self.layout,
                                                not self.hide_operations,
                                                not self.hide_attributes,
                                                -1, self.ast.types(),
                                                toc, self.prefix, False)
      elif self.inheritance:
         generator = InheritanceGenerator(dotfile, self.layout,
                                          not self.hide_operations,
                                          not self.hide_attributes,
                                          toc, self.prefix, False)
      elif self.collaboration:
         sys.stderr.write("sorry, collaboration diagrams not yet implemented\n");
         return self.ast
         #generator = CollaborationGenerator(dotfile)
      for d in self.ast.declarations():
            d.accept(generator)
      dotfile.write("}\n")
      dotfile.close()
      if oformat == "dot":
         os.rename(tmpfile, self.output)
      elif oformat == "png":
         _format_png(tmpfile, self.output)
         #os.remove(tmpfile)
      elif oformat == "html":
         _format_html(tmpfile, self.output, self.base_url)
         #os.remove(tmpfile)
      else:
         _format(tmpfile, self.output, oformat)
         os.remove(tmpfile)

      return self.ast

