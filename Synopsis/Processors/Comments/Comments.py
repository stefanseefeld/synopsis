# $Id: Comments.py,v 1.7 2001/04/05 16:21:56 chalky Exp $
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
# $Log: Comments.py,v $
# Revision 1.7  2001/04/05 16:21:56  chalky
# Allow user-specified comment processors
#
# Revision 1.6  2001/04/05 11:11:39  chalky
# Many more comments
#
# Revision 1.5  2001/02/12 04:08:09  chalky
# Added config options to HTML and Linker. Config demo has doxy and synopsis styles.
#
# Revision 1.4  2001/02/07 17:00:43  chalky
# Added Qt-style comments support
#
# Revision 1.3  2001/02/07 14:13:51  chalky
# Small fixes.
#
# Revision 1.2  2001/02/07 09:57:00  chalky
# Support for "previous comments" in C++ parser and Comments linker.
#
# Revision 1.1  2001/02/06 06:55:18  chalky
# Initial commit. Support SSD and Java comments. Selection of comments only
# (same as ssd,java formatters in HTML)
#
#

# TODO - Linker/ is only a temporary location for this file.

"""Comment Processor"""
# System modules
import sys, string, re, getopt, types

# Synopsis modules
from Synopsis.Core import AST, Util

class CommentProcessor (AST.Visitor):
    """Base class for comment processors.

    This is an AST visitor, and by default all declarations call process()
    with the current declaration. Subclasses may override just the process
    method.
    """
    def processAll(self, declarations):
	for decl in declarations:
	    decl.accept(self)
    def process(self, decl):
	"""Process comments for the given declaration"""
    def visitDeclaration(self, decl):
	self.process(decl)

class SSDComments (CommentProcessor):
    """A class that selects only //. comments."""
    __re_star = r'/\*(.*?)\*/'
    __re_ssd = r'^[ \t]*//\. ?(.*)$'
    def __init__(self):
	"Compiles the regular expressions"
	self.re_star = re.compile(SSDComments.__re_star, re.S)
	self.re_ssd = re.compile(SSDComments.__re_ssd, re.M)
    def process(self, decl):
	"Calls processComment on all comments"
	map(self.processComment, decl.comments())
    def processComment(self, comment):
	"""Replaces the text in the comment. It calls strip_star() first to
	remove all multi-line star comments, then follows with parse_ssd().
	"""
	text = comment.text()
	text = self.parse_ssd(self.strip_star(text))
	comment.set_text(text)
    def strip_star(self, str):
	"""Strips all star-format comments from the string"""
	mo = self.re_star.search(str)
	while mo:
	    str = str[:mo.start()] + str[mo.end():]
	    mo = self.re_star.search(str)
	return str
    def parse_ssd(self, str):
	"""Filters str and returns just the lines that start with //."""
	return string.join(self.re_ssd.findall(str),'\n')

class JavaComments (CommentProcessor):
    """A class that formats java /** style comments"""
    __re_java = r"/\*\*[ \t]*(?P<text>.*)(?P<lines>\n[ \t]*\*.*)*?(\n[ \t]*)?\*/"
    __re_line = r"\n[ \t]*\*[ \t]*(?P<text>.*)"
    def __init__(self):
	"Compiles the regular expressions"
	self.re_java = re.compile(JavaComments.__re_java)
	self.re_line = re.compile(JavaComments.__re_line)
    def process(self, decl):
	"Calls processComment on all comments"
	map(self.processComment, decl.comments())
    def processComment(self, comment):
	"""Finds comments in the java format. The format is  /** ... */, and
	it has to cater for all four line forms: "/** ...", " * ...", " */" and
	the one-line "/** ... */".
	"""
	text = comment.text()
	text_list = []
	mo = self.re_java.search(text)
	while mo:
	    text_list.append(mo.group('text'))
	    lines = mo.group('lines')
	    if lines:
		mol = self.re_line.search(lines)
		while mol:
		    text_list.append(mol.group('text'))
		    mol = self.re_line.search(lines, mol.end())
	    mo = self.re_java.search(text, mo.end())
	text = string.join(text_list,'\n')
	comment.set_text(text)

class QtComments (CommentProcessor):
    """A class that finds Qt style comments. These have two styles: //! ...
    and /*! ... */. The first means "brief comment" and there must only be
    one. The second type is the detailed comment."""
    __re_brief = r"[ \t]*//!(.*)"
    __re_detail = r"[ \t]*/\*!(.*)\*/[ \t\n]*"
    def __init__(self):
	"Compiles the regular expressions"
	self.re_brief = re.compile(self.__re_brief)
	self.re_detail = re.compile(self.__re_detail, re.S)
    def process(self, decl):
	"Calls processComment on all comments"
	map(self.processComment, decl.comments())
    def processComment(self, comment):
	"Matches either brief or detailed comments"
	text = comment.text()
	mo = self.re_brief.match(text)
	if mo:
	    comment.set_text(mo.group(1))
	    return
	mo = self.re_detail.match(text)
	if mo:
	    comment.set_text(mo.group(1))
	    return
	comment.set_text('')

class Dummies (CommentProcessor):
    """A class that deals with dummy declarations and their comments. This
    class just removes them. Since it has to modify the AST, it creates a new
    AST which can then be copied over the old."""
    def processAll(self, declarations):
	"""Overrides the default processAll() to setup the stack"""
	self.__scopestack = []
	self.__currscope = []
	for decl in declarations: decl.accept(self)
	declarations[:] = self.__currscope
    def push(self):
	"""Pushes the current scope onto the stack and starts a new one"""
	self.__scopestack.append(self.__currscope)
	self.__currscope = []
    def pop(self, decl):
	"""Pops the current scope from the stack, and appends the given
	declaration to it"""
	self.__currscope = self.__scopestack.pop()
	self.__currscope.append(decl)
    def add(self, decl):
	"""Adds the given decl to the current scope"""
	self.__currscope.append(decl)
    def currscope(self):
	"""Returns the current scope: a list of declarations"""
	return self.__currscope
	
    def visitDeclaration(self, decl):
	"""Checks for dummy declarations"""
	if decl.type() == "dummy": return
	self.add(decl)
    def visitScope(self, scope):
	"""Visits all children of the scope in a new scope. The value of
	currscope() at the end of the list is used to replace scope's list of
	declarations - hence you can remove (or insert) declarations from the
	list. Such as dummy declarations :)"""
	self.push()
	for decl in scope.declarations(): decl.accept(self)
	scope.declarations()[:] = self.__currscope
	self.pop(scope)
    def visitEnum(self, enum):
	"""Does the same as visitScope, but for the enum's list of
	enumerators"""
	self.push()
	for enumor in enum.enumerators(): enumor.accept(self)
	enum.enumerators()[:] = self.__currscope
	self.pop(enum)
    def visitEnumerator(self, enumor):
	"""Removes dummy enumerators"""
	if enumor.type() == "dummy": return #This wont work since Core.AST.Enumerator forces type to "enumerator"
	if not len(enumor.name()): return # workaround.
	self.add(enumor)

class Previous (Dummies):
    """A class that maps comments that begin with '<' to the previous
    declaration"""
    def processAll(self, declarations):
	"""decorates processAll() to initialise last and laststack"""
	self.last = None
	self.__laststack = []
	Dummies.processAll(self, declarations)
    def push(self):
	"""decorates push() to also push 'last' onto 'laststack'"""
	Dummies.push(self)
	self.__laststack.append(self.last)
	self.last = None
    def pop(self, decl):
	"""decorates pop() to also pop 'last' from 'laststack'"""
	Dummies.pop(self, decl)
	self.last = self.__laststack.pop()
    def visitScope(self, scope):
	"""overrides visitScope() to set 'last' after each declaration"""
	self.push()
	for decl in scope.declarations():
	    decl.accept(self)
	    self.last = decl
	scope.declarations()[:] = self.currscope()
	self.pop(scope)
    def checkPrevious(self, decl):
	"""Checks a decl to see if the comment should be moved. If the comment
	begins with a less-than sign, then it is moved to the 'last'
	declaration"""
	if len(decl.comments()):
	    first = decl.comments()[0]
	    if len(first.text()) and first.text()[0] == "<" and self.last:
		first.set_text(first.text()[1:]) # Remove '<'
		self.last.comments().append(first)
		del decl.comments()[0]
    def visitDeclaration(self, decl):
	"""Calls checkPrevious on the declaration and removes dummies"""
	self.checkPrevious(decl)
	if decl.type() != "dummy": self.add(decl)
    def visitEnum(self, enum):
	"""Does the same as visitScope but for enum and enumerators"""
	self.push()
	for enumor in enum.enumerators():
	    enumor.accept(self)
	    self.last = enumor
	enum.enumerators()[:] = self.currscope()
	self.pop(enum)
    def visitEnumerator(self, enumor):
	"""Checks previous comment and removes dummies"""
	self.checkPrevious(enumor)
	if len(enumor.name()): self.add(enumor)

	    

processors = {
    'ssd': SSDComments,
    'java': JavaComments,
    'qt': QtComments,
    'dummy': Dummies,
    'prev': Previous,
}
def __parseArgs(args, config):
    """Parses the arguments and config object"""
    global processor_list
    processor_list = []
    languagize = None

    if hasattr(config, 'comment_processors'):
	for proc in config.comment_processors:
	    if type(proc) == types.StringType:
		if processors.has_key(proc):
		    processor_list.append(processors[proc]())
		else:
		    raise ImportError, 'No such processor: %s'%(proc,)
	    elif type(proc) == types.TupleType:
		mod = Util._import(proc[0])
		clas = getattr(mod, proc[1])
		processor_list.append(clas())

    try:
        opts, remainder = getopt.getopt(args, "p:")
    except getopt.error, e:
        sys.stderr.write("Error in arguments: " + e + "\n")
        sys.exit(1)

    for o,a in opts:
	if o == '-p':
	    if processors.has_key(a):
		processor_list.append(processors[a]())
	    else:
		print "Available processors:",string.join(processors.keys())
		sys.exit(2)

def process(declarations, types, args, config):
    """Main module entry point"""
    __parseArgs(args, config)
    for processor in processor_list:
	processor.processAll(declarations)
