# $Id: TreeFormatter.py,v 1.2 2002/11/01 03:39:21 chalky Exp $
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
# $Log: TreeFormatter.py,v $
# Revision 1.2  2002/11/01 03:39:21  chalky
# Cleaning up HTML after using 'htmltidy'
#
# Revision 1.1  2001/06/05 10:43:07  chalky
# Initial TreeFormatter and a JS version
#
#

"""Tree formatter interface.

This module contains the class which defines the interface for all tree views.
A tree is a structure of leaves and nodes, where each leave has one node, each
node can have many child leaves or nodes, and there is one root node. Trees
are used to describe the relationship between modules, and also between files.
The user can select between different ways of representing trees, for example
as simple nested lists or as complex javascript trees that the user can expand
and collapse individual branches of. This module contains the tree interface
which is common to all implementations."""

class TreeFormatter:
    """Interface for all tree implementations. This tree class provides
    default implementations of its methods for example and basic usage. The
    implementation provided outputs a nested tree using the UL and LI html
    elements."""
    def __init__(self, page):
	"""A tree is a strategy, so it must be passed the page instance to
	display to."""
	self.page = page
	self.write = page.write
	self.os = page.os()

    def startTree(self):
	"""Writes anything to the file that needs to be written at the start.
	For example a script section for the global scripts used by a
	javascript tree."""
	self.write('<ul>')

    def endTree(self):
	"""Writes anything that needs to be written at the end."""
	self.write('</ul>')
    
    def writeLeaf(self, text):
	"""Writes a leaf to the output. A leaf is a node with no children, for
	example a module (not package) or a file (not directory). The text is
	output verbatim in the appropriate html tags, which in the default
	instance is LI"""
	self.write('<li>%s</li>'%text)

    def writeNodeStart(self, text):
	"""Starts a node with children. The text is written, and then a block
	of children is started. This method call must be followed by a
	corresponding writeNodeEnd() call. Invididual leaves inside the block
	may be written out using the writeLeaf() method."""
	self.write('<li>%s<ul>'%text)

    def writeNodeEnd(self):
	"""Ends a node with children. This method just closes any tags opened
	by the corresponding writeNodeStart() method for a node."""
	self.write('</ul></li>')
