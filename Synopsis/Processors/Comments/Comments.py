# $Id: Comments.py,v 1.23 2003/10/15 03:44:01 stefan Exp $
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
# Revision 1.23  2003/10/15 03:44:01  stefan
# add new Grouper 'group2' to match '@group {' and '@group }'
#
# Revision 1.22  2003/10/14 00:28:54  stefan
# * separate the 'Grouper' class into base and derived
#   such that the derived class provides the 'process' method
#   that is specific to the actual tags used for opening and closing the group
# * add a 'Stripper' class that removes all but the last comments from
#   all declarations
#
# Revision 1.21  2003/10/13 18:50:19  stefan
# * provide a clearer definition of 'Comment', i.e.
#   a single comment as opposed to a list of comments
# * simplify grouping tags with the above clarifications
#
# Revision 1.20  2003/10/07 21:15:00  stefan
# adjust to new default grouping syntax
#
# Revision 1.19  2003/10/07 02:55:40  stefan
# make group handling more fault tolerant
#
# Revision 1.18  2003/01/20 06:43:02  chalky
# Refactored comment processing. Added AST.CommentTag. Linker now determines
# comment summary and extracts tags. Increased AST version number.
#
# Revision 1.17  2002/10/11 11:07:53  chalky
# Added missing parent __init__ call in Group
#
# Revision 1.16  2002/10/11 05:57:17  chalky
# Support suspect comments
#
# Revision 1.15  2002/09/20 10:34:52  chalky
# Add a comment parser for plain old // comments
#
# Revision 1.14  2002/08/23 04:37:26  chalky
# Huge refactoring of Linker to make it modular, and use a config system similar
# to the HTML package
#
# Revision 1.13  2002/04/26 01:21:14  chalky
# Bugs and cleanups
#
# Revision 1.12  2002/04/25 23:54:09  chalky
# Fixed bug caused by new re module in python 2.1 handling groups differently
#
# Revision 1.11  2001/06/11 10:37:49  chalky
# Better grouping support
#
# Revision 1.10  2001/06/08 21:04:38  stefan
# more work on grouping
#
# Revision 1.9  2001/06/08 04:50:13  stefan
# add grouping support
#
# Revision 1.8  2001/05/25 13:45:49  stefan
# fix problem with getopt error reporting
#
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

"""Comment Processor"""
# System modules
import sys, string, re, getopt, types

# Synopsis modules
from Synopsis.Core import AST, Util

from Synopsis.Linker.Linker import config, Operation

class CommentProcessor (AST.Visitor):
    """Base class for comment processors.

    This is an AST visitor, and by default all declarations call process()
    with the current declaration. Subclasses may override just the process
    method.
    """
    def processAll(self, declarations):
        for decl in declarations:
            decl.accept(self)
        self.finalize(declarations)
    def process(self, decl):
        """Process comments for the given declaration"""
        pass
    def finalize(self, declarations):
        """Do any finalization steps that may be necessary."""
        pass
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
    __re_java = r"/\*\*[ \t]*(?P<text>.*)(?P<lines>(\n[ \t]*\*.*)*?)(\n[ \t]*)?\*/"
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

class SSComments (CommentProcessor):
    """A class that selects only // comments."""
    __re_star = r'/\*(.*?)\*/'
    __re_ss = r'^[ \t]*// ?(.*)$'
    def __init__(self):
	"Compiles the regular expressions"
	self.re_star = re.compile(SSComments.__re_star, re.S)
	self.re_ss = re.compile(SSComments.__re_ss, re.M)
    def process(self, decl):
	"Calls processComment on all comments"
	map(self.processComment, decl.comments())
    def processComment(self, comment):
	"""Replaces the text in the comment. It calls strip_star() first to
	remove all multi-line star comments, then follows with parse_ss().
	"""
	text = comment.text()
	text = self.parse_ss(self.strip_star(text))
	comment.set_text(text)
    def strip_star(self, str):
	"""Strips all star-format comments from the string"""
	mo = self.re_star.search(str)
	while mo:
	    str = str[:mo.start()] + str[mo.end():]
	    mo = self.re_star.search(str)
	return str
    def parse_ss(self, str):
	"""Filters str and returns just the lines that start with //"""
	return string.join(self.re_ss.findall(str),'\n')


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

class Transformer (CommentProcessor):
    """A class that creates a new AST from an old one. This is a helper base for
    more specialized classes that manipulate the AST based on the comments in the nodes"""
    def __init__(self):
	"""Constructor"""
	self.__scopestack = []
	self.__currscope = []
    def finalize(self, declarations):
	"""replace the AST with the newly created one"""
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
	
class Dummies (Transformer):
    """A class that deals with dummy declarations and their comments. This
    class just removes them."""
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
	scope.declarations()[:] = self.currscope()
	self.pop(scope)
    def visitEnum(self, enum):
	"""Does the same as visitScope, but for the enum's list of
	enumerators"""
	self.push()
	for enumor in enum.enumerators(): enumor.accept(self)
	enum.enumerators()[:] = self.currscope()
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
	for decl in declarations:
	    decl.accept(self)
	    self.last = decl
	declarations[:] = self.currscope()
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
	self.removeSuspect(scope)
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
	if len(decl.comments()) and self.last:
	    first = decl.comments()[0]
	    if len(first.text()) and first.text()[0] == "<" and self.last:
		first.set_suspect(0) # Remove suspect flag
		first.set_text(first.text()[1:]) # Remove '<'
		self.last.comments().append(first)
		del decl.comments()[0]
    def removeSuspect(self, decl):
	"""Removes any suspect comments from the declaration"""
	non_suspect = lambda decl: not decl.is_suspect()
	comments = decl.comments()
	comments[:] = filter(non_suspect, comments)
    def visitDeclaration(self, decl):
	"""Calls checkPrevious on the declaration and removes dummies"""
	self.checkPrevious(decl)
	self.removeSuspect(decl)
	if decl.type() != "dummy":
	    self.add(decl)
    def visitEnum(self, enum):
	"""Does the same as visitScope but for enum and enumerators"""
	self.removeSuspect(enum)
	self.push()
	for enumor in enum.enumerators():
	    enumor.accept(self)
	    self.last = enumor
	enum.enumerators()[:] = self.currscope()
	self.pop(enum)
    def visitEnumerator(self, enumor):
	"""Checks previous comment and removes dummies"""
	self.removeSuspect(enumor)
	self.checkPrevious(enumor)
	if len(enumor.name()): self.add(enumor)

class Grouper (Transformer):
    """A class that detects grouping tags and moves the enclosed nodes into a subnode (a 'Group')"""
    def __init__(self):
	Transformer.__init__(self)
        self.__group_stack = [[]]

    def strip_dangling_groups(self):
        """As groups must not overlap with 'real' scopes,
        make sure all groups created in the current scope are closed
        when leaving the scope."""
        if self.__group_stack[-1]:
            print 'Warning: group stack is non-empty !'
            while (self.__group_stack[-1]):
                group = self.__group_stack[-1][-1]
                print 'forcing closing of group %s (opened near %s:%d)'%(group.name(), group.file().filename(), group.line())
                self.pop_group()

    def finalize(self, declarations):
        """replace the AST with the newly created one"""
        self.strip_dangling_groups()
        Transformer.finalize(self, declarations)

    def push(self):
        """starts a new group stack to be able to validate group scopes"""
        Transformer.push(self)
        self.__group_stack.append([])

    def pop(self, decl):
        """Make sure the current group stack is empty."""
        self.strip_dangling_groups()
        self.__group_stack.pop()
        Transformer.pop(self, decl)

    def push_group(self, group):
        """Push new group scope to the stack."""
        self.__group_stack[-1].append(group)
        Transformer.push(self)

    def pop_group(self, decl=None):
        """Pop a group scope from the stack.

        decl -- an optional declaration from which to extract the context,
        used for the error message if needed.
        """
        if self.__group_stack[-1]:
            group = self.__group_stack[-1].pop()
            group.declarations()[:] = self.currscope()
            Transformer.pop(self, group)
        else:
            if decl:
                print "Warning: no group open in current scope (near %s:%d), ignoring."%(decl.file().filename(), decl.line())
            else:
                print "Warning: no group open in current scope, ignoring."

    def process(self, decl):
        """Checks for grouping tags.
        If an opening tag is found in the middle of a comment, a new Group is generated, the preceeding
        comments are associated with it, and is pushed onto the scope stack as well as the groups stack.
        """
        pass

    def visitDeclaration(self, decl):
        self.process(decl)
	self.add(decl)
        
    def visitScope(self, scope):
	"""Visits all children of the scope in a new scope. The value of
	currscope() at the end of the list is used to replace scope's list of
	declarations - hence you can remove (or insert) declarations from the
	list. Such as dummy declarations :)"""
        self.process(scope)
	self.push()
	for decl in scope.declarations(): decl.accept(self)
	scope.declarations()[:] = self.currscope()
	self.pop(scope)

    def visitEnum(self, enum):
	"""Does the same as visitScope, but for the enum's list of
	enumerators"""
        self.process(enum)
	self.push()
	for enumor in enum.enumerators(): enumor.accept(self)
	enum.enumerators()[:] = self.currscope()
	self.pop(enum)

    def visitEnumerator(self, enumor):
	"""Removes dummy enumerators"""
	if enumor.type() == "dummy": return #This wont work since Core.AST.Enumerator forces type to "enumerator"
	if not len(enumor.name()): return # workaround.
	self.add(enumor)

class Grouper1(Grouper):
    """Grouper that detects ' @group {' and '}' group markup"""
    __re_group = r'^[ \t]*((?P<open>@group[ \t]*(?P<name>.*){)|(?P<close>[ \t]*}))[ \t]*\Z'
    def __init__(self):
	Grouper.__init__(self)
	self.re_group = re.compile(Grouper1.__re_group, re.M)

    def process(self, decl):
        """Checks for grouping tags.
        If an opening tag is found in the middle of a comment, a new Group is generated, the preceeding
        comments are associated with it, and is pushed onto the scope stack as well as the groups stack.
        """
        comments = []
        process_comments = decl.comments()
        while len(process_comments):
            c = process_comments.pop(0)
            tag = self.re_group.search(c.text())
            if not tag:
                comments.append(c)
                continue
            elif tag.group('open'):
                # Open group. Name is remainder of line
                label = tag.group('name') or 'unnamed'
                # The comment before the open marker becomes the group comment
                if tag.start('open') > 0:
                    text = c.text()[:tag.start('open')]
                    comments.append(AST.Comment(text, c.file(), c.line()))
                group = AST.Group(decl.file(), decl.line(), decl.language(), "group", [label])
                group.comments()[:] = comments
                comments = []
                self.push_group(group)
            elif tag.group('close'):
                self.pop_group(decl)
        decl.comments()[:] = comments

class Grouper2(Grouper):
    """Grouper that detects ' @group {' and ' @group }' group markup"""
    __re_group = r'^[ \t]*((?P<open>@group[ \t]*(?P<name>.*){)|(?P<close>@group[ \t]*}))(?P<remainder>.*)$'
    def __init__(self):
	Grouper.__init__(self)
	self.re_group = re.compile(Grouper2.__re_group, re.M)

    def process(self, decl):
        """Checks for grouping tags.
        If an opening tag is found in the middle of a comment, a new Group is generated, the preceeding
        comments are associated with it, and is pushed onto the scope stack as well as the groups stack.
        """
        comments = []
        process_comments = decl.comments()
        while len(process_comments):
            c = process_comments.pop(0)
            tag = self.re_group.search(c.text())
            if not tag:
                comments.append(c)
                continue
            elif tag.group('open'):
                # Open group. Name is remainder of line
                label = tag.group('name') or 'unnamed'
                # The comment before the open marker becomes the group comment
                if tag.start('open') > 0:
                    text = c.text()[:tag.start('open')]
                    comments.append(AST.Comment(text, c.file(), c.line()))
                group = AST.Group(decl.file(), decl.line(), decl.language(), "group", [label])
                group.comments()[:] = comments
                comments = []
                # The comment after the open marker becomes the next comment to process
                if tag.group('remainder'):
                    process_comments.insert(0, AST.Comment(tag.group('remainder'), c.file(), c.line()))
                self.push_group(group)
            elif tag.group('close'):
                self.pop_group(decl)
            # The comment before the close marker is ignored...? maybe post-comment?
            # The comment after the close marker becomes the next comment to process
            remainder = string.join([tag.group('remainder'), c.text()[tag.end():]], '')
            if remainder:
                process_comments.insert(0, AST.Comment(remainder, c.file(), c.line()))
        decl.comments()[:] = comments

class Stripper(CommentProcessor):
    """Strip off all but the last comment."""
    def process(self, decl):
	"""Summarize the comment of this declaration."""
	if not len(decl.comments()):
	    return
        decl.comments()[:] = [decl.comments()[-1]]

class Summarizer (CommentProcessor):
    """Splits comments into summary/detail parts."""
    re_summary = r"[ \t\n]*(.*?\.)([ \t\n]|$)"
    def __init__(self):
	self.re_summary = re.compile(Summarizer.re_summary, re.S)
    def process(self, decl):
	"""Summarize the comment of this declaration."""
	comments = decl.comments()
	if not len(comments):
	    return
	# Only use last comment
	comment = comments[-1]
	tags = comment.tags()
	# Decide how much of the comment is the summary
	text = comment.text()
	mo = self.re_summary.match(text)
	if mo:
	    # Set summary to the sentence
	    comment.set_summary(mo.group(1))
	else:
	    # Set summary to whole text
	    comment.set_summary(text)

class JavaTags (CommentProcessor):
    """Extracts javadoc-style @tags from the end of comments."""

    # The regexp to use for finding all the tags
    _re_tags = '\n[ \t]*(?P<tags>@[a-zA-Z]+[ \t]+.*)'

    def __init__(self):
	self.re_tags = re.compile(self._re_tags,re.M|re.S)
    def process(self, decl):
	"""Extract tags from each comment of the given decl"""
	for comment in decl.comments():
	    # Find tags
	    text = comment.text()
	    mo = self.re_tags.search(text)
	    if not mo:
		continue
	    # A lambda to use in the reduce expression
	    joiner = lambda x,y: len(y) and y[0]=='@' and x+[y] or x[:-1]+[x[-1]+' '+y]

	    tags = mo.group('tags')
	    text = text[:mo.start('tags')]
	    # Split the tag section into lines
	    tags = map(string.strip, string.split(tags,'\n'))
	    # Join non-tag lines to the previous tag
	    tags = reduce(joiner, tags, [])
	    # Split the tag lines into @name, rest-of-line pairs
	    tags = map(lambda line: string.split(line,' ',1), tags)
	    # Convert the pairs into CommentTag objects
	    tags = map(lambda pair: AST.CommentTag(pair[0], pair[1]), tags)
	    # Store back in comment
	    comment.set_text(text)
	    comment.tags().extend(tags)

processors = {
    'ssd': SSDComments,
    'ss' : SSComments,
    'java': JavaComments,
    'qt': QtComments,
    'dummy': Dummies,
    'prev': Previous,
    'group': Grouper1,
    'group2': Grouper2,
    'stripper': Stripper,
    'summary' : Summarizer,
    'javatags' : JavaTags,
}

class Comments(Operation):
    def __init__(self):
	"""Constructor, parses the config object"""
	self.processor_list = []

	if hasattr(config, 'comment_processors'):
	    for proc in config.comment_processors:
		if type(proc) == types.StringType:
		    if processors.has_key(proc):
			self.processor_list.append(processors[proc]())
		    else:
			raise ImportError, 'No such processor: %s'%(proc,)
		elif type(proc) == types.TupleType:
		    mod = Util._import(proc[0])
		    clas = getattr(mod, proc[1])
		    self.processor_list.append(clas())

    def execute(self, ast):
	declarations = ast.declarations()
	for processor in self.processor_list:
	    processor.processAll(declarations)

linkerOperation = Comments

