# $Id: Action.py,v 1.3 2002/04/26 01:21:13 chalky Exp $
#
# This file is a part of Synopsis.
# Copyright (C) 2000, 2001 Stefan Seefeld
# Copyright (C) 2000, 2001 Stephen Davies
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
# $Log: Action.py,v $
# Revision 1.3  2002/04/26 01:21:13  chalky
# Bugs and cleanups
#
# Revision 1.2  2001/11/07 05:58:21  chalky
# Reorganised UI, opening a .syn file now builds a simple project to view it
#
# Revision 1.1  2001/11/05 06:52:11  chalky
# Major backside ui changes
#

"""Actions control the functionality of the stand-alone Synopsis. The
functionality is divided into Actions, which are responsible for different
steps such as checking source files, parsing, linking, formatting and
caching.

The actual Action objects only contain the information needed to perform the
actions, but are as lightweight as possible otherwise. This is because there
is potentially a lot of memory associated with each stage, which should be
allocated as late as possible and freed as soon as possible."""

import os, stat, string, re

class Action:
    """A Synopsis Action, ie: parsing, linking, formatting etc"""
    def __init__(self, x, y, name):
	self.__x = x
	self.__y = y
	self.__name = name
	self.__inputs = []
	self.__outputs = []
    
    # These are to do with the GUI, and may be moved out at some stage...
    def x(self): return self.__x
    def y(self): return self.__y
    def pos(self): return self.__x, self.__y
    def move_to(self, x, y): self.__x = x; self.__y = y
    def move_by(self, dx, dy): self.__x = self.__x+dx; self.__y = self.__y+dy
    
    # Proper action methods
    def name(self): return self.__name
    def set_name(self, name): self.__name = name
    def inputs(self): return self.__inputs
    def outputs(self): return self.__outputs
    def accept(self, visitor): visitor.visitAction(self)

class ActionVisitor:
    """A visitor for the Action hierarchy"""
    def visitAction(self, action): pass
    def visitSource(self, action): pass
    def visitParser(self, action): pass
    def visitLinker(self, action): pass
    def visitCacher(self, action): pass
    def visitFormat(self, action): pass

class SourcePath:
    """Struct for a source path"""
    def __init__(self, pathtype, dir, glob=None):
	if pathtype not in ('Simple', 'Dir', 'Base'):
	    raise ValueError, 'pathtype not valid'
	self.type = pathtype
	self.dir = dir
	self.glob = glob

class SourceAction (Action):
    """A Synopsis Action that loads source files"""
    def __init__(self, x, y, name):
	Action.__init__(self, x, y, name)
	self.__paths = []
    def paths(self):
	"""Returns a list of paths. Each path is a SourcePath object with a path or
	dir+glob combination of the source files to load."""
	return self.__paths
    def accept(self, visitor): return visitor.visitSource(self)

class ParserAction (Action):
    """A Synopsis Action that parses source files.
    
    Each parser object has a config object, which is passed to the Parser
    module. For a default config object, use Synopsis.Config.Base.xxx where
    xxx is the Parser module."""
    def __init__(self, x, y, name):
	Action.__init__(self, x, y, name)
	self.__config = None
    def config(self):
	"""Returns the config object for this Parser"""
	return self.__config
    def set_config(self, config):
	"""Sets the config object for this Parser."""
	self.__config = config
    def accept(self, visitor): return visitor.visitParser(self)

class LinkerAction (Action):
    """A Synopsis Action that links ASTs"""
    def __init__(self, x, y, name):
	Action.__init__(self, x, y, name)
	self.__config = None
    def config(self):
	"""Returns the config object for this Linker"""
	return self.__config
    def set_config(self, config):
	"""Sets the config object for this Linker."""
	self.__config = config
    def accept(self, visitor): return visitor.visitLinker(self)

class CacherAction (Action):
    """A Synopsis Action that caches ASTs to disk. It can optionally be used
    to load from a .syn file on disk, by setting the file attribute."""
    def __init__(self, x, y, name):
	Action.__init__(self, x, y, name)
	self.dir = '.'
	self.file = None
    def accept(self, visitor): return visitor.visitCacher(self)

class FormatAction (Action):
    """A Synopsis Action that formats ASTs into other media"""
    def __init__(self, x, y, name):
	Action.__init__(self, x, y, name)
	self.__config = None
    def config(self):
	"""Returns the config object for this Formatter"""
	return self.__config
    def set_config(self, config):
	"""Sets the config object for this Formatter."""
	self.__config = config
    def accept(self, visitor): return visitor.visitFormat(self)


