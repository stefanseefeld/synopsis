# $Id: RawFile.py,v 1.3 2002/11/13 02:29:24 chalky Exp $
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
# $Log: RawFile.py,v $
# Revision 1.3  2002/11/13 02:29:24  chalky
# Support exclude_glob option to exclude files from listings. Remove debug info.
#
# Revision 1.2  2002/11/11 15:04:05  chalky
# Fix bugs when start directory is ''
#
# Revision 1.1  2002/11/02 06:37:37  chalky
# Allow non-frames output, some refactoring of page layout, new modules.
#
#

# System modules
import time, os, stat, statcache, os.path, string

# Synopsis modules
from Synopsis.Core import AST, Util

# Formatter modules
from Synopsis.Formatter import TOC

# HTML modules
import Page
import core
import ASTFormatter
from core import config
from Tags import *

class RawFilePages (Page.Page):
    """A module for creating a page for each file with hyperlinked source"""
    def __init__(self, manager):
        Page.Page.__init__(self, manager)
        self.__base = config.base_dir
        self.__start = config.start_dir
        self.__files = None

    def filename(self):
        """since RawFilePages generates a whole file hierarchy, this method returns the current filename,
        which may change over the lifetime of this object"""
        return self.__filename
    def title(self):
        """since RawFilePages generates a while file hierarchy, this method returns the current title,
        which may change over the lifetime of this object"""
        return self.__title

    def _get_files(self):
        """Returns a list of (path, output_filename) for each file"""
        if self.__files is not None: return self.__files
        self.__files = []
        dirs = [self.__start]
        while dirs:
            dir = dirs.pop(0)
            for entry in os.listdir(os.path.abspath(dir)):
		# Check if entry is in exclude list
		exclude = 0
		for re in self.__exclude_globs:
		    if re.match(entry):
			exclude = 1
		if exclude:
		    continue
                entry_path = os.path.join(dir, entry)
                info = statcache.stat(entry_path)
                if stat.S_ISDIR(info[stat.ST_MODE]):
                    dirs.append(entry_path)
                else:
                    filename = config.files.nameOfFileSource(entry_path)
                    self.__files.append( (entry_path, filename) )
        return self.__files

    def process(self, start):
        """Creates a page for every file"""
        for path, filename in self._get_files():
            self.process_file(path, filename)

    def register_filenames(self, start):
        """Registers a page for every file"""
        for path, filename in self._get_files():
            self.manager.register_filename(filename, self, path)

    def process_file(self, original, filename):
        """Creates a page for the given file"""
        # Check that we got the rego
        reg_page, reg_scope = self.manager.filename_info(filename)
        if reg_page is not self: return

        self.__filename = filename
        self.__title = original
        self.start_file()
        self.write(self.manager.formatHeader(filename, 2))
        self.write('<h1>'+original+'</h1>')
        try:
            f = open(original, 'rt')
            lines = ['']+f.readlines()
            f.close()
            wid = 1
            if len(lines) > 1000: wid = 4
            elif len(lines) > 100: wid = 3
            elif len(lines) > 10: wid = 2
            spec = '%%0%dd | %%s'%wid
            for i in range(1, len(lines)):
                lines[i] = spec%(i, anglebrackets(lines[i]))
            self.write('<pre>')
            self.write(string.join(lines, ''))
            self.write('</pre>')
        except:
            self.write('An error occurred')
        self.end_file()

htmlPageClass = RawFilePages

