# $Id: DirBrowse.py,v 1.1 2002/11/02 06:37:37 chalky Exp $
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
# $Log: DirBrowse.py,v $
# Revision 1.1  2002/11/02 06:37:37  chalky
# Allow non-frames output, some refactoring of page layout, new modules.
#


# System modules
import os, stat, statcache, os.path, string, time

# Synopsis modules
from Synopsis.Core import AST, Util

# HTML modules
import Page
import core
from core import config
from Tags import *

class DirBrowse(Page.Page):
    """A page that shows the entire contents of directories, in a form similar
    to LXR."""
    def __init__(self, manager):
	Page.Page.__init__(self, manager)
        self.__filename = config.files.nameOfSpecial('dir')
        self.__title = 'Directory Listing'
	self.__base = config.base_dir
	self.__start = config.start_dir

    def filename(self):
        """since FileTree generates a whole file hierarchy, this method returns the current filename,
        which may change over the lifetime of this object"""
        return self.__filename
    def title(self):
        """since FileTree generates a while file hierarchy, this method returns the current title,
        which may change over the lifetime of this object"""
        return self.__title

    def filename_for_dir(self, dir):
	"""Returns the output filename for the given input directory"""
	if dir is self.__start:
	    return config.files.nameOfSpecial('dir')
	scope = string.split(rel(self.__start, dir), os.sep)
	return config.files.nameOfScopedSpecial('dir', scope)

    def register(self):
	"""Registers a page for each file in the hierarchy"""
	#if not self.__base: return
	config.set_main_page(self.__filename)
        self.__filename = config.files.nameOfSpecial('dir')
	self.manager.addRootPage(self.__filename, 'Files', 'main', 2)

    def register_filenames(self, start):
	"""Registers a page for every directory"""
	dirs = [self.__start]
	while dirs:
	    dir = dirs.pop(0)
	    for entry in os.listdir(dir):
		entry_path = os.path.join(dir, entry)
		info = statcache.stat(entry_path)
		if not stat.S_ISDIR(info[stat.ST_MODE]):
		    continue
		filename = self.filename_for_dir(dir)
		self.manager.register_filename(filename, self, entry_path)
   
    def process(self, start):
	"""Recursively visit each directory below the base path given in the
	config."""
	#if not self.__base: return
	self.process_dir(self.__start)
    
    def process_dir(self, path):
	"""Process a directory, producing an output page for it"""

	# Find the filename
	filename = self.filename_for_dir(path)
	self.__filename = filename

	# Start the file
	self.start_file()
	self.write(self.manager.formatHeader(self.filename(), 2))
	# Write intro stuff
	root = rel(self.__base, self.__start)
	if not len(root) or root[-1] != '/': root = root + '/'
	if path is self.__start:
	    self.write('<h1> '+root)
	else:
	    self.write('<h1>'+href(config.files.nameOfSpecial('dir'), root + ' '))
	    dirscope = []
	    scope = string.split(rel(self.__start, path), os.sep)
	    for dir in scope[:-1]:
		dirscope.append(dir)
		self.write(href(config.files.nameOfScopedSpecial('dir', dirscope), dir+'/ '))
	    if len(scope) > 0:
		self.write(scope[-1]+'/')
	self.write(' - Directory listing</h1>')
	# Start the table
	self.write('<table summary="Directory Listing">\n')
	self.write('<tr><th align=left>Name</th>')
	self.write('<th align="right">Size (bytes)</th>')
	self.write('<th align="right">Last modified (GMT)</th></tr>\n')
	# List all files in the directory
	entries = os.listdir(path)
	entries.sort()
	files = []
	dirs = []
	for entry in entries:
	    entry_path = os.path.join(path, entry)
	    info = os.stat(entry_path)
	    if stat.S_ISDIR(info[stat.ST_MODE]):
		# A directory, process now
		scope = string.split(rel(self.__start, entry_path), os.sep)
		linkpath = config.files.nameOfScopedSpecial('dir', scope)
		self.write('<tr><td>%s</td><td></td><td align="right">%s</td></tr>\n'%(
		    href(rel(self.filename(), linkpath), entry+'/'),
		    time.asctime(time.gmtime(info[stat.ST_MTIME]))))
		dirs.append(entry_path)
	    else:
		files.append((entry_path, entry, info))
	for path, entry, info in files:
	    size = info[stat.ST_SIZE]
	    timestr = time.asctime(time.gmtime(info[stat.ST_MTIME]))
	    linkpath = config.files.nameOfFileSource(path)
	    rego = self.manager.filename_info(linkpath)
	    if rego:
		self.write('<tr><td>%s</td><td align=right>%d</td><td align="right">%s</td></tr>\n'%(
		    href(linkpath, entry, target="main"), size, timestr))
	    else:
		print "No link for",linkpath
		self.write('<tr><td>%s</td><td align=right>%d</td><td align="right">%s</td></tr>\n'%(
		    entry, size, timestr))
	# End the table and file
	self.write('</table>')
	self.end_file()

	# recursively create all child directory pages
	for dir in dirs:
	    self.process_dir(dir)
	
htmlPageClass = DirBrowse
