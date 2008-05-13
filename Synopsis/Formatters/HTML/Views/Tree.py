#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis import config
from Synopsis.Processor import Parameter
from Synopsis.Formatters.HTML.View import View
from Synopsis.Formatters.HTML.Tags import *
import os

class Tree(View):
    """View that makes Javascript trees. The trees have expanding and
    collapsing nodes. call js_init() with the button images and default
    open/close policy during process"""

    def register(self, frame):

        super(Tree, self).register(frame)
        self.__id = 0
        share = config.datadir
        open_img = os.path.join(share, 'syn-down.png')
        close_img = os.path.join(share, 'syn-right.png')
        leaf_img = os.path.join(share, 'syn-dot.png')
        js = os.path.join(share, 'html.js')
        self.tree_open = 'tree_open.png'
        self.tree_close = 'tree_close.png'
        self.tree_leaf = 'tree_leaf.png'
        # Copy resources across
        self.directory_layout.copy_file(open_img, self.tree_open)
        self.directory_layout.copy_file(open_img, self.tree_open)
        self.directory_layout.copy_file(close_img, self.tree_close)
        self.directory_layout.copy_file(leaf_img, self.tree_leaf)
        self.directory_layout.copy_file(js, 'synopsis.js')

    def get_id(self):
        
        self.__id += 1
        return 'tree%d'%self.__id

    def start_file(self):

        js = '<script type="text/javascript" src="synopsis.js"></script>'
        View.start_file(self, headextra=js)
      
    def write_leaf(self, text):
        """Write a leaf node to the output at the current tree level."""
        i = img(src=self.tree_leaf, alt='leaf')
        self.write(div('module-section', i + text) + '\n')

    def write_node_start(self, text):
        """Write a non-leaf node to the output at the current tree level, and
        start a new level."""
        # Get a unique id for this node
        id = self.get_id()
        # Get the image for this node
        node = img(id='%s_img'%id, src=self.tree_close, alt='node', border='0')
        # Get the scripted link for the image
        link = href("javascript:toggle('%s');"%id, node)
        # Write the item
        self.write('<div class="module-section">%s%s'%(link, text))
        # Start the (collapsible) section for the child nodes
        self.write('<div id="%s" style="display:none;">\n'%id)

    def write_node_end(self):
        """Finish a non-leaf node, and close the current tree level."""

        self.write('</div>\n</div>\n')

    def end_tree(self):
        """Writes the end of the tree."""

        js_end = """<script type="text/javascript" language="JavaScript1.2"><!--
        tree_max_node = %d; // --></script>"""

        self.write(js_end%self.__id)

        self.write(div('tree-navigation',
                       href('javascript:open_all()', 'Open All') +
                       href('javascript:close_all()', 'Close All')))

