# $Id: View.py,v 1.4 2001/06/26 04:32:16 stefan Exp $
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
# $Log: View.py,v $
# Revision 1.4  2001/06/26 04:32:16  stefan
# A whole slew of changes mostly to fix the HTML formatter's output generation,
# i.e. to make the output more robust towards changes in the layout of files.
#
# the rpm script now works, i.e. it generates source and binary packages.
#
# Revision 1.3  2001/02/05 05:26:24  chalky
# Graphs are separated. Misc changes
#
# Revision 1.2  2001/02/01 15:23:24  chalky
# Copywritten brown paper bag edition.
#
#

"""
A module with Page so you can import it for derivation
"""

from Synopsis.Core import Util

import core
from core import config
from Tags import *

class Page:
    """Base class for a Page"""
    def __init__(self, manager):
	"Constructor"
	self.manager = manager
	self.__os = None

    def os(self):
	"Returns the output stream opened with startFile"
	return self.__os

    def write(self, str):
	"""Writes the given string to the currently opened file"""
	self.__os.write(str)
       
    def process(self, start):
	"""Process the given Scope recursively. This is the method which is
	called to actually create the files, so you probably want to override
	it ;)"""
	
    def startFile(self, filename, title, body='<body>', headextra=''):
	"""Start a new file with given filename, title and body. This method
	opens a file for writing, and writes the html header crap at the top.
	You must specify a title, which is prepended with the project name.
	The body argument is optional, and it is preferred to use stylesheets
	for that sort of stuff. You may want to put an onLoad handler in it
	though in which case that's the place to do it. The opened file is
	stored and can be accessed using the os() method."""
	self.__filename = filename
	self.__os = Util.open(self.__filename)
	self.write("<html><head>\n")
	self.write(entity('title','Synopsis - '+title))
	if len(config.stylesheet):
	    self.write(entity('link', '', rel='stylesheet', href=config.stylesheet))
	self.write(headextra)
	self.write("</head>%s\n"%body)
    
    def endFile(self, body='</body>'):
	"Close the file using given close body tag"
	self.write("%s</html>\n"%body)
	self.__os.close()
	self.__os = None



