# $Id: Formatter.py,v 1.2 2001/02/01 15:23:24 chalky Exp $
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
# $Log: Formatter.py,v $
# Revision 1.2  2001/02/01 15:23:24  chalky
# Copywritten brown paper bag edition.
#
#

"""CommentParser, CommentFormatter and derivatives."""

# System modules
import re, string

# Synopsis modules
from Synopsis.Core import AST

# HTML modules
import core
from core import config
from Tags import *

class CommentParser:
    """A class that takes a Declaration and sorts its comments out."""
    def __init__(self):
	self._formatters = core.config.commentFormatterList
    def parse(self, decl):
	"""Parses the comments of the given AST.Declaration.
	Returns a struct with vars:
	 summary - one-line summary, detail - detailed info,
	 has_detail - true if should show detail,
	 decl - the declaration for this comment
	"""
	strlist = map(lambda x:str(x), decl.comments())
	comm = core.Struct(
	    detail=string.join(strlist), summary='', 
	    has_detail=0, decl=decl)
	map(lambda f,c=comm: f.parse(c), self._formatters)
	return comm

class CommentFormatter:
    """A class that takes a comment Struct and formats its contents."""

    def parse(self, comm):
	"""Parse the comment struct"""
	pass

class SSDFormatter:
    """A class that strips //.'s from the start of lines in detail"""
    __re_star = r'/\*(.*?)\*/'
    __re_ssd = r'^[ \t]*//\. ?(.*)$'
    def __init__(self):
	self.re_star = re.compile(SSDFormatter.__re_star, re.S)
	self.re_ssd = re.compile(SSDFormatter.__re_ssd, re.M)
    def parse(self, comm):
	comm.detail = self.parse_ssd(self.strip_star(comm.detail))
    def strip_star(self, str):
	"""Strips all star-format comments from the docstring"""
	mo = self.re_star.search(str)
	while mo:
	    str = str[:mo.start()] + str[mo.end():]
	    mo = self.re_star.search(str)
	return str
    def parse_ssd(self, str):
	return string.join(self.re_ssd.findall(str),'\n')

class JavaFormatter:
    """A class that formats java /** style comments"""
    __re_java = r"/\*\*[ \t]*(?P<text>.*)(?P<lines>\n[ \t]*\*.*)*?(\n[ \t]*)?\*/"
    __re_line = r"\n[ \t]*\*[ \t]*(?P<text>.*)"
    def __init__(self):
	self.re_java = re.compile(JavaFormatter.__re_java)
	self.re_line = re.compile(JavaFormatter.__re_line)
    def parse(self, comm):
	if comm.detail is None: return
	text_list = []
	mo = self.re_java.search(comm.detail)
	while mo:
	    text_list.append(mo.group('text'))
	    lines = mo.group('lines')
	    if lines:
		mol = self.re_line.search(lines)
		while mol:
		    text_list.append(mol.group('text'))
		    mol = self.re_line.search(lines, mol.end())
	    mo = self.re_java.search(comm.detail, mo.end())
	comm.detail = string.join(text_list,'\n')

class SummarySplitter:
    """A formatter that splits comments into summary and detail"""
    __re_summary = r"[ \t\n]*(.*?\.)([ \t\n]|$)"
    def __init__(self):
	self.re_summary = re.compile(SummarySplitter.__re_summary, re.S)
    def parse(self, comm):
	"""Split comment into summary and detail. This method uses a regular
	expression to find the first sentence. If the expression fails then
	summary is the whole comment and detail is None. If there is only one
	sentence then detail is still the full comment (you must compare
	them). It then calls _calculateHasDetail to fill in has_detail."""
	mo = self.re_summary.match(comm.detail)
	if mo: comm.summary = mo.group(1)
	else:
	    comm.summary = comm.detail
	    comm.detail = None
	self._calculateHasDetail(comm)
    def _calculateHasDetail(self, comm):
	"""Template method to fill in the has_detail variable. The default
	algorithm follows set has_detail if:
	 . A summary was found (first sentence)
	 . The summary text is different from the detail text (no sentence)
	 . Enums always have detail (the enum values)
	 . Functions have detail if they have an exception spec
	 . Scopes never have detail since they have their own pages for that
	"""
	decl = comm.decl
	# Basic detail check
	has_detail = comm.detail is not comm.summary
	has_detail = has_detail and comm.detail is not None
	# Always show enums
	has_detail = has_detail or isinstance(decl, AST.Enum)
	# Show functions if they have exceptions
	has_detail = has_detail or (isinstance(decl, AST.Function) and len(decl.exceptions()))
	# Don't show detail for scopes (they have their own pages)
	has_detail = has_detail and not isinstance(decl, AST.Scope)
	# Store:
	comm.has_detail = has_detail

class JavadocFormatter:
    """A formatter that formats comments similar to Javadoc @tags"""
    # @see IDL/Foo.Bar
    __re_see = '@see (([A-Za-z+]+)/)?(([A-Za-z_]+\.?)+)'
    __re_see_line = '^[ \t]*@see[ \t]+(([A-Za-z+]+)/)?(([A-Za-z_]+\.?)+)(\([^)]*\))?([ \t]+(.*))?$'

    def __init__(self):
	"""Create regex objects for regexps"""
	self.re_see = re.compile(JavadocFormatter.__re_see)
	self.re_see_line = re.compile(JavadocFormatter.__re_see_line,re.M)
    def parse(self, comm):
	"""Parse the comm.detail for @tags"""
	comm.detail = self.parse_see(comm.detail)
	comm.summary = self.parse_see(comm.summary)
    def extract(self, regexp, str):
	"""Extracts all matches of the regexp from the text. The MatchObjects
	are returned in a list"""
	mo = regexp.search(str)
	ret = []
	while mo:
	    ret.append(mo)
	    start, end = mo.start(), mo.end()
	    str = str[:start] + str[end:]
	    mo = regexp.search(str, start)
	return str, ret

    def parse_see(self, str):
	if str is None: return str
	str, see = self.extract(self.re_see_line, str)
	mo = self.re_see.search(str)
	while mo:
	    groups, start, end = mo.groups(), mo.start(), mo.end()
	    lang = groups[1] or ''
	    name = string.split(groups[2], '.')
	    entry = config.toc.lookup(name)
	    if entry: tag = "see "+href(entry.link, lang+groups[2])
	    else: tag = "see "+lang+groups[2]
	    str = str[:start] + tag + str[end:]
	    end = start + len(tag)
	    mo = self.re_see.search(str, end)
	# Add @see lines at end
	if len(see):
	    seestr = div('tag-see-header', "See Also:")
	    seelist = []
	    for mo in see:
		groups = mo.groups()
		lang = groups[1] or ''
		name = string.split(groups[2], '.')
		entry = config.toc.lookup(name)
		tag = groups[6] or lang+groups[2]
		if entry: tag = href(entry.link, tag)
		seelist.append(div('tag-see', tag))
	    str = str + seestr + string.join(seelist,'')
	return str

class SectionFormatter:
    """A test formatter"""
    __re_break = '\n[ \t]*\n'

    def __init__(self):
	self.re_break = re.compile(SectionFormatter.__re_break)
    def parse(self, comm):
	comm.detail = self.parse_break(comm.detail)
    def parse_break(self, str):
	if str is None: return str
	mo = self.re_break.search(str)
	while mo:
	    start, end = mo.start(), mo.end()
	    text = '</p><p>'
	    str = str[:start] + text + str[end:]
	    end = start + len(text)
	    mo = self.re_break.search(str, end)
	return '<p>%s</p>'%str



commentParsers = {
    'default' : CommentParser,
}
commentFormatters = {
    'none' : CommentFormatter,
    'ssd' : SSDFormatter,
    'java' : JavaFormatter,
    'summary' : SummarySplitter,
    'javadoc' : JavadocFormatter,
    'section' : SectionFormatter,
}


