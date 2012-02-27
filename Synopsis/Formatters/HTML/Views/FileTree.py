#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Processor import Parameter
from Tree import Tree
from Synopsis.Formatters.HTML.Tags import *

import os

class FileTree(Tree):
    """Create a javascript-enabled file tree."""

    link_to_views = Parameter(False, 'some docs...')

    def filename(self):

        if self.main:
            return self.directory_layout.index()
        else:
            return self.directory_layout.special('FileTree')

    def title(self):

        return 'File Tree'

    def root(self):

        return self.filename(), self.title()

    def process(self):

        # Start the file
        self.start_file()
        self.write_navigation_bar()
        self.begin_tree()
        # recursively visit all nodes
        self.process_node(self.processor.file_tree)
        self.end_tree()
        self.end_file()

    def process_node(self, node):

        def node_cmp(a, b):
            a_leaf = hasattr(a, 'declarations')
            b_leaf = hasattr(b, 'declarations')
            if a_leaf != b_leaf:
                return cmp(b_leaf, a_leaf)
            return cmp(a.path[-1].upper(), b.path[-1].upper())

        dirname, filename = os.path.split(node.path)
        if hasattr(node, 'declarations'):
            # Leaf node
            text = href(self.directory_layout.file_index(node.path),
                        filename, target='detail')
            self.write_leaf(text)
            return
        # Non-leaf node
        children = node.children[:]
        children.sort(node_cmp)
        if node.path:
            self.write_node_start(filename + os.sep)
        if len(children):
            for child in children:
                # self.write('<div class="files">')
                self.process_node(child)
                # self.write('</div>')
        if node.path:
            self.write_node_end()
