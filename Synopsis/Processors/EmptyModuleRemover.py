# $Id: EmptyModuleRemover.py,v 1.1 2002/08/23 04:37:26 chalky Exp $
#
# This file is a part of Synopsis.
# Copyright (C) 2000, 2001 Stefan Seefeld
# Copyright (C) 2000, 2001 Stephen Davies
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
# $Log: EmptyModuleRemover.py,v $
# Revision 1.1  2002/08/23 04:37:26  chalky
# Huge refactoring of Linker to make it modular, and use a config system similar
# to the HTML package
#

import string

from Synopsis.Core import AST, Type, Util

from Linker import config, Operation

class EmptyNS (Operation, AST.Visitor):
    """A class that removes empty namespaces"""
    def __init__(self):
	"""Overrides the default processAll() to setup the stack"""
	self.__scopestack = []
	self.__currscope = []
    def execute(self, ast):
	declarations = ast.declarations()
	for decl in declarations:
	    decl.accept(self)
	declarations[:] = self.__currscope
    def push(self):
	"""Pushes the current scope onto the stack and starts a new one"""
	self.__scopestack.append(self.__currscope)
	self.__currscope = []
    def pop(self, decl):
	"""Pops the current scope from the stack, and appends the given
	declaration to it"""
	self.__currscope = self.__scopestack.pop()
	self.__currscope.append(decl)
    def pop_only(self):
	"""Only pops, doesn't append to scope"""
	self.__currscope = self.__scopestack.pop()
    def add(self, decl):
	"""Adds the given decl to the current scope"""
	self.__currscope.append(decl)
    def currscope(self):
	"""Returns the current scope: a list of declarations"""
	return self.__currscope
    def visitDeclaration(self, decl):
	"""Adds declaration to scope"""
	self.add(decl)
    def visitGroup(self, group):
	"""Overrides recursive behaviour to just add the group"""
	self.add(group)
    def visitEnum(self, enum):
	"""Overrides recursive behaviour to just add the enum"""
	self.add(enum)
    def visitModule(self, module):
	"""Visits all children of the module, and if there are no declarations
	after that removes the module"""
	self.push()
	for decl in module.declarations(): decl.accept(self)
	module.declarations()[:] = self.currscope()
	count = self._count_not_forwards(self.currscope())
	#print string.join(module.name(),'::'),"%d (%d)"%(len(self.currscope()),count),"children"
	if count: self.pop(module)
	else: self.pop_only()
    def _count_not_forwards(self, decls):
	"""Returns the number of declarations not instances of AST.Forward"""
	count = 0
	for decl in decls:
	    if not isinstance(decl, AST.Forward): count = count+1
	return count

linkerOperation = EmptyNS
