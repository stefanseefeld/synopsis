# $Id: AccessRestrictor.py,v 1.1 2002/08/23 04:37:26 chalky Exp $
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
# $Log: AccessRestrictor.py,v $
# Revision 1.1  2002/08/23 04:37:26  chalky
# Huge refactoring of Linker to make it modular, and use a config system similar
# to the HTML package
#

import string

from Synopsis.Core import AST, Type, Util

from Linker import config, Operation

class AccessRestrictor(AST.Visitor):
    """This class processes declarations, and removes those that need greated
    access than the maximum passed to the constructor"""
    def __init__(self):
	self.__max = config.max_access
	self.__scopestack = []
	self.__currscope = []
    def execute(self, ast):
	if self.__max is None: return
	declarations = ast.declarations()
	for decl in declarations:
	    decl.accept(self)
	declarations[:] = self.__currscope
    def push(self):
	self.__scopestack.append(self.__currscope)
	self.__currscope = []
    def pop(self, decl):
	self.__currscope = self.__scopestack.pop()
	self.__currscope.append(decl)
    def add(self, decl):
	self.__currscope.append(decl)
    def currscope(self): return self.__currscope

    def visitDeclaration(self, decl):
	if decl.accessibility() > self.__max: return
	self.add(decl)
    def visitScope(self, scope):
	if scope.accessibility() > self.__max: return
	self.push()
	for decl in scope.declarations():
	    decl.accept(self)
	scope.declarations()[:] = self.__currscope
	self.pop(scope)

linkerOperation = AccessRestrictor
