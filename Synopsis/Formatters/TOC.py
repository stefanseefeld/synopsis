#
# Copyright (C) 2000 Stefan Seefeld
# Copyright (C) 2000 Stephen Davies
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

"""Table of Contents classes"""

from Synopsis import ASG, Util

import re

class Linker:
   """Abstract class for linking declarations. This class has only one
   method, link(decl), which returns the link for the given declaration. This
   is dependant on the type of formatter used, and so a Linker derivative
   must be passed to the TOC upon creation."""

   def link(decl): pass

class TOC(ASG.Visitor):
   """Maintains a dictionary of all declarations which can be looked up
   to create cross references. Names are fully scoped."""

   class Entry:
      """Struct for an entry in the table of contents.
      Vars: link, lang, type (all strings)
      Also: name (scoped)"""

      def __init__(self, name, link, lang, type):

         self.name = name
         self.link = link
         self.lang = lang
         self.type = type

   def __init__(self, linker, verbose = False):
      """linker is an instance that implements the Linker interface and is
      used to generate the links from declarations."""

      self.__toc = {}
      self.linker = linker
      self.verbose = verbose
    
   def lookup(self, name):

      name = tuple(name)
      if self.__toc.has_key(name): return self.__toc[name]
      if self.verbose and len(name) > 1:
         print "Warning: TOC lookup of",name,"failed!"
      return None

   # def referenceName(self, name, scope, label=None, **keys):
   #     """Same as reference but takes a tuple name"""
   #     if not label: label = Util.ccolonName(name, scope)
   #     entry = self[name]
   #     if entry: return apply(href, (entry.link, label), keys)
   #     return label or ''

   def size(self): return len(self.__toc)
    
   __getitem__ = lookup
    
   def insert(self, entry):
      self.__toc[tuple(entry.name)] = entry
    
   def store(self, file):
      """store the table of contents into a file, such that it can be used later when cross referencing"""

      fout = open(file, 'w')
      nocomma = lambda str: str.replace("&","&amp;").replace(",","&2c;")
      for name in self.__toc.keys():
         scopedname = nocomma('::'.join(name))
         lang = self.__toc[tuple(name)].lang
         link = nocomma(self.__toc[tuple(name)].link)
         fout.write(scopedname + "," + lang + "," + link + "\n")

   def load(self, resource):

      args = resource.split('|')
      file = args[0]
      if len(args) > 1: url = args[1]
      else: url = ""
      fin = open(file, 'r')
      line = fin.readline()
      recomma = lambda str: re.sub('&2c;', ',', str)
      while line:
         if line[-1] == '\n': line = line[:-1]
         scopedname, lang, link = line.split(',')
         scopedname, link = recomma(scopedname), recomma(link)
         param_index = scopedname.find('(')
         if param_index >= 0:
            name = scopedname[:param_index].split('::')
            name = name[:-1] + [name[-1]+scopedname[param_index:]]
         else:
            name = scopedname.split('::')
         if len(url): link = [url, link].join('/')
         entry = TOC.Entry(name, link, lang, "decl")
         self.insert(entry)
         line = fin.readline()
    
   def visit_declaration(self, decl):

      file = decl.file
      entry = TOC.Entry(decl.name, self.linker.link(decl),
                        file and file.annotations['language'] or '', 'decl')
      self.insert(entry)

