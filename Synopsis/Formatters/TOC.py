# $Id: TOC.py,v 1.1 2001/02/01 18:37:25 chalky Exp $
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
# $Log: TOC.py,v $
# Revision 1.1  2001/02/01 18:37:25  chalky
# moved TOC out to here
#
#

"""Table of Contents classes"""

import string

from Synopsis.Core import AST, Util

# Not sure how this should be set..
verbose = 0

class Linker:
    """Abstract class for linking declarations. This class has only one
    method, link(decl), which returns the link for the given declaration. This
    is dependant on the type of formatter used, and so a Linker derivative
    must be passed to the TOC upon creation."""
    def link(decl): pass

class TocEntry:
    """Struct for an entry in the table of contents.
    Vars: link, lang, type (all strings)
    Also: name (scoped)"""
    def __init__(self, name, link, lang, type):
	self.name = name
	self.link = link
	self.lang = lang
	self.type = type

class TableOfContents(AST.Visitor):
    """
    Maintains a dictionary of all declarations which can be looked up to create
    cross references. Names are fully scoped.
    """
    def __init__(self, linker):
	"""linker is an instance that implements the Linker interface and is
	used to generate the links from declarations."""
	self.__toc = {}
	self.linker = linker
    
    def lookup(self, name):
	name = tuple(name)
        if self.__toc.has_key(name): return self.__toc[name]
	if verbose and len(name) > 1:
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
    
    def insert(self, entry): self.__toc[tuple(entry.name)] = entry
    
    def store(self, file):
        """store the table of contents into a file, such that it can be used later when cross referencing"""
        fout = open(file, 'w')
        for name in self.__toc.keys():
            scopedname = string.join(name, "::")
            lang = self.__toc[tuple(name)].lang
	    link = self.__toc[tuple(name)].link
            fout.write(scopedname + "," + lang + "," + link + "\n")
            
    def load(self, resource):
        args = string.split(resource, "|")
        file = args[0]
        if len(args) > 1: url = args[1]
        else: url = ""
        fin = open(file, 'r')
        line = fin.readline()
        while line:
            if line[-1] == '\n': line = line[:-1]
            scopedname, lang, link = string.split(line, ",")
            name = string.split(scopedname, "::")
            if len(url): link = string.join([url, link], "/")
            entry = TocEntry(name, link, lang, "decl")
            self.insert(entry)
            line = fin.readline()
    
    def visitAST(self, ast):
	for decl in ast.declarations():
	    decl.accept(self)
    def visitDeclaration(self, decl):
	entry = TocEntry(decl.name(), self.linker.link(decl), decl.language(), "decl")
	self.insert(entry)
    def visitForward(self, decl):
	pass


