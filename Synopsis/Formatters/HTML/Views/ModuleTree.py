#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis import ASG
from Synopsis.Formatters.HTML.Tags import *
from Tree import Tree

class ModuleTree(Tree):
    """Create a javascript-enabled module tree."""

    def register(self, frame):

        super(ModuleTree, self).register(frame)
        self._children_cache = {}

    def filename(self):

        if self.main:
            return self.directory_layout.index()
        else:
            return self.directory_layout.special('ModuleTree')

    def title(self):

        return 'Module Tree'

    def root(self):

        return self.filename(), self.title()

    def process(self):

        self.start_file()
        self.write_navigation_bar()
        module = self.processor.root
        self.index_module(module, module.name)
        self.end_tree()
        self.end_file()

    def _link_href(self, module):

        return self.directory_layout.module_index(module.name)

    def get_children(self, decl):

        try: return self._children_cache[decl]
        except KeyError: pass
        children = [c for c in decl.declarations if isinstance(c, ASG.Module)]
        self._children_cache[decl] = children
        return children

    def index_module(self, module, qname):
        """Write a link for this module and recursively visit child modules."""

        # Find children, and sort so that compound children (packages) go first
        children = self.get_children(module)
        children.sort(lambda a,b,g=self.get_children:
                      cmp(len(g(b)),len(g(a))))
        # Print link to this module
        if module.name:
            label = str(qname.prune(module.name))
        else:
            label = 'Global %s'%module.type.capitalize()
        link = self._link_href(module)
        text = href(link, label, target='detail')
        if not len(children):
            self.write_leaf(text)
        else:
            self.write_node_start(text)
            # Add children
            for child in children:
                self.index_module(child, module.name)
            self.write_node_end()

