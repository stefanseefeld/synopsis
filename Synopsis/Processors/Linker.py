# $Id: Linker.py,v 1.5 2002/12/10 07:28:49 chalky Exp $
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
# $Log: Linker.py,v $
# Revision 1.5  2002/12/10 07:28:49  chalky
# Unduplicate the list of declarations for each file
#
# Revision 1.4  2002/10/28 16:30:05  chalky
# Trying to fix some bugs in the unduplication/stripping stages. Needs more work
#
# Revision 1.3  2002/10/27 12:23:27  chalky
# Fix crash bug
#
# Revision 1.2  2002/09/20 10:35:12  chalky
# Better handling of namespaces with repeated comments
#
# Revision 1.1  2002/08/23 04:37:26  chalky
# Huge refactoring of Linker to make it modular, and use a config system similar
# to the HTML package
#

import string

from Synopsis.Core import AST, Type, Util

from Linker import config, Operation

class Unduplicator(AST.Visitor, Type.Visitor):
    """Visitor that removes duplicate declarations"""
    def __init__(self):
        self.__global = AST.MetaModule("", "",[])
	self.__scopes = [self.__global]
	global_dict = {}
	self.__dict_map = { id(self.__global) : global_dict }
	self.__dicts = [ global_dict ]
    def execute(self, ast):
	declarations = ast.declarations()
	for decl in declarations:
	    decl.accept(self)
	declarations[:] = self.__global.declarations()
	for file in ast.files().values():
	    self.visitSourceFile(file)
    def lookup(self, name):
        """look whether the current scope already contains a declaration with the given name"""
	if self.__dicts[-1].has_key(name):
	    return self.__dicts[-1][name]
	#for decl in self.__scopes[-1].declarations():
	#    if hasattr(decl, 'name') and decl.name() == name:
	#	return decl
	return None
    def append(self, declaration):
        """append declaration to the current scope"""
	self.__scopes[-1].declarations().append(declaration)
	self.__dicts[-1][declaration.name()] = declaration
    def push(self, scope):
        """push new scope on the stack"""
	self.__scopes.append(scope)
	dict = self.__dict_map.setdefault(id(scope), {})
	self.__dicts.append(dict)
    def pop(self):
        """restore the previous scope"""
	del self.__scopes[-1]
	del self.__dicts[-1]
    def top(self):
	return self.__scopes[-1]
    def top_dict(self):
	return self.__dicts[-1]

    def linkType(self, type):
	"Returns the same or new proxy type"
	self.__type = type
	if type is not None: type.accept(self)
	return self.__type

    #################### Type Visitor ###########################################

    def visitBaseType(self, type):
	if config.types.has_key(type.name()):
	    self.__type = config.types[type.name()]

    def visitUnknown(self, type):
	if config.types.has_key(type.name()):
	    self.__type = config.types[type.name()]

    def visitDeclared(self, type):
	if config.types.has_key(type.name()):
	    self.__type = config.types[type.name()]
	else:
	    print "Couldn't find declared type:",type.name()

    def visitTemplate(self, type):
	# Should be a Declared with the same name
	if not config.types.has_key(type.name()):
	    return
	declared = config.types[type.name()]
	if not isinstance(declared, Type.Declared):
	    print "Warning: template declaration was not a declaration:",type.name(),declared.__class__.__name__
	    return
	decl = declared.declaration()
	if not hasattr(decl, 'template'):
	    #print "Warning: non-class/function template",type.name(), decl.__class__.__name__
	    return
	if decl.template():
	    self.__type = decl.template()
	else:
	    print "Warning: template type disappeared:",type.name()

    def visitModifier(self, type):
	alias = self.linkType(type.alias())
	if alias is not type.alias():
	    type.set_alias(alias)
	self.__type = type

    def visitArray(self, type):
	alias = self.linkType(type.alias())
	if alias is not type.alias():
	    type.set_alias(alias)
	self.__type = type

    def visitParametrized(self, type):
	templ = self.linkType(type.template())
	if templ is not type.template():
	    type.set_template(templ)
	params = tuple(type.parameters())
	del type.parameters()[:]
	for param in params:
	    type.parameters().append(self.linkType(param))
	self.__type = type

    def visitFunctionType(self, type):
	ret = self.linkType(type.returnType())
	if ret is not type.returnType():
	    type.set_returnType(ret)
	params = tuple(type.parameters())
	del type.parameters()[:]
	for param in params:
	    type.parameters().append(self.linkType(param))
	self.__type = type

    #################### AST Visitor ############################################

    def visitSourceFile(self, file):
	"""Resolves any duplicates in the list of declarations from this
	file"""
	types = config.types

	# Clear the list and refill it
	old_decls = list(file.declarations())
	new_decls = file.declarations()
	new_decls[:] = []

	for decl in old_decls:
	    # Try to find a replacement declaration
	    if types.has_key(decl.name()):
		declared = types[decl.name()]
		if isinstance(type, Type.Declared):
		    decl = declared.declaration()
	    new_decls.append(decl)
	
	# TODO: includes.

    def visitModule(self, module):
        #hmm, we assume that the parent node is a MetaModule. Can that ever fail ?
	metamodule = self.lookup(module.name())
	if metamodule is None:
	    metamodule = AST.MetaModule(module.language(), module.type(),module.name())
	    self.append(metamodule)
	metamodule.module_declarations().append(module)
	self.merge_comments(metamodule.comments(), module.comments())
	self.push(metamodule)
	decls = tuple(module.declarations())
	del module.declarations()[:]
        for decl in decls: decl.accept(self)
	self.pop()

    def merge_comments(self, dest, src):
	"""Merges the src comments into dest. Merge is just an append, unless
	src already exists inside dest!"""
	texter = lambda x: x.text()
	dest_str = map(texter, dest)
	src_str = map(texter, src)
	if dest_str[-len(src):] == src_str: return
	dest.extend(src)

    def visitMetaModule(self, module):        
        #hmm, we assume that the parent node is a MetaModule. Can that ever fail ?
	metamodule = self.lookup(module.name())
	if metamodule is None or not isinstance(metamodule, AST.MetaModule):
	    metamodule = AST.MetaModule(module.language(), module.type(),module.name())
	    self.append(metamodule)
	metamodule.module_declarations().extend(module.module_declarations())
	metamodule.comments().extend(module.comments())
	self.push(metamodule)
	decls = tuple(module.declarations())
	del module.declarations()[:]
        for decl in decls: decl.accept(self)
	self.pop()

    def addDeclaration(self, decl):
	"""Adds a declaration to the current (top) scope.
	If there is already a Forward declaration, then this replaces it
	unless this is also a Forward.
	"""
	name = decl.name()
	dict = self.__dicts[-1]
	decls = self.top().declarations()
	if dict.has_key(name):
	    prev = dict[name]
	    if not isinstance(prev, AST.Forward):
		return
	    if not isinstance(decl, AST.Forward):
		decls.remove(prev)
		decls.append(decl)
		dict[name] = decl # overwrite prev
	    return
	decls.append(decl)
	dict[name] = decl

    def visitNamed(self, decl):
	name = decl.name()
	if self.lookup(decl.name()): return
	self.addDeclaration(decl)

    visitDeclaration = addDeclaration
    visitForward = addDeclaration
    visitEnum = addDeclaration

    def visitFunction(self, func):
	if not isinstance(self.top(), AST.Class):
	    for decl in self.top().declarations():
		if not isinstance(decl, AST.Function): continue
		if func.name() == decl.name():
		    return
	ret = self.linkType(func.returnType())
	if ret is not func.returnType():
	    func.set_returnType(ret)
	for param in func.parameters():
	    self.visitParameter(param)
	self.top().declarations().append(func)

    visitOperation = visitFunction

    def visitVariable(self, var):
	#if not scopedNameOkay(var.name()): return
	vt = self.linkType(var.vtype())
	if vt is not var.vtype():
	    var.set_vtype(vt)
	self.addDeclaration(var)

    def visitTypedef(self, tdef):
	alias = self.linkType(tdef.alias())
	if alias is not tdef.alias():
	    tdef.set_alias(alias)
	self.addDeclaration(tdef)

    def visitClass(self, clas):
	name = clas.name()
	prev = self.lookup(name)
	if prev:
	    if isinstance(prev, AST.Forward):
		# Forward declaration, replace it
		self.top().declarations().remove(prev)
		del self.top_dict()[name]
	    elif isinstance(prev, AST.Class):
		# Previous class. Would ignore duplicate but clas may have
		# class declarations that prev doesn't. (forward declared
		# nested -- see ThreadData.hh for example)
		self.push(prev)
		for decl in clas.declarations():
		    if isinstance(decl, AST.Class):
			decl.accept(self)
		self.pop()
		return
	    else:
		print "Unduplicator.visitClass: clas=%s, prev=%s"%(clas.name(), prev)
		if hasattr(prev, 'name'): print "prev.name=%s"%(prev.name())
		raise TypeError, "Previous class declaration not a class"
	self.addDeclaration(clas)
	for parent in clas.parents():
	    parent.accept(self)
	self.push(clas)
	decls = tuple(clas.declarations())
	del clas.declarations()[:]
        for decl in decls: decl.accept(self)
	self.pop()

    def visitInheritance(self, parent):
	type = parent.parent()
	if isinstance(type, Type.Declared) or isinstance(type, Type.Unknown):
	    ltype = self.linkType(type)
	    if ltype is not type:
		parent.set_parent(ltype)
	elif isinstance(type, Type.Parametrized):
	    ltype = self.linkType(type.template())
	    if ltype is not type.template():
		# Must find a Type.Template from it
		if not isinstance(ltype, Type.Declared):
		    # Error
		    return
		decl = ltype.declaration()
		if isinstance(decl, AST.Class):
		    type.set_template(decl.template())
	else:
	    # Unknown type in class inheritance
	    pass

    def visitParameter(self, param):
	type = self.linkType(param.type())
	if type is not param.type():
	    param.set_type(type)

    def visitConst(self, const):
	ct = self.linkType(const.ctype())
	if ct is not const.ctype():
	    const.set_ctype(ct)
	self.addDeclaration(const)

linkerOperation = Unduplicator
