# $Id: ModuleListingJS.py,v 1.2 2001/02/05 07:58:39 chalky Exp $
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
# $Log: ModuleListingJS.py,v $
# Revision 1.2  2001/02/05 07:58:39  chalky
# Cleaned up image copying for *JS. Added synopsis logo to ScopePages.
#
# Revision 1.1  2001/02/05 05:31:52  chalky
# Initial commit. Image finding is a hack
#
# Revision 1.2  2001/02/01 15:23:24  chalky
# Copywritten brown paper bag edition.
#
#

# System modules
import os

# Synopsis modules
from Synopsis.Core import AST, Util

# HTML modules
import Page
import core
from core import config
from Tags import *

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
# The HTML for one image. %s's are the same id string
img_html = """<a onClick="toggle('%s')" href="javascript:void(0);"
><img id="%s_img" border=0 src="tree_open.png"
></a>"""

class ModuleListingJS(Page.Page): 
    """Create an index of all modules with JS. The JS allows the user to
    expand/collapse sections of the tree!"""
    def __init__(self, manager):
	Page.Page.__init__(self, manager)
	self.__id = 0
	self.__filename = config.files.nameOfSpecial('module_listing')
	link = href(self.__filename, 'Modules', target="contents")
	manager.addRootPage('Modules', link, 2)
	config.set_contents_page(self.__filename)

    def getId(self):
	self.__id = self.__id + 1
	return "tree%d"%self.__id

    def process(self, start):
	"""Create a page with an index of all modules"""
	# Copy images across
	share = os.path.split(AST.__file__)[0]+"/../share"
	config.files.copyFile(share+'/syn-down.png', 'tree_open.png')
	config.files.copyFile(share+'/syn-right.png', 'tree_close.png')
	# Creare the file
	self.startFile(self.__filename, "Module Index", headextra=top_js%('tree_open.png', 'tree_close.png'))
	self.write(self.manager.formatRoots('Modules', 2))
	self.write('<hr>')
	self.indexModule(start, start.name())
	self.endFile()

    def _child_filter(self, child):
	return isinstance(child, AST.Module)
    def indexModule(self, ns, rel_scope):
	"Write a link for this module and recursively visit child modules."
	my_scope = ns.name()
	# Find children
	config.sorter.set_scope(ns)
	config.sorter.sort_sections()
	children = config.sorter.children()
	children = filter(self._child_filter, children)
	# Print link to this module
	id = self.getId();
	button = len(children) and img_html%(id,id) or ""
	name = Util.ccolonName(my_scope, rel_scope) or "Global Namespace"
	link = config.files.nameOfModuleIndex(ns.name())
	self.write(button + href(link, name, target='index') + '<br>')
	# Add children
	if not len(children): return
	self.write('<div id="%s">'%id)
	for child in children:
	    self.write('<div class="module-section">')
	    self.indexModule(child, my_scope)
	    self.write('</div>')
	self.write('</div>')

htmlPageClass = ModuleListingJS

