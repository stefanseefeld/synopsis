# $Id: FramesIndex.py,v 1.2 2001/02/01 15:23:24 chalky Exp $
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
# $Log: FramesIndex.py,v $
# Revision 1.2  2001/02/01 15:23:24  chalky
# Copywritten brown paper bag edition.
#
#

import Page
from core import config
from Tags import *

class FramesIndex (Page.Page):
    """A class that creates an index with frames"""
    # TODO figure out how to set the default frame contents
    def __init__(self, manager):
	Page.Page.__init__(self, manager)

    def process(self, start):
	"""Creates a frames index file"""
	me = config.files.nameOfIndex()
	# TODO use project name..
	self.startFile(me, "Synopsis - Generated Documentation", body='')
	fcontents = rel(me, config.page_contents)
	findex = rel(me, config.page_index)
	fglobal = rel(me, config.files.nameOfScope(start.name()))
	frame1 = solotag('frame', name='contents', src=fcontents)
	frame2 = solotag('frame', name='index', src=findex)
	frame3 = solotag('frame', name='main', src=fglobal)
	frameset1 = entity('frameset', frame1+frame2, rows="30%,*")
	frameset2 = entity('frameset', frameset1+frame3, cols="200,*")
	self.write(frameset2)
	self.endFile()


htmlPageClass = FramesIndex
