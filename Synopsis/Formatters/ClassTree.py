#
# Copyright (C) 2000 Stefan Seefeld
# Copyright (C) 2000 Stephen Davies
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

"""Contains the utility class ClassTree, for creating inheritance trees."""

from Synopsis import ASG, Type


def sort(list):
   "Utility func to sort and return the given list"

   list.sort()
   return list

class ClassTree(ASG.Visitor):
   """Maintains a tree of classes directed by inheritance. This object always
   exists in HTML, since it is used for other things such as printing class bases."""
   # TODO - only create if needed (if Info tells us to)

   def __init__(self):
      self.__superclasses = {}
      self.__subclasses = {}
      self.__classes = []
      # Graph stuffs:
      self.__buckets = [] # List of buckets, each a list of classnames
      self.__processed = {} # Map of processed class names
    
   def add_inheritance(self, supername, subname):
      """Adds an edge to the graph. Supername and subname are the scoped
      names of the two classes involved in the edge, and are copied before
      being stored."""

      supername, subname = tuple(supername), tuple(subname)
      self.add_class(supername)
      if not self.__subclasses.has_key(supername):
         subs = self.__subclasses[supername] = []
      else:
         subs = self.__subclasses[supername]
      if subname not in subs:
         subs.append(subname)
      if not self.__superclasses.has_key(subname):
         sups = self.__superclasses[subname] = []
      else:
         sups = self.__superclasses[subname]
      if supername not in sups:
         sups.append(supername)
    
   def subclasses(self, classname):
      """Returns a sorted list of all classes derived from the given
      class"""

      classname = tuple(classname)
      if self.__subclasses.has_key(classname):
         return sort(self.__subclasses[classname])
      return []

   def superclasses(self, classname):
      """Returns a sorted list of all classes the given class derives
      from. The classes are returned as scoped names, which you may use to
      lookup the class declarations in the 'types' dictionary if you need
      to."""

      classname = tuple(classname)
      if self.__superclasses.has_key(classname):
         return sort(self.__superclasses[classname])
      return []
    
   def classes(self):
      """Returns a sorted list of all class names"""

      return sort(self.__classes)

   def add_class(self, name):
      """Adds a class to the list of classes by name"""

      name = tuple(name)
      if name not in self.__classes:
         self.__classes.append(tuple(name))
    
   def _is_root(self, name): return not self.__superclasses.has_key(name)
   def _is_leaf(self, name): return not self.__subclasses.has_key(name)
   def roots(self):
      """Returns a list of classes that have no superclasses"""

      return filter(self._is_root, self.classes())

   #
   # Graph methods
   #
   def graphs(self):
      """Returns a list of graphs. Each graph is just a list of connected
      classes."""

      self._make_graphs()
      return self.__buckets

   def leaves(self, graph):
      """Returns a list of leaves in the given graph. A leaf is a class with
      no subclasses"""

      return filter(self._is_leaf, graph)

   def _make_graphs(self):

      for root in self.roots():
         if self.__processed.has_key(root):
            # Already processed this class
            continue
         bucket = [] ; self.__buckets.append(bucket)
         classes = [root]
         while len(classes):
            name = classes.pop()
            if self.__processed.has_key(name):
               # Already processed this class
               continue
            # Add to bucket
            bucket.append(name)
            self.__processed[name] = None
            # Add super- and sub-classes
            classes.extend( self.superclasses(name) )
            classes.extend( self.subclasses(name) )
   #
   # ASG Visitor
   #

   def visit_scope(self, scope):
      for decl in scope.declarations():
         decl.accept(self)

   def visit_class(self, clas):
      """Adds this class and all edges to the lists"""
      name = clas.name()
      self.add_class(name)
      for inheritance in clas.parents():
         parent = inheritance.parent()
         if hasattr(parent, 'declaration'):	
            self.add_inheritance(parent.declaration().name(), name)
         elif isinstance(parent, Type.Parametrized) and parent.template():
            self.add_inheritance(parent.template().name(), name)
         elif isinstance(parent, Type.Unknown):
            self.add_inheritance(parent.link(), name)
      for decl in clas.declarations():
         decl.accept(self)


