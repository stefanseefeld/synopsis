#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis import AST, Util
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

        # Creare the file
        self.start_file()
        self.write_navigation_bar()
        # FIXME: see HTML.Formatter
        module = self.processor.ast.declarations()[0]
        self.index_module(module, module.name())
        self.end_tree()
        self.end_file()

    def _child_filter(self, child):

        return isinstance(child, AST.Module)

    def _link_href(self, module):

        return self.directory_layout.module_index(module.name())

    def get_children(self, decl):

        try: return self._children_cache[decl]
        except KeyError: pass
        sorter = self.processor.sorter
        sorter.set_scope(decl)
        sorter.sort_sections()
        children = sorter.children()
        children = filter(self._child_filter, children)
        self._children_cache[decl] = children
        return children

    def index_module(self, module, scope):
        """Write a link for this module and recursively visit child modules."""

        name = module.name()
        # Find children, and sort so that compound children (packages) go first
        children = self.get_children(module)
        children.sort(lambda a,b,g=self.get_children:
                      cmp(len(g(b)),len(g(a))))
        # Print link to this module
        label = Util.ccolonName(name, scope) or 'Global Module'
        link = self._link_href(module)
        text = href(link, label, target='detail')
        if not len(children):
            self.write_leaf(text)
        else:
            self.write_node_start(text)
            # Add children
            for child in children:
                self.index_module(child, name)
            self.write_node_end()

