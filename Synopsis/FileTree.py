# $Id: FileTree.py,v 1.1 2002/10/20 03:11:37 chalky Exp $
#
# This file is a part of Synopsis.
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
# $Log: FileTree.py,v $
# Revision 1.1  2002/10/20 03:11:37  chalky
# Moved FileTree to Core, as it is needed in the GUI
#

class FileTree(AST.Visitor):
    """Maintains a tree of directories and files"""
    def __init__(self):
	"Installs self in config object as 'fileTree'"
	config.fileTree = self
	self.__files = {}
    
    def buildTree(self):
	"Takes the visited info and makes a tree of directories and files"
	# Split filenames into lists of directories
	files = []
	for file in self.__files.keys():
	    files.append(string.split(file, os.sep))
	self.__root = Struct(path=(), children={})
	process_list = [self.__root]
	while len(process_list):
	    # Get node
	    node = process_list[0]
	    del process_list[0]
	    for file in files:
		if len(file) <= len(node.path) or tuple(file[:len(node.path)]) != node.path:
		    continue
		child_path = tuple(file[:len(node.path)+1])
		if node.children.has_key(child_path): continue
		child = Struct(path=child_path, children={})
		node.children[child_path] = child
		if len(child_path) < len(file):
		    process_list.append(child)
		else:
		    fname = string.join(file, os.sep)
		    child.decls = self.__files[fname]
    
    def root(self):
	"""Returns the root node in the file tree.
	Each node is a Struct with the following members:
	 path - tuple of dir or filename (eg: 'Prague','Sys','Thread.hh')
	 children - dict of children by their path tuple
	Additionally, leaf nodes have the attribute:
	 decls - dict of declarations in this file by scoped name
	"""
	return self.__root
    
    ### AST Visitor
    
    def visitAST(self, ast):
	# Reset files dictionary to files listed by the AST object
	for file in ast.filenames():
	    self.__files[file] = {}
	# Visit top level declarations
	for decl in ast.declarations():
	    decl.accept(self)
    def visitDeclaration(self, decl):
	file = decl.file()
	if not file: return #print "Decl",decl,"has no file."
	if not self.__files.has_key(file):
	    self.__files[file] = {}
	# Store the declaration in the per-file dictionary
	self.__files[file][decl.name()] = decl
    def visitForward(self, decl):
	# Don't include (excluded) forward decls in file listing
	pass
    def visitClass(self, scope):
	self.visitDeclaration(scope)
	# Only nested classes may be in different files
	for decl in scope.declarations():
	    if isinstance(decl, AST.Class):
		self.visitClass(decl)
    def visitMetaModule(self, scope):
	for decl in scope.declarations():
	    decl.accept(self)
	
