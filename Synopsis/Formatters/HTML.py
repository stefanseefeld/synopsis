# $Id: HTML.py,v 1.53 2001/01/24 12:50:23 chalky Exp $
#
# This file is a part of Synopsis.
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
# $Log: HTML.py,v $
# Revision 1.53  2001/01/24 12:50:23  chalky
# Fixes to get summary/detail linking again since AST.Visitor changed ...
#
# Revision 1.52  2001/01/24 01:38:36  chalky
# Added docstrings to all modules
#
# Revision 1.51  2001/01/23 00:20:27  chalky
# Added employee.cc demo. Fixed small bug wrt linking to details of variables.
#
# Revision 1.50  2001/01/22 20:14:36  stefan
# forgot two flags in the usage() function
#
# Revision 1.49  2001/01/22 19:54:41  stefan
# better support for help message
#
# Revision 1.48  2001/01/22 17:06:15  stefan
# added copyright notice, and switched on logging
#

# HTML2 Formatter by Stephen Davies
# Based on HTML Formatter by Stefan Seefeld
#
# Generates a page for each Module/Class and an inheritance tree
# Also 3-frame format inspired by Javadoc

# TODO:
# . TOC - Consider storing scoped name tuple instead of colonated name
#   o NOTE: TOC also stores types for some reason. investigate.
# . Unite Nodes/NamespaceBuilder into TOC
"""
HTML formatter. Output specifies a directory. See manual for usage.
"""

import sys, getopt, os, os.path, string, types, errno, stat, re
from Synopsis.Core import AST, Type, Util

# Set this to true if your name is Chalky :)
debug=0

def k2a(keys):
    "Convert a keys dict to a string of attributes"
    return string.join(map(lambda item:' %s="%s"'%item, keys.items()), '')
def rel(frm, to):
    "Find link to to relative to frm"
    frm = string.split(frm, '/'); to = string.split(to, '/')
    for l in range((len(frm)<len(to)) and len(frm) or len(to)):
        if to[0] == frm[0]: del to[0]; del frm[0]
        else: break
    if len(frm) > len(to): to = ['..']*(len(frm)-len(to))+to
    return string.join(to,'/')
def href(ref, label, **keys):
    return '<a href="%s"%s>%s</a>'%(ref,k2a(keys),label)
def name(ref, label): return '<a class="name" name="%s">%s</a>'%(ref,label)
def span(clas, body): return '<span class="%s">%s</span>'%(clas,body)
def div(clas, body): return '<div class="%s">%s</div>'%(clas,body)
def entity(type, body, **keys):
    return '<%s%s>%s</%s>'%(type,k2a(keys),body,type)
def solotag(type, **keys):
    return '<%s%s>'%(type,k2a(keys))
def desc(text):
    block = filter(lambda s: s.text()[0:3] == "//.", text)
    if not len(block): return ""
    block = map(lambda s: s.text()[3:], block)
    return div("desc", string.join(block, '\n'))
def anglebrackets(text):
    """Replace angle brackets with HTML codes"""
    return re.sub('<','&lt;',re.sub('>','&gt;',text))

class FileNamer:
    """A class that encapsulates naming files."""
    def __init__(self):
	if basename is None:
	    print "ERROR: No output directory specified."
	    print "You must specify a base directory with the -o option."
	    sys.exit(1)
	import os.path
	if not os.path.exists(basename):
	    print "Warning: Output directory does not exist. Creating."
	    try:
		os.makedirs(basename, 0755)
	    except EnvironmentError, reason:
		print "ERROR: Creating directory:",reason
		sys.exit(2)
	if stylesheet_file:
	    try:
		# Copy stylesheet in
		stylesheet_new = basename+"/"+stylesheet
		filetime = os.stat(stylesheet_file)[stat.ST_MTIME]
		if not os.path.exists(stylesheet_new) or \
		    filetime > os.stat(stylesheet_new)[stat.ST_MTIME]:
		    fin = open(stylesheet_file,'r')
		    fout = open(stylesheet_new, 'w')
		    fout.write(fin.read())
		    fin.close()
		    fout.close()
	    except EnvironmentError, reason:
		print "ERROR: ", reason
		sys.exit(2)
	if not os.path.isdir(basename):
	    print "ERROR: Output must be a directory."
	    sys.exit(1)
    def nameOfScope(self, scope):
	if not len(scope): return self.nameOfSpecial('global')
	return string.join(scope,'-') + ".html"
    def nameOfFile(self, filetuple):
	return "_file-"+string.join(filetuple,'-')+".html"
    def nameOfIndex(self):
	return "index.html"
    def nameOfSpecial(self, name):
	return "_" + name + ".html"
    def nameOfModuleTree(self):
	return "_modules.html"
    def nameOfModuleIndex(self, scope):
	return "_module_" + string.join(scope, '-') + ".html"
    def chdirBase(self):
	import os
	self.__old_dir = os.getcwd()
	os.chdir(basename)
    def chdirRestore(self):
	import os
	os.chdir(self.__old_dir)
	

def sort(list):
    "Utility func to sort and return the given list"
    list.sort()
    return list

class ScopeSorter:
    """A class that takes a scope and sorts its children by type."""
    def __init__(self, scope=None):
	"Optional scope starts using that AST.Scope"
	if scope: self.set_scope(scope)
    def set_scope(self, scope):
	"Sort children of given scope"
	self.__sections = []
	self.__section_dict = {}
	self.__children = []
	self.__child_dict = {}
	scopename = scope.name()
	for decl in scope.declarations():
	    if isinstance(decl, AST.Forward): continue
	    name, section = decl.name(), self._section_of(decl)
	    if name[:-1] != scopename: continue
	    if not self.__section_dict.has_key(section):
		self.__section_dict[section] = []
		self.__sections.append(section)
	    self.__section_dict[section].append(decl)
	    self.__children.append(decl)
	    self.__child_dict[tuple(name)] = decl
	self._sort_sections()
    def _section_of(self, decl):
	_axs_str = ('','Public ','Protected ','Private ')
	section = string.capitalize(decl.type())
	if decl.accessability != AST.DEFAULT:
	    section = _axs_str[decl.accessability()]+section
	return section
    def _sort_sections(self): pass
    def sort_section_names(self):
	sort(self.__sections)
    def sort_sections(self):
	for children in self.__section_dict.values()+[self.__children]:
	    dict = {}
	    for child in children: dict[child.name()] = child
	    names = dict.keys()
	    sort(names)
	    del children[:]
	    for name in names: children.append(dict[name])
    def child(self, name):
	"Returns the child with the given name. Throws KeyError if not found."
	return self.__child_dict[name]
    def sections(self):
	"Returns a list of available section names"
	return self.__sections
    def children(self, section=None):
	"Returns list of children in given section, or all children"
	if section is None: return self.__children
	if self.__section_dict.has_key(section):
	    return self.__section_dict[section]
	return {}

class Struct:
    "Dummy class. Initialise with keyword args."
    def __init__(self, **keys):
	for name, value in keys.items(): setattr(self, name, value)

class CommentParser:
    """A class that takes a Declaration and sorts its comments out."""
    def __init__(self):
	self._formatters = map(lambda n,c=commentFormatters: c[n](), commentFormatterList)
    def parse(self, decl):
	"""Parses the comments of the given AST.Declaration.
	Returns a struct with vars:
	 summary - one-line summary, detail - detailed info
	"""
	strlist = map(lambda x:str(x), decl.comments())
	comm = Struct(detail=string.join(strlist),summary='')
	map(lambda f,c=comm: f.parse(c), self._formatters)
	return comm

class CommentFormatter:
    """A class that takes a comment Struct and formats its contents."""

    def parse(self, comm):
	"""Parse the comment struct"""
	pass

class SSDFormatter:
    """A class that strips //.'s from the start of lines in detail"""
    __re_ssd = '^[ \t]*//\. ?(.*)$'
    def __init__(self):
	self.re_ssd = re.compile(SSDFormatter.__re_ssd, re.M)
    def parse(self, comm):
	comm.detail = self.parse_ssd(comm.detail)
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
	mo = self.re_summary.match(comm.detail)
	if mo: comm.summary = mo.group(1)
	else:
	    comm.summary = comm.detail
	    comm.detail = None

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
	    entry = toc.lookup(name)
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
		entry = toc.lookup(name)
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

class CommentDictionary:
    """This class just maintains a mapping from declaration to comment, since
    any particular comment is required at least twice. Upon initiation, an
    instance of this class installs itself in the global namespace as
    "comments"."""
    def __init__(self):
	self.__dict = {}
	self._parser = commentParsers[commentParser]()
	global comments
	comments = self
    def commentForName(self, name):
	if self.__dict.has_key(name): return self.__dict[name]
	return None
    def commentFor(self, decl):
	"Returns a comment struct (@see CommentParser) for given decl"
	key = decl.name()
	if self.__dict.has_key(key): return self.__dict[key]
	self.__dict[key] = comment = self._parser.parse(decl)
	return comment
    __getitem__ = commentFor

class TocEntry:
    """Struct for an entry in the table of contents.
    Vars: link, lang, type (all strings)
    Also: name (scoped)"""
    def __init__(self, name, link, lang, type):
	self.name = name
	self.link = link
	self.lang = lang
	self.type = type

class Linker:
    """Class that translates AST.Declarations into links"""
    def __init__(self):
	global linker
	linker = self
    def link(self, decl):
	if isinstance(decl, AST.Scope):
	    # This is a class or module, so it has its own file
	    return filer.nameOfScope(decl.name())
	# Assume parent scope is class or module, and this is a <A> name in it
	filename = filer.nameOfScope(decl.name()[:-1])
	return filename + "#" + decl.name()[-1]
    __call__ = link

class TableOfContents(AST.Visitor):
    """
    Maintains a dictionary of all declarations which can be looked up to create
    cross references. Names are fully scoped.
    """
    def __init__(self):
	global toc
	toc = self
	self.__toc = {}
	#self.__global = None
    
    def lookup(self, name):
	name = tuple(name)
        if self.__toc.has_key(name): return self.__toc[name]
	if debug and len(name) > 1:
	    print "Warning: TOC lookup of",name,"failed!"
        return None

    def size(self): return len(self.__toc)

    __getitem__ = lookup

    def insert(self, entry): self.__toc[tuple(entry.name)] = entry

    def store(self, file):
        """store the table of contents into a file, such that it can be used later when cross referencing"""
        fout = open(file, 'w')
        for name in self.__toc.keys():
            scopedname = string.join(name, "::")
            lang = self.__toc[tuple(name)].lang
            link = os.path.join(basename, self.__toc[tuple(name)].link)
            fout.write(scopedname + "," + lang + "," + link + "\n")
            
    def load(self, file):
        fin = open(os.path.join("..", file), 'r')
        line = fin.readline()
        while line:
            scopedname, lang, link = string.split(line, ",")
            name = string.split(scopedname, "::")
            entry = TocEntry(name, os.path.join("..", link), lang, "decl")
            self.insert(entry)
            line = fin.readline()

    #def globalns(self): return self.__global
    #def set_global(self, ns): self.__global = ns

    def visitAST(self, ast):
	for decl in ast.declarations():
	    decl.accept(self)
    def visitDeclaration(self, decl):
	entry = TocEntry(decl.name(), linker(decl), decl.language(), "decl")
	self.insert(entry)
    def visitForward(self, decl):
	pass
    #def visitScope(self, scope):
    #	self.visitDeclaration(scope)
    #	for decl in scope.declarations():
    #	    decl.accept(self)
    #def visitEnum(self, enum):
    #	self.visitDeclaration(enum)
    #	for enumor in enum.enumerators():
    #	    enumor.accept(self)

class FileTree(AST.Visitor):
    """Maintains a tree of directories and files"""
    def __init__(self):
	global fileTree
	fileTree = self
	self.__files = {}

    def buildTree(self):
	"Takes the visited info and makes a tree of directories and files"
	# Split filenames into lists of directories
	files = []
	for file in self.__files.keys():
	    files.append(string.split(file, os.sep))
	self.__root = Struct(path=(), children={})
	process_list = [self.__root]
	while len(process_list):
	    # Get node
	    node = process_list[0]
	    del process_list[0]
	    for file in files:
		if len(file) <= len(node.path) or tuple(file[:len(node.path)]) != node.path:
		    continue
		child_path = tuple(file[:len(node.path)+1])
		if node.children.has_key(child_path): continue
		child = Struct(path=child_path, children={})
		node.children[child_path] = child
		if len(child_path) < len(file):
		    process_list.append(child)
		else:
		    fname = string.join(file, os.sep)
		    child.decls = self.__files[fname]

    def root(self):
	"""Returns the root node in the file tree.
	Each node is a Struct with the following members:
	 path - tuple of dir or filename (eg: 'Prague','Sys','Thread.hh')
	 children - dict of children by their path tuple
	Additionally, leaf nodes have the attribute:
	 decls - dict of declarations in this file by scoped name
	"""
	return self.__root

    ### AST Visitor

    def visitAST(self, ast):
	for decl in ast.declarations():
	    decl.accept(self)
    def visitDeclaration(self, decl):
	file = decl.file()
	if not file: print "Decl",decl,"has no file."
	if not self.__files.has_key(file):
	    self.__files[file] = {}
	self.__files[file][decl.name()] = decl
    def visitForward(self, decl):
	# Don't include (excluded) forward decls in file listing
	pass
    def visitClass(self, scope):
	self.visitDeclaration(scope)
	# Only nested classes may be in different files
	for decl in scope.declarations():
	    if isinstance(decl, AST.Class):
		self.visitClass(decl)
    def visitMetaModule(self, scope):
	for decl in scope.declarations():
	    decl.accept(self)
	


class ClassTree(AST.Visitor):
    """Maintains a tree of classes directed by inheritance"""
    def __init__(self):
	global classTree
	classTree = self
	self.__superclasses = {}
	self.__subclasses = {}
	self.__classes = []

    def add_inheritance(self, supername, subname):
	supername, subname = tuple(supername), tuple(subname)
	if not self.__subclasses.has_key(supername):
	    subs = self.__subclasses[supername] = []
	else:
	    subs = self.__subclasses[supername]
	if subname not in subs:
	    subs.append(subname)
	if not self.__superclasses.has_key(subname):
	    sups = self.__superclasses[subname] = []
	else:
	    sups = self.__superclasses[subname]
	if supername not in sups:
	    sups.append(supername)

    def subclasses(self, classname):
	classname = tuple(classname)
	if self.__subclasses.has_key(classname):
	    return sort(self.__subclasses[classname])
	return []
    def superclasses(self, classname):
	classname = tuple(classname)
	if self.__superclasses.has_key(classname):
	    return sort(self.__superclasses[classname])
	return []

    def classes(self): return sort(self.__classes)
    def add_class(self, name):
	name = tuple(name)
	if name not in self.__classes:
	    self.__classes.append(tuple(name))

    def visitAST(self, ast):
	for decl in ast.declarations():
	    decl.accept(self)
    def visitScope(self, scope):
	for decl in scope.declarations():
	    decl.accept(self)
    def visitClass(self, clas):
	name = clas.name()
	self.add_class(name)
	for inheritance in clas.parents():
	    parent = inheritance.parent()
	    if hasattr(parent, 'name'):	
		self.add_inheritance(parent.name(), name)
	for decl in clas.declarations():
	    decl.accept(self)

class BaseFormatter(Type.Visitor, AST.Visitor):
    """
    A Formatter with common base functionality between summary and detail
    sections.
    """
    def __init__(self, toc):
        self.__os = None
        self.__toc = toc
        self.__scope = []

    def toc(self): return self.__toc
    def scope(self): return self.__scope
    def write(self, text): self.__os.write(text)
    def set_scope(self, scope): self.__scope = list(scope)
    def set_ostream(self, os): self.__os = os
    
    # Access to generated values
    def type_ref(self): return self.__type_ref
    def type_label(self): return self.__type_label
    def declarator(self): return self.__declarator
    def parameter(self): return self.__parameter

    def reference(self, ref, label, **keys):
        """reference takes two strings, a reference (used to look up the symbol and generated the reference),
        and the label (used to actually write it)"""
	raise TypeError, "reference is deprecated"
	if ref is None: return label
	try:
	    info = toc.lookup(ref)
	except:
	    print "*******\n*** ref is",ref
	    raise
	if info is None: return span('type', label)
        return apply(href, (info.link, label), keys)

    def referenceName(self, name, label=None, **keys):
	"""Same as reference but takes a tuple name"""
	if not label: label = Util.ccolonName(name, self.scope())
	entry = toc[name]
	if entry: return apply(href, (entry.link, label), keys)
	#return label and span('type', label) or ''
	return label or ''

    def label(self, ref, label=None):
	"""Create a label for the given scoped reference name"""
	if label is None: label = ref
	# some labels are templates with <>'s
        entry = toc[ref]
        label = anglebrackets(Util.ccolonName(label, self.scope()))
	if entry is None: return label #return Util.ccolonName(label)
	location = entry.link
	index = string.find(location, '#')
	if index >= 0: location = location[index+1:]
        return location and name(location, label) or label

    def formatModifiers(self, modifiers):
	"Returns a HTML string from the given list of modifiers"
	keyword = lambda m,span=span: span("keyword", m)
	return string.join(map(keyword, modifiers))

    def formatType(self, typeObj):
	"Returns a reference string for the given type object"
	if typeObj is None: return "(unknown)"
        typeObj.accept(self)
        return self.type_label()

    def formatParameters(self, parameters):
        return string.join(map(self.getParameterText, parameters), ", ")

    def getDeclaratorName(self, declarator):
	"""
	Returns the scoped, sized name of the given declarator.
	EG: ('Warsaw', 'SomeModule', 'foobar[3][2]')
	"""
	declarator.accept(self)
	return self.__declarator

    def getParameterText(self, parameter):
	""" Returns the full HTML for the given parameter.  """
	parameter.accept(self)
	return self.__parameter

    def formatOperationExceptions(self, operation): pass

    #################### Type Visitor ###########################################

    def visitBaseType(self, type):
        self.__type_label = self.referenceName(type.name())
        
    def visitUnknown(self, type):
        self.__type_label = self.referenceName(type.name())
        #self.__type_label = Util.ccolonName(type.name(), self.scope())
        
    def visitDeclared(self, type):
        self.__type_label = self.referenceName(type.name())
        #self.__type_label = Util.ccolonName(type.name(), self.scope())
        
    def visitModifier(self, type):
        alias = self.formatType(type.alias())
	#pre = string.join(map(lambda x:x+" ", type.premod()), '')
	pre = string.join(map(lambda x:x+"&nbsp;", type.premod()), '')
	post = string.join(type.postmod(), '')
        self.__type_label = "%s%s%s"%(pre,alias,post)
            
    def visitParametrized(self, type):
	if type.template():
	    type_label = self.referenceName(type.template().name())
	else:
	    type_label = "(unknown)"
        parameters_label = []
        for p in type.parameters():
            #p.accept(self)
            #parameters_ref.append(self.__type_ref)
            parameters_label.append(self.formatType(p))
        self.__type_label = type_label + "&lt;" + string.join(parameters_label, ", ") + "&gt;"

    def visitTemplate(self, type):
	self.__type_label = "template&lt;%s&gt;"%(
	    string.join(map(lambda x:"typename "+x, map(self.formatType, type.parameters())), ",")
	)

    def visitFunctionType(self, type):
	ret = self.formatType(type.returnType())
	params = map(self.formatType, type.parameters())
	pre = string.join(type.premod(), '')
	self.__type_label = "%s(%s)(%s)"%(ret,pre,string.join(params, ", "))

    #################### AST Visitor ############################################
    def visitTypedef(self, typedef):
	type = self.formatType(typedef.alias())
	self.writeSectionItem(type, self.label(typedef.name()), typedef)

    def visitConst(self, const):
	type = self.formatType(const.ctype())
        name = self.label(const.name()) + " = " + const.value()
	self.writeSectionItem(type, name, const)

    def visitDeclarator(self, node):
	""" Calculates the name for the given paramter.  """
        self.__declarator = node.name()
        for i in node.sizes():
            self.__declarator[-1] = self.__declarator[-1] + "[" + str(i) + "]"

    def visitParameter(self, parameter):
	""" Calculates the HTML for the given parameter.  """
	str = []
	keyword = lambda m,span=span: span("keyword", m)
	str.extend(map(keyword, parameter.premodifier()))
	typestr = self.formatType(parameter.type())
	if typestr: str.append(typestr)
	str.extend(map(keyword, parameter.postmodifier()))
        if len(parameter.identifier()) != 0:
            str.append(span("variable", parameter.identifier()))
            if len(parameter.value()) != 0:
                str.append(" = " + span("value", parameter.value()))
	self.__parameter = string.join(str)

    def visitEnum(self, enum):
	type, name = self.label(enum.name()), []
        for enumerator in enum.enumerators():
	    name.append(enumerator.name()[-1])
	name = string.join(name, ', ')
	self.writeSectionItem(type, name, enum)
	
    def visitOperation(self, oper):
	premod = self.formatModifiers(oper.premodifier())
	type = self.formatType(oper.returnType())
	name = self.label(oper.name(), oper.realname())
	if oper.language() == 'C++' and len(oper.realname())>1:
	    if oper.realname()[-1] == oper.realname()[-2]: type = '<i>constructor</i>'
	    elif oper.realname()[-1] == "~"+oper.realname()[-2]: type = '<i>destructor</i>'
	    elif oper.realname()[-1] == "(conversion)":
		name = "(%s)"%type
	params = self.formatParameters(oper.parameters())
	postmod = self.formatModifiers(oper.postmodifier())
	raises = self.formatOperationExceptions(oper)
	type = '%s %s'%(premod,type)
        if oper.type() == "attribute": name = '%s %s %s'%(name, postmod, raises)
	else: name = '%s(%s) %s %s'%(name, params, postmod, raises)
	self.writeSectionItem(type, name, oper)
    visitFunction = visitOperation

    def visitVariable(self, variable):
	# TODO: deal with sizes
        type = self.formatType(variable.vtype())
	self.writeSectionItem(type, self.label(variable.name()), variable)


    # These are overridden in {Summary,Detail}Formatter
    def writeSectionStart(self, heading): pass
    def writeSectionEnd(self, heading): pass
    def writeSectionItem(self, type, name, node): pass

class SummaryFormatter(BaseFormatter):
    def __init__(self, toc):
	BaseFormatter.__init__(self, toc)
	self.__link_detail = 0

    def set_link_detail(self, boolean):
	self.__link_detail = boolean

    def label(self, ref, label=None):
	if label is None: label = ref
	if self.__link_detail: return span('name',self.referenceName(ref, Util.ccolonName(label, self.scope())))
	return BaseFormatter.label(self, ref, label)
	
    def getSummary(self, node):
	comment = comments[node].summary
	if len(comment): return div('summary', comment)
	else: return ''

    def formatOperationExceptions(self, oper):
        if len(oper.exceptions()):
            return self.referenceName(oper.name(), " raises")
	return ''

    def writeSectionStart(self, heading):
	str = '<table width="100%%">'\
	'<col width="100px"><col width="100%%">'\
	'<tr><td colspan="2" class="heading">%s</td></tr>'
	self.write(str%heading)

    def writeSectionEnd(self, heading):
	str = '</table><br>'
	self.write(str)

    def writeSectionItem(self, type, name, node):
	if type is None:
	    str = '<tr><td colspan="2">%s%s</td></tr>'
	    self.write(str%(name,self.getSummary(node)))
	else:
	    str = '<tr><td valign="top">%s</td><td>%s%s</td></tr>'
	    self.write(str%(type,name,self.getSummary(node)))

    def visitModule(self, module):
	name = self.referenceName(module.name())
	self.writeSectionItem(None, name, module)

    def visitClass(self, clas):
	name = self.referenceName(clas.name())
	self.writeSectionItem(None, name, clas)

    def visitEnum(self, enum):
	type, name = self.label(enum.name()), []
        for enumerator in enum.enumerators():
	    name.append(enumerator.name()[-1])
	name = string.join(name, ', ')
	self.writeSectionItem(type, name, enum)


class DetailFormatter(BaseFormatter):
    def __init__(self, toc):
	BaseFormatter.__init__(self, toc)

    def getDetail(self, node):
	comment = comments[node].detail
	if comment and len(comment): return '<br>'+div('desc', comment)
	else: return ''

    def formatParent(self, node):
	# Figure out the enclosing scope
	scope, text = [], []
	for name in node.name()[:-1]:
	    scope.append(name)
	    text.append(self.referenceName(scope))
	text.append(node.name()[-1])
	return string.join(text, "::")

    def formatInheritance(self, inheritance):
	if hasattr(inheritance, 'name'):
	    return '%s %s'%( self.formatModifiers(inheritance.attributes()),
		self.referenceName(inheritance.parent().name()))
	return '%s %s'%( self.formatModifiers(inheritance.attributes()),
	    self.formatType(inheritance.parent()))

    def writeSectionStart(self, heading):
	str = '<table width="100%%">'\
	'<tr><td colspan="2" class="heading">%s</td></tr></table>'
	self.write(str%heading)

    def writeSectionItem(self, text1, text2, node):
	str = '%s %s<br>%s<hr>'
	self.write(str%(text1, text2, self.getDetail(node)))

    def visitModule(self, module):
	# Module details are only printed at the top of their page
	type = string.capitalize(module.type())
	name = self.formatParent(module)
	self.write(entity('h1', "%s %s"%(type, name)))

	# Print any comments for this module
	comment = comments[module].detail or comments[module].summary
	if comment: self.write(div('desc',comment)+'<br><br>')

    def visitClass(self, clas):
	# Class details are only printed at the top of their page
	type = string.capitalize(clas.type())
	name = self.formatParent(clas)
	self.write(entity('h1', "%s %s"%(type, name)))

	# Print template
	templ = clas.template()
	if templ:
	    self.write(entity('h2', "template &lt;%s&gt;"%string.join(map(self.formatType, templ.parameters()),", ")))

	# Print filename
	file = string.split(clas.file(), os.sep)
	while len(file) and file[0] == '..': del file[0]
	file = string.join(file, os.sep)
	self.write("Defined in: "+href(filer.nameOfFile(string.split(clas.file(),os.sep)),file,target='contents')+"<br>")

	# Print any comments for this class
	comment = comments[clas].detail or comments[clas].summary
	if comment: self.write(div('desc',comment)+'<br><br>')

	# Print out a list of the parents
	if clas.parents():
	    self.write('<div class="superclasses">\n')
	    self.write("Superclasses: ")
	    parents = map(self.formatInheritance, clas.parents())
	    self.write(string.join(parents, ", "))
	    self.write("</div>\n")

	# Print subclasses
	subs = classTree.subclasses(clas.name())
	if subs:
	    self.write("Known subclasses: ")
	    refs = map(self.referenceName, subs)
	    self.write(string.join(refs, ", "))
	
	self.write("<br>")

    def formatOperationExceptions(self, oper):
        if len(oper.exceptions()):
            raises = span("keyword", "raises")
            exceptions = []
            for exception in oper.exceptions():
                exceptions.append(self.referenceName(exception.name()))
	    exceptions = span("raises", string.join(exceptions, ", "))
            return '%s (%s)'%(raises, exceptions)
	return ''

    def visitEnum(self, enum):
        self.write(span("keyword", "enum ") + self.label(enum.name()) + "\n")
        if len(enum.comments()): self.write("\n" + desc(enum.comments()) + "\n")            
        self.write("<div class=\"enum\"><ul>\n")
        for enumerator in enum.enumerators():
            enumerator.accept(self)
            self.write("<br>")
        self.write("</ul></div>\n")

    def visitEnumerator(self, enumerator):
	self.write('<li>')
        self.write(self.label(enumerator.name()))
        if len(enumerator.value()):
            self.write(" = " + span("value", enumerator.value()))
	self.write(desc(enumerator.comments()))
	self.write('</li>')



class Paginator:
    """Visitor that takes the Namespace hierarchy and splits it into pages"""
    def __init__(self, root):
	self.__root = root
	self.summarizer = SummaryFormatter(toc)
	self.detailer = DetailFormatter(toc)
	self.__os = None
	self.__scope = []
	self.sorter = ScopeSorter(root)
	
	# Resolve start namespace using global namespace var
	self.__start = self.__root
	scope_names = string.split(namespace, "::")
	scope = []
	for scope_name in scope_names:
	    if not scope_name: break
	    scope.append(scope_name)
	    try:
		child = self.sorter.child(tuple(scope))
		if isinstance(child, AST.Scope):
		    self.__start = child
		    self.sorter.set_scope(self.__start)
		else:
		    raise TypeError, 'Not a Scope'
	    except:
		# Don't continue if scope traversal failed!
		import traceback
		traceback.print_exc()
		print "Fatal: Couldn't find child scope",scope
		sys.exit(3)

    def scope(self): return self.__scope
    def write(self, text): self.__os.write(text)
    
    def process(self):
	"""Creates all pages"""
	self.createNamespacePages()
	self.createModuleIndexFile()
	self.createModuleIndexes()
	self.createFramesFile()
	self.createInheritanceTree()
	self.createFileTree()

    def createNamespacePages(self):
	"""Creates a page for every Namespace"""
	self.__namespaces = [self.__start]
	while self.__namespaces:
	    ns = self.__namespaces.pop(0)
	    self.processNamespacePage(ns)

    def createModuleIndexFile(self):
	"""Create a page with an index of all modules"""
	self.startFile(filer.nameOfModuleTree(), "Module Index")
	tree = href(filer.nameOfSpecial('tree'), 'Tree', target='main')
	ftree = href(filer.nameOfSpecial('FileTree'), 'Files')
	mtree = entity('b', "Modules")
	self.write('%s | %s | %s<br>'%(mtree, ftree, tree))
	self.indexModule(self.__start)
	self.endFile()

    def createModuleIndexes(self):
	"""Creates indexes for all modules"""
	self.__namespaces = [self.__start]
	while self.__namespaces:
	    ns = self.__namespaces.pop(0)
	    self.processNamespaceIndex(ns)

    def createFramesFile(self):
	"""Create a frames index file"""
	me = filer.nameOfIndex()
	self.startFile(me, "Synopsis - Generated Documentation", body='')
	findex = rel(me, filer.nameOfModuleTree())
	fmindex = rel(me, filer.nameOfModuleIndex(self.__start.name()))
	fglobal = rel(me, filer.nameOfScope(self.__start.name()))
	frame1 = solotag('frame', name='index', src=findex)
	frame2 = solotag('frame', name='contents', src=fmindex)
	frame3 = solotag('frame', name='main', src=fglobal)
	frameset1 = entity('frameset', frame1+frame2, rows="30%,*")
	frameset2 = entity('frameset', frameset1+frame3, cols="200,*")
	self.write(frameset2)
	self.endFile()

    def createInheritanceTree(self):
	"""Creates a file with the inheritance tree"""
	classes = classTree.classes()
	def root(name):
	    try:
		return not classTree.superclasses(name)
	    except:
		print "Filtering",Util.ccolonName(name),"failed:"
		return 0
	roots = filter(root, classes)
	self.detailer.set_scope(self.__start.name())

	self.startFile(filer.nameOfSpecial('tree'), "Synopsis - Class Hierarchy")
	self.write(entity('h1', "Inheritance Tree"))
	self.write('<ul>')
	map(self.processClassInheritance, roots)
	self.write('</ul>')
	self.endFile()   

    def processClassInheritance(self, name):
	self.write('<li>')
	self.write(self.detailer.referenceName(name))
	parents = classTree.superclasses(name)
	if parents:
	    self.write(' <i>(%s)</i>'%string.join(map(Util.ccolonName, parents), ", "))
	subs = classTree.subclasses(name)
	if subs:
	    self.write('<ul>')
	    map(self.processClassInheritance, subs)
	    self.write('</ul>\n')
	self.write('</li>')

    def processNamespacePage(self, ns):
	"""Creates a page for the given namespace"""
	details = {} # A hash of lists of detailed children by type
	sections = [] # a list of detailed sections

	# Open file and setup scopes
	self.startFileScope(ns.name())
	self.sorter.set_scope(ns)
	self.sorter.sort_section_names()

	# Write heading
	if ns is self.__root: 
	    self.write(entity('h1', "Global Namespace"))
	else:
	    # Use the detailer to print an appropriate heading
	    ns.accept(self.detailer)

	# Loop throught all the types of children
	self.printNamespaceSummaries(ns, details, sections)
	self.printNamespaceDetails(details, sections)
	self.endFile()

	# Queue child namespaces
	for child in self.sorter.children():
	    if isinstance(child, AST.Scope):
		self.__namespaces.append(child)

    def printNamespaceSummaries(self, ns, details, sections):
	"Print out the summaries from the given ns and note detailed items"
	for section in self.sorter.sections():
	    if section[-1] == 's': heading = section+'es Summary:'
	    else: heading = section+'s Summary:'
	    self.summarizer.writeSectionStart(heading)
	    # Get a list of children of this type
	    children = self.sorter.children(section)
	    for child in children:
		# Check if need to add to detail list
		has_detail = comments[child].detail is not comments[child].summary
		has_detail = has_detail and comments[child].detail is not None
		has_detail = has_detail or isinstance(child, AST.Enum)
		has_detail = has_detail or (isinstance(child, AST.Function) and len(child.exceptions()))
		if has_detail and not isinstance(child, AST.Scope):
		    if not details.has_key(section):
			details[section] = []
			sections.append(section)
		    details[section].append(child)
		    self.summarizer.set_link_detail(1)
		# Print out summary for the child
		child.accept(self.summarizer)
		self.summarizer.set_link_detail(0)
	    self.summarizer.writeSectionEnd(heading)

    def printNamespaceDetails(self, details, sections):
	"Print out the details from the given dict of lists"
	for section in sections:
	    heading = section+' Details:'
	    self.detailer.writeSectionStart(heading)
	    # Get the sorted list of children of this type
	    for child in details[section]:
		child.accept(self.detailer)
	    self.detailer.writeSectionEnd(heading)

    def indexModule(self, ns):
	"Write a link for this module and recursively visit child modules."
	# Print link to this module
	name = Util.ccolonName(ns.name()) or "Global Namespace"
	#if not name: name = "Global Namespace"
	link = filer.nameOfModuleIndex(ns.name())
	self.write(href(link, name, target='contents') + '<br>')
	# Add children
	self.sorter.set_scope(ns)
	self.sorter.sort_sections()
	for child in self.sorter.children():
	    if isinstance(child, AST.Module):
		self.indexModule(child)

    def processNamespaceIndex(self, ns):
	self.detailer.set_scope(ns.name())
	self.sorter.set_scope(ns)
	self.sorter.sort_section_names()
	self.sorter.sort_sections()

	# Create file
	name = Util.ccolonName(ns.name()) or "Global Namespace"
	fname = filer.nameOfModuleIndex(ns.name())
	self.startFile(fname, name+" Index")
	link = href(filer.nameOfScope(ns.name()), name, target='main')
	self.write(entity('b', link+" Index"))

	# Make script to switch main frame upon load
	script = '<!--\nwindow.parent.frames["main"].location="%s";\n-->'
	script = script%filer.nameOfScope(ns.name())
	self.write(entity('script', script, language='Javascript'))

	# Loop throught all the types of children
	for section in self.sorter.sections():
	    if section[-1] == 's': heading = section+'es'
	    else: heading = section+'s'
	    heading = '<br>'+entity('i', heading)+'<br>'
	    # Get a list of children of this type
	    for child in self.sorter.children(section):
		# Print out summary for the child
		if isinstance(child, AST.Scope):
		    if heading:
			self.write(heading)
			heading = None
		    self.write(self.detailer.referenceName(child.name(), target='main'))
		    self.write('<br>')
	self.endFile()

	# Queue child namespaces
	for child in self.sorter.children():
	    if isinstance(child, AST.Scope):
		self.__namespaces.append(child)
    
    def createFileTree(self):
	fname = filer.nameOfSpecial('FileTree')
	self.startFile(fname, "File Tree")
	tree = href(filer.nameOfSpecial('tree'), 'Tree', target='main')
	ftree = entity('b', 'Files')
	mtree = href(filer.nameOfModuleTree(), 'Modules')
	self.write('%s | %s | %s<br>'%(mtree, ftree, tree))
	# recursively visit all nodes
	self.processFileTreeNode(fileTree.root())
	self.endFile()
	# recursively create all node pages
	self.processFileTreeNodePage(fileTree.root())

    def processFileTreeNode(self, node):
	if hasattr(node, 'decls'):
	    self.write(href(filer.nameOfFile(node.path),node.path[-1],target='contents'))
	    return
	children = node.children.values()
	if len(node.path):
	    if node.path[-1] == '..':
		# Don't print anything..
		for child in children:
		    self.processFileTreeNode(child)
		    self.write('<br>')
		return
	    self.write(node.path[-1]+os.sep)
	if len(children):
	    self.write('<div class="files">')
	    for child in children:
		self.processFileTreeNode(child)
		self.write('<br>')
	    self.write('</div>')
	
    def processFileTreeNodePage(self, node):
	for child in node.children.values():
	    self.processFileTreeNodePage(child)
	if not hasattr(node, 'decls'): return

	fname = filer.nameOfFile(node.path)
	name = list(node.path)
	while len(name) and name[0] == '..': del name[0]
	self.startFile(fname, string.join(name, os.sep))
	self.write(entity('b', string.join(name, os.sep))+'<br>')
	for name, decl in node.decls.items():
	    # TODO make this nicer :)
	    entry = toc[name]
	    if not entry: print "no entry for",name
	    else:
		# Print link to declaration's page
		if isinstance(decl, AST.Function):
		    self.write(div('href',href(entry.link,anglebrackets(Util.ccolonName(decl.realname())),target='main')))
		else:
		    self.write(div('href',href(entry.link,Util.ccolonName(name),target='main')))
		# Print comment
		self.write(self.summarizer.getSummary(node.decls[name]))
	    


    def startFileScope(self, scope):
	"Start a new file from the given scope"
	fname = filer.nameOfScope(scope)
	title = string.join(scope)
	self.startFile(fname, title)
	self.summarizer.set_ostream(self.__os)
	self.summarizer.set_scope(scope)
	self.detailer.set_ostream(self.__os)
	self.detailer.set_scope(scope)

    def startFile(self, filename, title, body='<body>'):
	"Start a new file with given filename, title and body"
	self.__filename = filename
	self.__os = open(self.__filename, "w")
	self.write("<html><head>\n")
	self.write(entity('title','Synopsis - '+title))
	if len(stylesheet):
	    self.write(entity('link', '', rel='stylesheet', href=stylesheet))
	self.write("</head>%s\n"%body)

    def endFile(self, body='</body>'):
	"Close the file using given close body tag"
	self.write("%s</html>\n"%body)
	self.__os.close()

def usage():
    """Print usage to stdout"""
    print \
"""
  -o <dir>                             Output directory, created if it doesn't exist.
  -s <filename>                        Filename of stylesheet in output directory
  -S <filename>                        Filename of stylesheet to copy
                                       If this is newer than the one in the output directory then it
		                       is copied over it.
  -n <namespace>                       Namespace to output
  -c <formatter>                       Comment formatter to use
                                       - default Nothing
		                       - ssd     Filters for and strips //. comments
		                       - javadoc @tag style comments
                                       - section test section breaks.
		                       You may use multiple -f options
  -t <filename>                        Generate a table of content and write it to <filename>
  -r <filename>                        merge in table of content from <filename> and use it to resolve external symbols"""

def __parseArgs(args):
    global basename, stylesheet, namespace, commentParser, stylesheet_file, toc_out, toc_in
    global commentFormatterList
    basename = None
    stylesheet = ""
    stylesheet_file = None
    namespace = ""
    commentParser = "default"
    toc_out = ""
    toc_in = []
    commentFormatterList = []
    try:
        opts,remainder = Util.getopt_spec(args, "ho:s:n:c:S:t:r:")
    except Util.getopt.error, e:
        sys.stderr.write("Error in arguments: " + e + "\n")
        sys.exit(1)

    for opt in opts:
        o,a = opt
        if o == "-o":
            basename = a #open(a, "w")
        elif o == "-s":
            stylesheet = a
	elif o == "-S":
	    stylesheet_file = a
	elif o == "-n":
	    namespace = a
	elif o == "-c":
	    if commentFormatters.has_key(a):
		commentFormatterList.append(a)
	    else:
		print "Available comment formatters:",string.join(commentFormatters.keys(), ', ')
		sys.exit(1)
        elif o == "-t":
            toc_out = a
        elif o == "-r":
            toc_in.append(a)
	elif o == "-h":
	    usage()
	    sys.exit(1)

def format(types, declarations, args):
    global basename, stylesheet, toc, filer, toc_out, toc_in
    __parseArgs(args)

    # Create the file namer
    filer = FileNamer()
    filer.chdirBase()

    # Create the Comments Dictionary
    CommentDictionary()

    # Create the Linker
    Linker()

    # Create the Class Tree
    ClassTree()

    # Create the File Tree
    FileTree()

    # Create table of contents index
    TableOfContents()
    # load external references from toc files, if any
    for t in toc_in: toc.load(t)

    if debug: print "HTML Formatter: Initialising TOC"
    # Add all declarations to the namespace tree
    for d in declarations:
        d.accept(toc)
	d.accept(classTree)
	d.accept(fileTree)

    if debug: print "TOC size:",toc.size()

    fileTree.buildTree()
    
    if debug: print "HTML Formatter: Writing Pages..."
    # Create the pages
    # TODO: have synopsis pass an AST with a "root" node to formatters.
    root = AST.Module('',-1,"C++","Global",())
    root.declarations()[:] = declarations
    Paginator(root).process()
    if debug: print "HTML Formatter: Done!"

    filer.chdirRestore()
    if len(toc_out): toc.store(toc_out)

