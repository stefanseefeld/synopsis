# $Id: Comments.py,v 1.1 2001/02/06 06:55:18 chalky Exp $
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
# Revision 1.1  2001/02/06 06:55:18  chalky
# Initial commit. Support SSD and Java comments. Selection of comments only
# (same as ssd,java formatters in HTML)
#
#

# TODO - Linker/ is only a temporary location for this file.

"""Comment Processor"""
# System modules
import sys, string, re, getopt

# Synopsis modules
from Synopsis.Core import AST, Util

class CommentProcessor (AST.Visitor):
    def process(self, decl):
	"""Process comments for the given declaration"""
    def visitDeclaration(self, decl):
	self.process(decl)

class SSDComments (CommentProcessor):
    """A class that selects only //. comments."""
    __re_star = r'/\*(.*?)\*/'
    __re_ssd = r'^[ \t]*//\. ?(.*)$'
    def __init__(self):
	self.re_star = re.compile(SSDComments.__re_star, re.S)
	self.re_ssd = re.compile(SSDComments.__re_ssd, re.M)
    def process(self, decl):
	text = string.join(map(lambda x:str(x), decl.comments()))
	text = self.parse_ssd(self.strip_star(text))
	decl.comments()[:] = [AST.Comment(text,'',-1)]
    def strip_star(self, str):
	"""Strips all star-format comments from the docstring"""
	mo = self.re_star.search(str)
	while mo:
	    str = str[:mo.start()] + str[mo.end():]
	    mo = self.re_star.search(str)
	return str
    def parse_ssd(self, str):
	return string.join(self.re_ssd.findall(str),'\n')

class JavaComments (CommentProcessor):
    """A class that formats java /** style comments"""
    __re_java = r"/\*\*[ \t]*(?P<text>.*)(?P<lines>\n[ \t]*\*.*)*?(\n[ \t]*)?\*/"
    __re_line = r"\n[ \t]*\*[ \t]*(?P<text>.*)"
    def __init__(self):
	self.re_java = re.compile(JavaComments.__re_java)
	self.re_line = re.compile(JavaComments.__re_line)
    def process(self, decl):
	text = string.join(map(lambda x:str(x), decl.comments()))
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
	decl.comments()[:] = [AST.Comment(text,'',-1)]

processors = {
    'ssd': SSDComments,
    'java': JavaComments,
}
def __parseArgs(args):
    global processor
    languagize = None

    try:
        opts, remainder = getopt.getopt(args, "p:")
    except getopt.error, e:
        sys.stderr.write("Error in arguments: " + e + "\n")
        sys.exit(1)

    for o,a in opts:
	if o == '-p':
	    if processors.has_key(a):
		processor = processors[a]()
	    else:
		print "Available processors:",string.join(processors.keys())
		sys.exit(2)

def process(declarations, types, args):
    global processor
    __parseArgs(args)
    if processor is not None:
	for decl in declarations:
	    decl.accept(processor)
