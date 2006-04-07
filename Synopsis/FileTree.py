#
# Copyright (C) 2000 Stefan Seefeld
# Copyright (C) 2000 Stephen Davies
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

import os.path

class FileTree:
   """Maintains a tree of directories and files.
    
   The FileTree is constructed using the SourceFiles in the AST. Each
   SourceFile has a list of declarations in it already. The FileTree object
   organises these lists into a tree structure, where each node is a
   directory or a source file."""

   class Node:
      """Base class for directories and files in the tree.
      @attr path the path of this node as a string.
      @attr filename the name of the node, i.e. the last element of the
      path string
      """
      def __init__(self, path, filename):
         self.path = path
         self.filename = filename
    
   class Directory(Node):
      """FileTree node for directories.
      @attr path the path of this node as a string.
      @attr filename the name of the directory, i.e. the last element of the
      path string
      @attr children the children of this directory, each Directory or File
      objects.
      """
      def __init__(self, path, filename):
         FileTree.Node.__init__(self, path, filename)
         self.children = []
    
   class File(Node):
      """FileTree node for files.
      @attr path the path of this node as a string.
      @attr filename the name of the file, i.e. the last element of the
      path string
      @attr decls the list of declarations in this file.
      """
      def __init__(self, path, filename, decls):
         FileTree.Node.__init__(self, path, filename)
         self.decls = decls
    
   def __init__(self):
      self.__dirs = None
      self.__root = None
      self.__ast = None
      
   def __add_dir(self, path):
      """Recursively add a directory to the tree"""
      parent_dir, filename = os.path.split(path)
      if parent_dir == path:
         # The root directory is added below the root node
         # This is in case absolute filenames are mixed with relative ones
         parent = self.__root
         filename = '/'
      else:
         parent_dir, filename = os.path.split(path)
         if parent_dir:
            if self.__dirs.has_key(parent_dir):
               parent = self.__dirs[parent_dir]
            else:
               parent = self.__add_dir(parent_dir)
         else:
            # No parent means an relative name like 'home/foo/bar'
            parent = self.__root
      new_dir = FileTree.Directory(path, filename)
      self.__dirs[path] = new_dir
      parent.children.append(new_dir)
      return new_dir

   def __add_file(self, file, decls):
      """Recursively add a file to the tree"""
      # Find the parent Directory object
      parent_dir, filename = os.path.split(file)
      if self.__dirs.has_key(parent_dir):
         parent = self.__dirs[parent_dir]
      else:
         parent = self.__add_dir(parent_dir)
      new_file = FileTree.File(file, filename, decls)
      parent.children.append(new_file)
    
   def set_ast(self, ast):
      """Sets the AST to use and builds the tree of Nodes"""
      self.__ast = ast
      if ast is None:
         self.__dirs = None
         self.__root = None
         return
      # Initialise dictionary and root node
      self.__dirs = {}
      self.__root = FileTree.Directory('', '')
      # Add each file to the hierarchy
      for filename, file in ast.files().items():
         if file.annotations['primary']:
            self.__add_file(filename, file.declarations)
      # Clean up dict
      self.__dirs = None
	    
   def root(self):
      """Returns the root node in the file tree, which is a Directory
      object. The root node is created by set_ast()."""
      return self.__root
 

