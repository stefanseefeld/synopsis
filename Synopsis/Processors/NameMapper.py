# $Id: NameMapper.py,v 1.2 2002/10/20 02:22:38 chalky Exp $
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
# $Log: NameMapper.py,v $
# Revision 1.2  2002/10/20 02:22:38  chalky
# Fix reference to verbose flag
#
# Revision 1.1  2002/08/23 04:37:26  chalky
# Huge refactoring of Linker to make it modular, and use a config system similar
# to the HTML package
#

import string

from Synopsis.Core import AST, Type, Util

from Linker import config, Operation

class NameMapper (Operation, AST.Visitor):
    """This class adds a prefix to all declaration and type names."""

    def visitDeclaration(self, decl):
	"""Changes the name of this declaration and its associated type"""
	# Change the name of the decl
	name = decl.name()
	newname = tuple(config.map_declaration_names + name)
	decl.set_name(newname)
	# Change the name of the associated type
	try:
	    type = config.types[name]
	    del config.types[name]
	    type.set_name(newname)
	    config.types[newname] = type
	except KeyError, msg:
	    if config.verbose: print "Warning: Unable to map name of type:",msg
    def visitGroup(self, node):
	"""Recursively visits declarations under this group/scope/etc"""
	self.visitDeclaration(node)
        for declaration in node.declarations():
	    declaration.accept(self)


    def execute(self, ast):
	if not config.map_declaration_names: return
	declarations = ast.declarations()
	# Map the names of declarations and their types
	for decl in declarations:
	    decl.accept(self)
	# Now we need to put the declarations in actual nested MetaModules
	lang, type = '', config.map_declaration_type
	names = config.map_declaration_names
	for index in range(len(names),0, -1):
	    module = AST.MetaModule(lang, type, names[:index])
	    module.declarations().extend(declarations)
	    declarations[:] = [module]

linkerOperation = NameMapper
