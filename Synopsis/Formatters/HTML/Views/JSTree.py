# $Id: JSTree.py,v 1.4 2001/07/05 05:39:58 stefan Exp $
#
# This file is a part of Synopsis.
# Copyright (C) 2000, 2001 Stephen Davies
# Copyright (C) 2000, 2001 Stefan Seefeld
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
# $Log: JSTree.py,v $
# Revision 1.4  2001/07/05 05:39:58  stefan
# advanced a lot in the refactoring of the HTML module.
# Page now is a truely polymorphic (abstract) class. Some derived classes
# implement the 'filename()' method as a constant, some return a variable
# dependent on what the current scope is...
#
# Revision 1.3  2001/04/17 13:36:10  chalky
# Slight enhancement to JSTree and derivatives, to use a dot graphic for leaves
#
# Revision 1.2  2001/02/06 18:06:35  chalky
# Added untested compatability with IE4 and Navigator 4
#
# Revision 1.1  2001/02/06 05:12:46  chalky
# Added JSTree class and FileTreeJS and modified ModuleListingJS to use JSTree
#
#

import Page
from core import config
from Tags import *

#The javascript that goes up the top
top_js = """<script language="JavaScript1.2"><!--
var isNav4 = false, isIE4 = false;
if (parseInt(navigator.appVersion.charAt(0)) == 4) {
    isNav4 = (navigator.appName == "Netscape") ? true : false;
} else if (parseInt(navigator.appVersion.charAt(0)) >= 4) {
    isIE4 = (navigator.appName.indexOf("Microsoft") != -1) ? true : false;
}
var isMoz = (isNav4 || isIE4) ? false : true;

showImage = new Image(); hideImage = new Image();
function init_tree(show_src, hide_src) {
    showImage.src = show_src; hideImage.src = hide_src;
}
function toggle(id) {
    if (isMoz) {
	section = document.getElementById(id);
	image = document.getElementById(id+"_img");
	if (section.style.display == "none") {
	    section.style.display = "";
	    image.src = showImage.src;
	} else {
	    section.style.display = "none";
	    image.src = hideImage.src;
	}
    } else if (isIE4) {
	section = document.items[id];
	image = document.images[id+"_img"];
	if (section.style.display == "none") {
	    section.style.display = "";
	    image.src = showImage.src;
	} else {
	    section.style.display = "none";
	    image.src = hideImage.src;
	}
    } else if (isNav4) {
	section = document.items[id];
	image = document.images[id+"_img"];
	if (section.display == "none") {
	    section.style.display = "";
	    image.src = showImage.src;
	} else {
	    section.display = "none";
	    image.src = hideImage.src;
	}
    }
}
init_tree("%s", "%s");
--></script>
"""
# The HTML for one image. %s's are 2x the same id string and the image
img_html = """<a href="javascript:toggle('%s');"
>%s</a>"""

class JSTree (Page.Page):
    """Page that makes Javascript trees. The trees have expanding and
    collapsing nodes. call js_init() with the button images and default
    open/close policy during process"""

    def __init__(self, manager):
	Page.Page.__init__(self, manager)
	self.__id = 0
	self.__open_img = ''
	self.__close_img = ''
	self.__leaf_img = ''
	
    def getId(self):
	self.__id = self.__id + 1
	return "tree%d"%self.__id

    def js_init(self, open_img, close_img, leaf_img, base, default_open=1):
	"""Initialise the JSTree page. This method copies the files to the
	output directory and stores the values given.
	@param open_img	     filename of original open image
	@param close_img     filename of original close image
	@param base filename with a %s for open/close images, eg "tree_%s.png"
	@param default_open  true if sections are open by default
	"""
	self.__open_img = open_img
	self.__close_img = close_img
	self.__leaf_img = leaf_img
	self.__def_open = default_open
	self.__base_open = base%'open'
	self.__base_close = base%'close'
	self.__base_leaf = base%'leaf'
	# Copy images across
	config.files.copyFile(open_img, self.__base_open)
	config.files.copyFile(close_img, self.__base_close)
	config.files.copyFile(leaf_img, self.__base_leaf)

    def start_file(self):
	"""Overrides start_file to add the javascript"""
	Page.Page.start_file(self, headextra=top_js%(self.__base_open, self.__base_close))

    def formatImage(self, id, filename, alt_text=""):
	"""Returns the image element for the given image"""
	# todo: resolve directory path
	id = id and 'id="%s" '%id or ''
	return '<img %sborder=0 src="%s" alt="%s">'%(id, filename, alt_text)

    def writeLeaf(self, item_text):
	"""Write a leaf node to the output at the current tree level."""
	img = self.formatImage(None, self.__base_leaf, "leaf")
	self.write(div('module-section', img+item_text))

    def writeNodeStart(self, item_text):
	"""Write a non-leaf node to the output at the current tree level, and
	start a new level."""
	# Get a unique id for this node
	id = self.getId()
	# Get the image for this node
	if self.__def_open: img = self.formatImage(id, self.__base_open, 'node')
	else: img = self.formatImage(id+"_img", self.__base_close, 'node')
	# Get the scripted link for the image
	img_link = img_html%(id, img)
	# Write the item
	self.write('<div class="module-section">%s%s'%(img_link, item_text))
	# Start the (collapsible) section for the child nodes
	if self.__def_open:
	    self.write('<div id="%s">'%id)
	else:
	    self.write('<div id="%s" style="display:none;">'%id)

    def writeNodeEnd(self):
	"""Finish a non-leaf node, and close the current tree level."""
	# Close the collapsible div, and the node's div
	self.write('</div></div>')


