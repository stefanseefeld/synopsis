# $Id: Stripper.py,v 1.3 2002/10/28 16:30:05 chalky Exp $
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
# $Log: Stripper.py,v $
# Revision 1.3  2002/10/28 16:30:05  chalky
# Trying to fix some bugs in the unduplication/stripping stages. Needs more work
#
# Revision 1.2  2002/10/11 05:57:17  chalky
# Support suspect comments
#
# Revision 1.1  2002/08/23 04:37:26  chalky
# Huge refactoring of Linker to make it modular, and use a config system similar
# to the HTML package
#

import string

from Synopsis.Core import AST, Type, Util

from Linker import config, Operation

def filterName(name, prefixes):
    if not prefixes: return 1
    for prefix in prefixes:
	if Util.ccolonName(name)[:len(prefix)] == prefix:
	    return 1
    return 0

class Stripper(Operation, AST.Visitor):
    """Strip common prefix from the declaration's name.
    Keep a list of root nodes, such that children whos parent
    scopes are not accepted but which themselfs are correct can
    be maintained as new root nodes."""
    def __init__(self):
	self.__strip = config.strip
	splitter = lambda x: tuple(string.split(x, "::"))
        self.__scopes = map(splitter, self.__strip)
        self.__root = []
        self.__in = 0

    def execute(self, ast):
        # strip prefixes and remove non-matching declarations
        self.stripDeclarations(ast.declarations())

        # Remove types not in strip
        self.stripTypes(ast.types())

    def _stripName(self, name):
	for scope in self.__scopes:
	    depth = len(scope)
	    if name[0:depth] == scope:
		if len(name) == depth: return None
		return name[depth:]
	return None
    def stripDeclarations(self, declarations):
	for decl in declarations:
	    decl.accept(self)
	declarations[:] = self.declarations()
	
    def stripTypes(self, types):
	prefixes = self.__strip
	# Remove the empty type (caused by C++ with extract_tails)
	if types.has_key(()): del types[()]
	if not prefixes: return
	for name, type in types.items():
	    try:
		del types[name]
		name = self._stripName(name)
		if name:
		    type.set_name(name)
		    types[name] = type
	    except:
		print "ERROR Processing:",name,types[name]
		raise

    def declarations(self): return self.__root

    def strip(self, declaration):
        """test whether the declaration matches one of the prefixes, strip
        it off, and return success. Success means that the declaration matches
        the prefix set and thus should not be removed from the AST."""
        passed = 0
        if not self.__scopes: return 1
        for scope in self.__scopes:
            depth = len(scope)
            name = declaration.name()
            if name[0:depth] == scope:
                if len(name) == depth: break
                if config.verbose: print "symbol", string.join(name, "::"),
                declaration.set_name(name[depth:])
                if config.verbose: print "stripped to", string.join(name, "::")
                passed = 1
            else: break
        if config.verbose and not passed: print "symbol", string.join(declaration.name(), "::"), "removed"
        return passed

    def visitScope(self, scope):
        root = self.strip(scope) and not self.__in
        if root:
            self.__in = 1
            self.__root.append(scope)
        for declaration in scope.declarations():
            declaration.accept(self)
        if root: self.__in = 0

    def visitClass(self, clas):
	self.visitScope(clas)
	templ = clas.template()
	if templ:
	    name = self._stripName(templ.name())
	    if name: templ.set_name(name)

    def visitDeclaration(self, decl):
        if self.strip(decl) and not self.__in:
            self.__root.append(decl)

    def visitEnumerator(self, enumerator):
         self.strip(enumerator)

    def visitEnum(self, enum):
	self.visitDeclaration(enum)
        for e in enum.enumerators():
            e.accept(self)

    def visitFunction(self, function):
	self.visitDeclaration(function)
        for parameter in function.parameters():
            parameter.accept(self)
	templ = function.template()
	if templ:
	    name = self._stripName(templ.name())
	    if name: templ.set_name(name)

    def visitOperation(self, operation):
	self.visitFunction(operation)

    def visitMetaModule(self, module):
	self.visitScope(module)
	for decl in module.module_declarations():
	    name = self._stripName(decl.name())
	    if name: decl.set_name(name)

linkerOperation = Stripper
