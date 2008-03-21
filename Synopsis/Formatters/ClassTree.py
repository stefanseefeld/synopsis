#
# Copyright (C) 2000 Stefan Seefeld
# Copyright (C) 2000 Stephen Davies
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

"""Contains the utility class ClassTree, for creating inheritance trees."""

from Synopsis import ASG


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
    
    def add_inheritance(self, supername, subname):
        """Adds an edge to the graph. Supername and subname are the scoped
        names of the two classes involved in the edge, and are copied before
        being stored."""
        self.add_class(supername)
        if not self.__subclasses.has_key(supername):
            subs = self.__subclasses[supername] = []
        else:
            subs = self.__subclasses[supername]
        if subname not in subs:
            subs.append(subname)
            subs.sort()
        if not self.__superclasses.has_key(subname):
            sups = self.__superclasses[subname] = []
        else:
            sups = self.__superclasses[subname]
        if supername not in sups:
            sups.append(supername)
            sups.sort()
    
    def subclasses(self, classname):
        """Returns a sorted list of all classes derived from the given
        class"""
        
        if self.__subclasses.has_key(classname):
            return self.__subclasses[classname]
        else:
            return []

    def superclasses(self, classname):
        """Returns a sorted list of all classes the given class derives
        from. The classes are returned as scoped names, which you may use to
        lookup the class declarations in the 'types' dictionary if you need
        to."""

        if self.__superclasses.has_key(classname):
            return self.__superclasses[classname]
        else:
            return []
    
    def classes(self):
        """Returns a sorted list of all class names"""

        return self.__classes

    def add_class(self, name):
        """Adds a class to the list of classes by name"""

        if name not in self.__classes:
            self.__classes.append(name)
            self.__classes.sort()
    
    def roots(self):
        """Returns a list of classes that have no superclasses"""

        return [c for c in self.classes() if c not in self.__superclasses]

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

        return [c for c in graph if c not in self.__subclasses]

    def _make_graphs(self):

        processed = {}
        for root in self.roots():
            if root in processed:
                continue
            bucket = []
            self.__buckets.append(bucket)
            classes = [root]
            while len(classes):
                name = classes.pop()
                if name in processed:
                    continue
                # Add to bucket
                bucket.append(name)
                processed[name] = True
                # Add super- and sub-classes
                classes.extend(self.superclasses(name))
                classes.extend(self.subclasses(name))
    #
    # ASG Visitor
    #

    def visit_scope(self, scope):
        for d in scope.declarations:
            d.accept(self)

    def visit_class(self, class_):
        """Adds this class and all edges to the lists"""

        self.add_class(class_.name)
        for inheritance in class_.parents:
            parent = inheritance.parent
            if hasattr(parent, 'declaration'):	
                self.add_inheritance(parent.declaration.name, class_.name)
            elif isinstance(parent, ASG.ParametrizedTypeId) and parent.template:
                self.add_inheritance(parent.template.name, class_.name)
            elif isinstance(parent, ASG.UnknownTypeId):
                self.add_inheritance(parent.link, class_.name)
        for d in class_.declarations:
            d.accept(self)
