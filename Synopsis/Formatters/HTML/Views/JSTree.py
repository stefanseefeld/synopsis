# $Id: JSTree.py,v 1.1 2001/02/06 05:12:46 chalky Exp $
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
# Revision 1.1  2001/02/06 05:12:46  chalky
# Added JSTree class and FileTreeJS and modified ModuleListingJS to use JSTree
#
#

import Page
from core import config

#The javascript that goes up the top
top_js = """<script language="JavaScript1.2"><!--
showImage = new Image(); hideImage = new Image();
function init_tree(show_src, hide_src) {
    showImage.src = show_src; hideImage.src = hide_src;
}
function toggle(id) {
    section = document.getElementById(id);
    image = document.getElementById(id+"_img");
    if (section.style.display == "none") {
	section.style.display = "";
	image.src = showImage.src;
    } else {
	section.style.display = "none";
	image.src = hideImage.src;
    }
}
init_tree("%s", "%s");
--></script>
"""
# The HTML for one image. %s's are 2x the same id string and the image
img_html = """<a onClick="toggle('%s')" href="javascript:void(0);"
><img id="%s_img" border=0 src="%s"
></a>"""

class JSTree (Page.Page):
    """Page that makes Javascript trees. The trees have expanding and
    collapsing nodes. call js_init() with the button images and default
    open/close policy during process"""

    def __init__(self, manager):
	Page.Page.__init__(self, manager)
	self.__id = 0
	self.__open_img = ''
	self.__close_img = ''
	
    def getId(self):
	self.__id = self.__id + 1
	return "tree%d"%self.__id

    def js_init(self, open_img, close_img, base, default_open=1):
	"""Initialise the JSTree page. This method copies the files to the
	output directory and stores the values given.
	@param open_img	     filename of original open image
	@param close_img     filename of original close image
	@param base filename with a %s for open/close images, eg "tree_%s.png"
	@param default_open  true if sections are open by default
	"""
	self.__open_img = open_img
	self.__close_img = close_img
	self.__def_open = default_open
	self.__base_open = base%'open'
	self.__base_close = base%'close'
	# Copy images across
	config.files.copyFile(open_img, self.__base_open)
	config.files.copyFile(close_img, self.__base_close)

    def startFile(self, filename, title):
	"""Overrides startFile to add the javascript"""
	Page.Page.startFile(self, filename, title, headextra=top_js%(
	    self.__base_open, self.__base_close
	))

    def formatButton(self):
	"""Returns the HTML for the open/close button. This method will get a
	new ID, and should be called before startSection which doesn't."""
	id = self.getId();
	return img_html%(id, id,
	    self.__def_open and self.__base_open or self.__base_close
	)

    def startSection(self):
	"""Starts an collapsible section. You must call this *after*
	formatButton, and before endSection."""
	id = self.__id
	if self.__def_open:
	    self.write('<div id="tree%d">'%id)
	else:
	    self.write('<div id="tree%d" style="display:none;">'%id)

    def endSection(self):
	self.write('</div>')


