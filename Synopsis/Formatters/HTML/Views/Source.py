# $Id: Source.py,v 1.2 2003/02/01 23:59:32 chalky Exp $
#
# This file is a part of Synopsis.
# Copyright (C) 2000-2003 Stephen Davies
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
# $Log: Source.py,v $
# Revision 1.2  2003/02/01 23:59:32  chalky
# Use full_filename() for file source so don't need file_path any more to
# find the original source files.
#
# Revision 1.1  2003/01/16 12:46:46  chalky
# Renamed FilePages to FileSource, FileTree to FileListing. Added FileIndexer
# (used to be part of FileTree) and FileDetails.
#
# Revision 1.22  2002/12/09 04:00:58  chalky
# Added multiple file support to parsers, changed AST datastructure to handle
# new information, added a demo to demo/C++. AST Declarations now have a
# reference to a SourceFile (which includes a filename) instead of a filename.
#
# Revision 1.21  2002/11/13 02:29:24  chalky
# Support exclude_glob option to exclude files from listings. Remove debug info.
#
# Revision 1.20  2002/11/13 01:01:49  chalky
# Improvements to links when using the Nested file layout
#
# Revision 1.19  2002/11/02 06:37:37  chalky
# Allow non-frames output, some refactoring of page layout, new modules.
#
# Revision 1.18  2002/11/01 03:39:20  chalky
# Cleaning up HTML after using 'htmltidy'
#
# Revision 1.17  2002/10/29 15:00:32  chalky
# Put toc in the right place
#
# Revision 1.16  2002/10/29 12:43:56  chalky
# Added flexible TOC support to link to things other than ScopePages
#
# Revision 1.15  2002/10/26 04:16:49  chalky
# Fix bug which didn't like empty paths (when path doesn't start at root
# directory)
#
# Revision 1.14  2002/07/19 14:26:32  chalky
# Revert prefix in FileLayout but keep relative referencing elsewhere.
#
# Revision 1.12  2002/01/13 09:44:51  chalky
# Allow formatted source in GUI
#

# System modules
import time, os

# Synopsis modules
from Synopsis.Core import AST, Util

# HTML modules
import Page
import core
import ASTFormatter
from core import config
from Tags import *

# Link module
link = None
try:
    link = Util._import("Synopsis.Parser.C++.link")
except ImportError:
    print "Warning: unable to import link module. Continuing..."

class FileSource (Page.Page):
    """A module for creating a page for each file with hyperlinked source"""
    def __init__(self, manager):
	Page.Page.__init__(self, manager)
	# Try old name first for backwards compatibility
	if hasattr(config.obj, 'FilePages'): myconfig = config.obj.FilePages
	else: myconfig = config.obj.FileSource
	self.__linkpath = myconfig.links_path
	self.__toclist = myconfig.toc_files
	self.__scope = myconfig.scope
	# We will NOT be in the Manual directory  TODO - use FileLayout for this
	self.__toclist = map(lambda x: ''+x, self.__toclist)
	if hasattr(myconfig, 'use_toc'):
	    self.__tocfrom = myconfig.use_toc
	else:
	    self.__tocfrom = config.default_toc

    def filename(self):
        """since FileSource generates a whole file hierarchy, this method returns the current filename,
        which may change over the lifetime of this object"""
        return self.__filename
    def title(self):
        """since FileSource generates a while file hierarchy, this method returns the current title,
        which may change over the lifetime of this object"""
        return self.__title

    def process(self, start):
	"""Creates a page for every file"""
	# Get the TOC
	toc = self.manager.getPage(self.__tocfrom).get_toc(start)
	tocfile = config.files.nameOfSpecial('FileSourceInputTOC')
	tocfile = os.path.join(config.basename, tocfile)
	toc.store(tocfile)
	self.__toclist.append(tocfile)
	# create a page for each main file
	for file in config.ast.files().values():
	    if file.is_main():
		self.process_node(file)
	
	os.unlink(tocfile)

    def register_filenames(self, start):
	"""Registers a page for every source file"""
	for file in config.ast.files().values():
	    if file.is_main():
		filename = file.filename()
		filename = os.path.join(config.base_dir, filename)
		filename = config.files.nameOfFileSource(filename)
		#print "Registering",filename
		self.manager.register_filename(filename, self, file)
	     
    def process_node(self, file):
	"""Creates a page for the given file"""

	# Start page
	toc = config.toc
	filename = file.filename()
	filename = os.path.join(config.base_dir, filename)
	self.__filename = config.files.nameOfFileSource(filename)
	#name = list(node.path)
	#while len(name) and name[0] == '..': del name[0]
        #source = string.join(name, os.sep)
	source = file.filename()
	self.__title = source

	# Massage toc list to prefix '../../.....' to any relative entry.
	prefix = rel(self.filename(), '')
	toc_to_change = config.toc_out
	toclist = list(self.__toclist)
	for index in range(len(toclist)):
	    if '|' not in toclist[index]:
		toclist[index] = toclist[index]+'|'+prefix

	self.start_file()
	self.write(self.manager.formatHeader(self.filename()))
	self.write('File: '+entity('b', self.__title))

	if not link:
	    # No link module..
	    self.write('link module for highlighting source unavailable')
	    try:
		self.write(open(file.full_filename(),'r').read())
	    except IOError, e:
		self.write("An error occurred:"+ str(e))
	else:
	    self.write('<br><div class="file-all">\n')
	    # Call link module
	    f_out = os.path.join(config.basename, self.__filename) + '-temp'
	    f_in = file.full_filename()
	    f_link = self.__linkpath%source
	    #print "file: %s    link: %s    out: %s"%(f_in, f_link, f_out)
	    try:
		link.link(toclist, f_in, f_out, f_link, self.__scope) #, config.types)
	    except link.error, msg:
		print "An error occurred:",msg
	    try:
		self.write(open(f_out,'r').read())
		os.unlink(f_out)
	    except IOError, e:
		self.write("An error occurred:"+ str(e))
	    self.write("</div>")

	self.end_file()
	
htmlPageClass = FileSource
