# $Id: Scope.py,v 1.3 2001/02/05 07:58:39 chalky Exp $
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
# $Log: Scope.py,v $
# Revision 1.3  2001/02/05 07:58:39  chalky
# Cleaned up image copying for *JS. Added synopsis logo to ScopePages.
#
# Revision 1.2  2001/02/01 15:23:24  chalky
# Copywritten brown paper bag edition.
#

# System modules
import time, os

# Synopsis modules
from Synopsis.Core import AST

# HTML modules
import Page
import core
import ASTFormatter
from core import config
from Tags import *

class ScopePages (Page.Page):
    """A module for creating a page for each Scope with summaries and
    details."""
    def __init__(self, manager):
	Page.Page.__init__(self, manager)
	#TODO use config...
	self.summarizer = ASTFormatter.SummaryFormatter()
	self.detailer = ASTFormatter.DetailFormatter()
	# Hack to find share dir..
	share = os.path.split(AST.__file__)[0]+"/../share"
	self.syn_logo = 'synopsis200.jpg'
	config.files.copyFile(share+'/synopsis200.jpg', self.syn_logo)

    def process(self, start):
	"""Creates a page for every Scope"""
	self.__namespaces = [start]
	while self.__namespaces:
	    ns = self.__namespaces.pop(0)
	    self.processScope(ns)

    def processScope(self, ns):
	"""Creates a page for the given scope"""
	details = {} # A hash of lists of detailed children by type
	sections = [] # a list of detailed sections
	
	# Open file and setup scopes
	self.startFileScope(ns.name())
	config.sorter.set_scope(ns)
	config.sorter.sort_section_names()
	
	# Write heading
	self.write(self.manager.formatRoots('')+'<hr>')
	if ns is self.manager.globalScope(): 
	    self.write(entity('h1', "Global Namespace"))
	else:
	    # Use the detailer to print an appropriate heading
	    ns.accept(self.detailer)
	
	# Loop throught all the types of children
	self.printScopeSummaries(ns, details, sections)
	self.printScopeDetails(details, sections)
	self.endFile()
	
	# Queue child namespaces
	for child in config.sorter.children():
	    if isinstance(child, AST.Scope):
		self.__namespaces.append(child)
    
    def printScopeSummaries(self, ns, details, sections):
	"Print out the summaries from the given ns and note detailed items"
	comments = config.comments
	for section in config.sorter.sections():
	    # Write a header for this section
	    if section[-1] == 's': heading = section+'es Summary:'
	    else: heading = section+'s Summary:'
	    self.summarizer.writeSectionStart(heading)
	    # Iterate through the children in this section
	    for child in config.sorter.children(section):
		# Check if need to add to detail list
		if comments[child].has_detail:
		    # Add this decl to the detail dictionary
		    if not details.has_key(section):
			details[section] = []
			sections.append(section)
		    details[section].append(child)
		    self.summarizer.set_link_detail(1)
		# Print out summary for the child
		child.accept(self.summarizer)
		self.summarizer.set_link_detail(0)
	    # Finish off this section
	    self.summarizer.writeSectionEnd(heading)

    def printScopeDetails(self, details, sections):
	"Print out the details from the given dict of lists"
	# Iterate through the sections with details
	for section in sections:
	    # Write a heading
	    heading = section+' Details:'
	    self.detailer.writeSectionStart(heading)
	    # Write the sorted list of children of this type
	    for child in details[section]:
		child.accept(self.detailer)
	    # Finish the section
	    self.detailer.writeSectionEnd(heading)

 
    def startFileScope(self, scope):
	"Start a new file from the given scope"
	fname = config.files.nameOfScope(scope)
	title = string.join(scope)
	self.startFile(fname, title)
	self.summarizer.set_ostream(self.os())
	self.summarizer.set_scope(scope)
	self.detailer.set_ostream(self.os())
	self.detailer.set_scope(scope)

    def endFile(self):
	"""Overrides endfile to provide synopsis logo"""
	self.write('<hr>Generated on %s by %s'%(
	    time.strftime(r'%c', time.localtime(time.time())),
	    solotag('img', align='top', src=self.syn_logo, border='0', alt="Synopsis")
	))
	Page.Page.endFile(self)
 
htmlPageClass = ScopePages
