#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Processor import Parameter
from Synopsis.Formatters.HTML.View import View
from Synopsis.Formatters.HTML.Tags import *

#The javascript that goes up the top
top_js = """<script type="text/javascript" language="JavaScript1.2"><!--
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
img_html = """<a href="javascript:toggle('%s');">%s</a>"""

class Tree(View):
   """View that makes Javascript trees. The trees have expanding and
   collapsing nodes. call js_init() with the button images and default
   open/close policy during process"""

   def register(self, processor):

      View.register(self, processor)
      self.__id = 0
      self.__open_img = ''
      self.__close_img = ''
      self.__leaf_img = ''
	
   def get_id(self):

      self.__id = self.__id + 1
      return "tree%d"%self.__id

   def js_init(self, open_img, close_img, leaf_img, base, default_open=1):
      """Initialise the Tree view. This method copies the files to the
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
      self.processor.file_layout.copy_file(open_img, self.__base_open)
      self.processor.file_layout.copy_file(close_img, self.__base_close)
      self.processor.file_layout.copy_file(leaf_img, self.__base_leaf)

   def start_file(self):
      """Overrides start_file to add the javascript"""

      View.start_file(self, headextra=top_js%(self.__base_open, self.__base_close))
      
   def format_image(self, id, filename, alt_text=""):
      """Returns the image element for the given image"""
      # todo: resolve directory path
      id = id and 'id="%s" '%id or ''
      return '<img %sborder="0" src="%s" alt="%s" />'%(id, filename, alt_text)

   def write_leaf(self, item_text):
      """Write a leaf node to the output at the current tree level."""
      img = self.format_image(None, self.__base_leaf, "leaf")
      self.write(div('module-section', img + item_text) + '\n')

   def write_node_start(self, item_text):
      """Write a non-leaf node to the output at the current tree level, and
      start a new level."""
      # Get a unique id for this node
      id = self.get_id()
      # Get the image for this node
      if self.__def_open: img = self.format_image(id, self.__base_open, 'node')
      else: img = self.format_image(id+"_img", self.__base_close, 'node')
      # Get the scripted link for the image
      img_link = img_html%(id, img)
      # Write the item
      self.write('<div class="module-section">%s%s'%(img_link, item_text))
      # Start the (collapsible) section for the child nodes
      if self.__def_open:
         self.write('<div id="%s">\n'%id)
      else:
         self.write('<div id="%s" style="display:none;">\n'%id)

   def write_node_end(self):
      """Finish a non-leaf node, and close the current tree level."""
      # Close the collapsible div, and the node's div
      self.write('</div>\n</div>\n')


