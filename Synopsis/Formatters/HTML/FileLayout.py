# $Id: FileLayout.py,v 1.19 2002/11/13 02:29:24 chalky Exp $
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
# $Log: FileLayout.py,v $
# Revision 1.19  2002/11/13 02:29:24  chalky
# Support exclude_glob option to exclude files from listings. Remove debug info.
#
# Revision 1.18  2002/11/13 01:01:49  chalky
# Improvements to links when using the Nested file layout
#
# Revision 1.17  2002/11/02 06:37:37  chalky
# Allow non-frames output, some refactoring of page layout, new modules.
#
# Revision 1.16  2002/11/01 07:21:15  chalky
# More HTML formatting fixes eg: ampersands and stuff
#
# Revision 1.15  2002/10/25 03:43:10  chalky
# Don't put spaces in url anchors
#
# Revision 1.14  2002/10/20 02:21:25  chalky
# Move quote function to Core.Util
#
# Revision 1.13  2002/09/28 06:32:08  chalky
# Ooops, removing print
#
# Revision 1.12  2002/09/28 06:15:51  chalky
# Quote filenames to prevent angle brackets being used
#
# Revision 1.11  2002/07/19 14:26:32  chalky
# Revert prefix in FileLayout but keep relative referencing elsewhere.
#
# Revision 1.9  2001/06/28 07:22:18  stefan
# more refactoring/cleanup in the HTML formatter
#
# Revision 1.8  2001/06/26 04:32:16  stefan
# A whole slew of changes mostly to fix the HTML formatter's output generation,
# i.e. to make the output more robust towards changes in the layout of files.
#
# the rpm script now works, i.e. it generates source and binary packages.
#
# Revision 1.7  2001/06/16 01:29:42  stefan
# change the HTML formatter to not use chdir, as this triggers a but in python's import implementation
#
# Revision 1.6  2001/02/05 07:58:39  chalky
# Cleaned up image copying for *JS. Added synopsis logo to ScopePages.
#
# Revision 1.5  2001/02/01 18:36:55  chalky
# Moved TOC out to Formatter/TOC.py
#
# Revision 1.4  2001/02/01 18:08:26  chalky
# Added ext=".html" argument to nameOfScopedSpecial
#
# Revision 1.3  2001/02/01 18:06:06  chalky
# Added nameOfScopedSpecial method
#
# Revision 1.2  2001/02/01 15:23:24  chalky
# Copywritten brown paper bag edition.
#
#

"""
FileLayout module. This is the base class for the FileLayout interface. The
default implementation stores everything in the same directory.
"""

# System modules
import os, sys, stat, string, re

# Synopsis modules
from Synopsis.Core import Util, AST
from Synopsis.Formatter import TOC

# HTML modules
import core
from core import config
from Tags import *

class FileLayout (TOC.Linker):
    """Base class for naming files.
    You may derive from this class an
    reimplement any methods you like. The default implementation stores
    everything in the same directory (specified by -o)."""
    def __init__(self):
	basename = config.basename
	stylesheet = config.stylesheet
	stylesheet_file = config.stylesheet_file
	if basename is None:
	    print "ERROR: No output directory specified."
	    print "You must specify a base directory with the -o option."
	    sys.exit(1)
	import os.path
	if not os.path.exists(basename):
            if config.verbose: print "Warning: Output directory does not exist. Creating."
	    try:
		os.makedirs(basename, 0755)
	    except EnvironmentError, reason:
		print "ERROR: Creating directory:",reason
		sys.exit(2)
	if stylesheet_file:
	    # Copy stylesheet in
	    self.copyFile(stylesheet_file, os.path.join(basename, stylesheet))
	if not os.path.isdir(basename):
	    print "ERROR: Output must be a directory."
	    sys.exit(1)
    def copyFile(self, orig_name, new_name):
	"""Copies file if newer. The file named by orig_name is compared to
	new_name, and if newer or new_name doesn't exist, it is copied."""
	try:
	    filetime = os.stat(orig_name)[stat.ST_MTIME]
	    if not os.path.exists(new_name) or \
		filetime > os.stat(new_name)[stat.ST_MTIME]:
		fin = open(orig_name,'r')
		fout = Util.open(new_name)
		fout.write(fin.read())
		fin.close()
		fout.close()
	except EnvironmentError, reason:
	    import traceback
	    traceback.print_exc()
	    print "ERROR: ", reason
	    sys.exit(2)
    
    def _checkMain(self, filename):
	"""Checks whether the given filename is the main index page or not. If
	it is, then it returns the filename from nameOfIndex(), else it returns
	it unchanged"""
	if filename == config.page_main: return self.nameOfIndex()
	return filename
	
    def nameOfScope(self, scope):
	"""Return the filename of a scoped name (class or module).
	The default implementation is to join the names with '-' and append
	".html" Additionally, special characters are Util.quoted in URL-style"""
	if not len(scope): return self._checkMain(self.nameOfSpecial('global'))
	return Util.quote(string.join(scope,'-')) + ".html"
    def _stripFilename(self, file):
        if len(file) and file[0] == '/': file = file[1:]
        if len(file) and file[-1] == '/': file = file[:-1]
        return file
    def nameOfFileIndex(self, file):
	"""Return the filename for the index of an input file.
        The base_dir config option is used.
	Default implementation is to join the path with '-', prepend "_file-"
	and append ".html" """
        file = self._stripFilename(file)
        return "_file-"+rel(config.base_dir, file).replace(os.sep, '-')+".html"
    def nameOfFileSource(self, file):
	"""Return the filename for the source of an input file.
        The base_dir config option is used.
	Default implementation is to join the path with '-', prepend "_source-"
	and append ".html" """
        file = self._stripFilename(file)
        return "_source-"+rel(config.base_dir, file).replace(os.sep, '-')+".html"
    def nameOfIndex(self):
	"""Return the name of the main index file. Default is index.html"""
	return "index.html"
    def nameOfSpecial(self, name):
	"""Return the name of a special file (tree, etc). Default is
	_name.html"""
	return self._checkMain("_" + name + ".html")
    def nameOfScopedSpecial(self, name, scope, ext=".html"):
	"""Return the name of a special type of scope file. Default is to join
	the scope with '-' and prepend '-'+name"""
	return "_%s-%s%s"%(name, Util.quote(string.join(scope, '-')), ext)
    def nameOfModuleTree(self):
	"""Return the name of the module tree index. Default is
	_modules.html"""
	return "_modules.html"
    def nameOfModuleIndex(self, scope):
	"""Return the name of the index of the given module. Default is to
	join the name with '-', prepend "_module_" and append ".html" """
	# Prefer module index for frames
	if config.using_module_index:
	    return "_module_" + Util.quote(string.join(scope, '-')) + ".html"
	# Fall back to the scope page
	return self.nameOfScope(scope)
	
    def link(self, decl):
	"""Create a link to the named declaration. This method may have to
	deal with the directory layout."""
	if isinstance(decl, AST.Scope):
	    # This is a class or module, so it has its own file
	    return self.nameOfScope(decl.name())
	# Assume parent scope is class or module, and this is a <A> name in it
	filename = self.nameOfScope(decl.name()[:-1])
	anchor = anglebrackets(decl.name()[-1].replace(' ','-'))
	return filename + "#" + anchor

class NestedFileLayout (FileLayout):
    """generates a structured file system instead of a flat one"""
    def __init__(self):
        FileLayout.__init__(self)

    def nameOfScope(self, scope):
	"""Return the filename of a scoped name (class or module).
	One subdirectory per scope"""
        prefix = 'Scopes'
	if not len(scope):
	    return self._checkMain(os.path.join(prefix, 'global') + '.html')
        else: return Util.quote(reduce(os.path.join, scope, prefix)) + '.html'

    def nameOfFileIndex(self, file):
	"""Return the filename for the index of an input file.
        The base_dir config option is used.
	Default implementation is to join the path with '-', prepend "_file-"
	and append ".html" """
        file = self._stripFilename(file)
        return os.path.join("File", rel(config.base_dir, file)+".html")

    def nameOfFileSource(self, file):
	"""Return the filename for the source of an input file.
        The base_dir config option is used.
	Default implementation is to join the path with '-', prepend "_source-"
	and append ".html" """
        file = self._stripFilename(file)
        return os.path.join("Source", rel(config.base_dir, file)+".html")

    def nameOfIndex(self):
	"""Return the name of the main index file."""
	return "index.html"

    def nameOfSpecial(self, name):
	"""Return the name of a special file (tree, etc)."""
	return self._checkMain(name + ".html")
    
    def nameOfScopedSpecial(self, name, scope, ext=".html"):
	"""Return the name of a special type of scope file"""
        return Util.quote(reduce(os.path.join, scope, name)) + ext

    def nameOfModuleTree(self):
	"""Return the name of the module tree index"""
	return "Modules.html"

    def nameOfModuleIndex(self, scope):
	"""Return the name of the index of the given module"""
        return Util.quote(reduce(os.path.join, scope, 'Modules')) + '.html'
