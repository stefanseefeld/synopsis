# $Id: Part.py,v 1.30 2002/11/01 03:39:20 chalky Exp $
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
# $Log: Part.py,v $
# Revision 1.30  2002/11/01 03:39:20  chalky
# Cleaning up HTML after using 'htmltidy'
#
# Revision 1.29  2002/10/28 08:16:52  chalky
# Undo previous table change. Put non-breaking spaces in first column instead
#
# Revision 1.28  2002/10/28 06:13:49  chalky
# Fix summary display: templates use special div, use nested table to fix
# formatting of first column
#
# Revision 1.27  2002/10/27 12:05:17  chalky
# Support putting the identifier in the right place in funcptr parameters.
#
# Revision 1.26  2002/10/27 08:40:36  chalky
# Oops, broke the templates on their own line thing
#
# Revision 1.25  2002/10/26 04:20:35  chalky
# Oops, limit constructor detection to functions
#
# Revision 1.24  2002/10/26 04:17:58  chalky
# Show templates on previous line. Hide constructors in base class. Commas
# between inherited members
#
# Revision 1.23  2002/10/20 15:38:08  chalky
# Much improved template support, including Function Templates.
#
# Revision 1.22  2002/03/14 00:19:47  chalky
# Added demo of template specializations, and fixed HTML formatter to deal with
# angle brackets in class names :)
#
# Revision 1.21  2001/07/19 04:03:05  chalky
# New .syn file format.
#
# Revision 1.20  2001/07/17 01:49:44  chalky
# Fixed names of strategies for defaults
#
# Revision 1.19  2001/07/15 08:28:43  chalky
# Added 'Inheritance' page Part
#
# Revision 1.18  2001/07/15 06:41:57  chalky
# Factored summarizer and detailer into 'Parts', and added a separate one for
# the top of the page (Heading)
#
# Revision 1.17  2001/07/10 05:08:28  chalky
# Refactored ASTFormatters into FormatStrategies, and simplified names all round
#
# Revision 1.16  2001/07/10 02:55:51  chalky
# Better comments in some files, and more links work with the nested layout
#
# Revision 1.15  2001/07/05 05:39:58  stefan
# advanced a lot in the refactoring of the HTML module.
# Page now is a truely polymorphic (abstract) class. Some derived classes
# implement the 'filename()' method as a constant, some return a variable
# dependent on what the current scope is...
#
# Revision 1.14  2001/07/04 08:17:16  uid20151
# Change the strategies to return just one string, and insert their own <TD>
# when necessary
#
# Revision 1.13  2001/06/28 07:22:18  stefan
# more refactoring/cleanup in the HTML formatter
#
# Revision 1.12  2001/06/26 04:32:15  stefan
# A whole slew of changes mostly to fix the HTML formatter's output generation,
# i.e. to make the output more robust towards changes in the layout of files.
#
# the rpm script now works, i.e. it generates source and binary packages.
#
# Revision 1.11  2001/06/11 10:37:49  chalky
# Better grouping support
#
# Revision 1.10  2001/06/08 21:04:38  stefan
# more work on grouping
#
# Revision 1.9  2001/04/05 09:58:14  chalky
# More comments, and use config object exclusively with basePackage support
#
# Revision 1.8  2001/03/29 14:06:41  chalky
# Skip boring graphs (ie, no sub or super classes)
#
# Revision 1.7  2001/02/16 06:59:32  chalky
# ScopePage summaries link to source
#
# Revision 1.6  2001/02/13 06:55:23  chalky
# Made synopsis -l work again
#
# Revision 1.5  2001/02/12 04:08:09  chalky
# Added config options to HTML and Linker. Config demo has doxy and synopsis styles.
#
# Revision 1.4  2001/02/02 02:01:01  stefan
# synopsis now supports inlined inheritance tree generation
#
# Revision 1.3  2001/02/01 15:23:24  chalky
# Copywritten brown paper bag edition.
#
#
"""AST Formatting classes.

This module contains classes for formatting parts of a scope page (class,
module, etc with methods, variables etc. The actual formatting of the
declarations is delegated to multiple strategies for each part of the page,
and are defined in the FormatStrategy module.
"""
# System modules
import types, os

# Synopsis modules
from Synopsis.Core import AST, Type, Util

# HTML modules
import Tags, core, FormatStrategy
from core import config
from Tags import *

class Part(Type.Visitor, AST.Visitor):
    """Base class for formatting a Part of a Scope Page.
    
    This class contains functionality for modularly formatting an AST node and
    its children for display. It is typically used to contruct Heading,
    Summary and Detail formatters. Strategy objects are added according to
    configuration, and this base class  then checks which format methods each
    strategy implements. For each AST declaration visited, the Part asks all
    Strategies which implement the appropriate format method to generate
    output for that declaration. The final writing of the formatted html is
    delegated to the writeSectionStart, writeSectionEnd, and writeSectionItem
    methods, which myst be implemented in a subclass.
    """
    def __init__(self, page):
        self.__page = page
	self.__formatters = []
	self.__id_holder = None
	# Lists of format methods for each AST type
	self.__formatdict = {
	    'formatDeclaration':[], 'formatForward':[], 'formatGroup':[], 'formatScope':[],
	    'formatModule':[], 'formatMetaModule':[], 'formatClass':[],
	    'formatTypedef':[], 'formatEnum':[], 'formatVariable':[],
	    'formatConst':[], 'formatFunction':[], 'formatOperation':[], 
	}
    
    def filename(self): return self.__page.filename()
    def os(self): return self.__page.os()
    def scope(self): return self.__page.scope()
    def write(self, text): self.os().write(text)

    def _init_formatters(self, config_option, type_msg):
	"""Loads strategies from config file"""
	base = 'Synopsis.Formatter.HTML.FormatStrategy.'
	try:
	    config_obj = getattr(config.obj.ScopePages, config_option)
	    if type(config_obj) not in (types.ListType, types.TupleType):
		raise TypeError, "ScopePages.%s must be a list or tuple of modules"%config_option
	    for formatter in config_obj:
		clas = core.import_object(formatter, basePackage=base)
		if config.verbose > 1: print "Using %s formatter:"%type_msg,clas
		self.addFormatter(clas)
	except AttributeError:
	    # Some defaults if config fails
	    self._init_default_formatters()

    def addFormatter(self, formatterClass):
	"""Adds the given formatter Class. An object is instantiated from the
	class passing self to the constructor. Stores the object, and stores
	which format methods it overrides"""
	formatter = formatterClass(self)
	self.__formatters.append(formatter)
	# For each method name:
	for method in self.__formatdict.keys():
	    no_func = getattr(FormatStrategy.Strategy, method).im_func
	    method_obj = getattr(formatter, method)
	    # If it was overridden in formatter
	    if method_obj.im_func is not no_func:
		# Add to the dictionary
		self.__formatdict[method].append(method_obj)
    
    # Access to generated values
    def type_ref(self): return self.__type_ref
    def type_label(self): return self.__type_label
    def declarator(self): return self.__declarator
    def parameter(self): return self.__parameter
    
    def reference(self, name, label=None, **keys):
	"""Returns a reference to the given name. The name is a scoped name,
	and the optional label is an alternative name to use as the link text.
	The name is looked up in the TOC so the link may not be local. The
	optional keys are appended as attributes to the A tag."""
	if not label: label = anglebrackets(Util.ccolonName(name, self.scope()))
	entry = config.toc[name]
	if entry: return apply(href, (rel(self.filename(), entry.link), label), keys)
	return label or ''

    def label(self, name, label=None):
	"""Create a label for the given name. The label is an anchor so it can
	be referenced by other links. The name of the label is derived by
	looking up the name in the TOC and using the link in the TOC entry.
	The optional label is an alternative name to use as the displayed
	name. If the name is not found in the TOC then the name is not
	anchored and just label is returned (or name if no label is given).
	"""
	if label is None: label = name
	# some labels are templates with <>'s
        entry = config.toc[name]
        label = anglebrackets(Util.ccolonName(label, self.scope()))
	if entry is None: return label
	location = entry.link
	index = string.find(location, '#')
	if index >= 0: location = location[index+1:]
        return location and Tags.name(location, label) or label


    def formatDeclaration(self, decl, method):
	"""Format decl using named method of each formatter. Each formatter
	returns two strings - type and name. All the types are joined and all
	the names are joined separately. The consolidated type and name
	strings are then passed to writeSectionItem."""
	# TODO - investigate quickest way of doing this. I tried.
	# A Lambda that calls the given function with decl
	format = lambda func, decl=decl: func(decl)
	# Get a list of 2tuples [('type','name'),('type','name'),...]
	type_name = map(format, self.__formatdict[method])
	if not len(type_name): return
	# NEW CODE:
	text = string.strip(string.join(type_name))
	self.writeSectionItem(text)
	return

    def process(self, decl):
	"""Formats the given decl, creating the output for this Part of the
	page. This method is implemented in various subclasses in different
	ways, for example Summary and Detail iterate through the children of
	'decl' section by section, whereas Heading only formats decl itself.
	"""
	pass
	
    #################### AST Visitor ############################################
    def visitDeclaration(self, decl):	self.formatDeclaration(decl, 'formatDeclaration')
    def visitForward(self, decl):	self.formatDeclaration(decl, 'formatForward')
    def visitGroup(self, decl):         self.formatDeclaration(decl, 'formatGroup')
    def visitScope(self, decl):		self.formatDeclaration(decl, 'formatScope')
    def visitModule(self, decl):	self.formatDeclaration(decl, 'formatModule')
    def visitMetaModule(self, decl):	self.formatDeclaration(decl, 'formatMetaModule')
    def visitClass(self, decl):		self.formatDeclaration(decl, 'formatClass')
    def visitTypedef(self, decl):	self.formatDeclaration(decl, 'formatTypedef')
    def visitEnum(self, decl):		self.formatDeclaration(decl, 'formatEnum')
    def visitVariable(self, decl):	self.formatDeclaration(decl, 'formatVariable')
    def visitConst(self, decl):		self.formatDeclaration(decl, 'formatConst')
    def visitFunction(self, decl):	self.formatDeclaration(decl, 'formatFunction')
    def visitOperation(self, decl):	self.formatDeclaration(decl, 'formatOperation')


    #################### Type Formatter/Visitor #################################
    def formatType(self, typeObj, id_holder = None):
	"Returns a reference string for the given type object"
	if typeObj is None: return "(unknown)"
	if id_holder:
	    save_id = self.__id_holder
	    self.__id_holder = id_holder
        typeObj.accept(self)
	if id_holder:
	    self.__id_holder = save_id
        return self.__type_label

    def visitBaseType(self, type):
	"Sets the label to be a reference to the type's name"
        self.__type_label = self.reference(type.name())
        
    def visitUnknown(self, type):
	"Sets the label to be a reference to the type's link"
        self.__type_label = self.reference(type.link())
        
    def visitDeclared(self, type):
	"Sets the label to be a reference to the type's name"
        self.__type_label = self.reference(type.name())

    def visitDependent(self, type):
	"Sets the label to be the type's name (which has no proper scope)"
        self.__type_label = type.name()[-1]
        
    def visitModifier(self, type):
	"Adds modifiers to the formatted label of the modifier's alias"
        alias = self.formatType(type.alias())
	pre = string.join(map(lambda x:x+"&nbsp;", type.premod()), '')
	post = string.join(type.postmod(), '')
        self.__type_label = "%s%s%s"%(pre,alias,post)
            
    def visitParametrized(self, type):
	"Adds the parameters to the template name in angle brackets"
	if type.template():
	    type_label = self.reference(type.template().name())
	else:
	    type_label = "(unknown)"
        params = map(self.formatType, type.parameters())
        self.__type_label = "%s&lt;%s&gt;"%(type_label,string.join(params, ", "))

    def visitTemplate(self, type):
	"Labs the template with the parameters"
	self.__type_label = "template&lt;%s&gt;"%(
	    string.join(map(lambda x:"typename "+x, map(self.formatType, type.parameters())), ",")
	)

    def visitFunctionType(self, type):
	"Labels the function type with return type, name and parameters"
	ret = self.formatType(type.returnType())
	params = map(self.formatType, type.parameters())
	pre = string.join(type.premod(), '')
	if self.__id_holder:
	    ident = self.__id_holder[0]
	    del self.__id_holder[0]
	else:
	    ident = ''
	self.__type_label = "%s(%s%s)(%s)"%(ret,pre,ident,string.join(params,", "))



    # These are overridden in {Summary,Detail}Formatter
    def write_start(self):
	"Abstract method to start the output, eg table headings"
	pass
    def writeSectionStart(self, heading):
	"Abstract method to start a section of declaration types"
	pass
    def writeSectionEnd(self, heading):
	"Abstract method to end a section of declaration types"
	pass
    def writeSectionItem(self, text):
	"Abstract method to write the output of one formatted declaration"
	pass
    def write_end(self):
	"Abstract method to end the output, eg close the table"
	pass

class Heading(Part):
    """Heading page part. Displays a header for the page -- its strategies are
    only passed the object that the page is for; ie a Class or Module"""
    def __init__(self, page):
	Part.__init__(self, page)
	self._init_formatters('heading_formatters', 'heading')

    def _init_default_formatters(self):
	self.addFormatter( FormatStrategy.Heading )
	self.addFormatter( FormatStrategy.ClassHierarchyGraph )
	self.addFormatter( FormatStrategy.DetailCommenter )

    def writeSectionItem(self, text):
	"""Writes text and follows with a horizontal rule"""
	self.write(text + '\n<hr>\n')

    def process(self, decl):
	"""Process this Part by formatting only the given decl"""
	decl.accept(self)

class Summary(Part):
    """Formatting summary visitor. This formatter displays a summary for each
    declaration, with links to the details if there is one. All of this is
    controlled by the ASTFormatters."""
    def __init__(self, page):
	Part.__init__(self, page)
	self.__link_detail = 0
	self._init_formatters('summary_formatters', 'summary')

    def _init_default_formatters(self):
	self.addFormatter( FormatStrategy.SummaryAST )
	self.addFormatter( FormatStrategy.SummaryCommenter )

    def set_link_detail(self, boolean):
	"""Sets link_detail flag to given value.
	@see label()"""
	self.__link_detail = boolean
	config.link_detail = boolean

    def label(self, ref, label=None):
	"""Override to check link_detail flag. If it's set, returns a reference
	instead - which will be to the detailed info"""
	if label is None: label = ref
	if self.__link_detail:
	    # Insert a reference instead
	    return span('name',self.reference(ref, Util.ccolonName(label, self.scope())))
	return Part.label(self, ref, label)
	
    def getSummary(self, node):
	comment = config.comments[node].summary
	if len(comment): return div('summary', comment)
	else: return ''

    def writeSectionStart(self, heading):
	"""Starts a table entity. The heading is placed in a row in a td with
	the class 'heading'."""
	self.write('<table width="100%%" summary="%s">\n'%heading)
	self.write('<col><col width="100%%">')
	self.write('<tr><td class="heading" colspan="2">' + heading + '</td></tr>\n')

    def writeSectionEnd(self, heading):
	"""Closes the table entity and adds a break."""
	self.write('</table>\n<br>\n')

    def writeSectionItem(self, text):
	"""Adds a table row"""
	if text[:22] == '<td class="summ-start"':
	    # text provided its own TD element
	    self.write('<tr>' + text + '</td></tr>\n')
	else:
	    self.write('<tr><td class="summ-start">' + text + '</td></tr>\n')

    def process(self, decl):
	"Print out the summaries from the given decl"
	comments = config.comments
	config.link_detail = 0
	
	config.sorter.set_scope(decl)
	config.sorter.sort_section_names()

	self.write_start()
	for section in config.sorter.sections():
	    # Write a header for this section
	    if section[-1] == 's': heading = section+'es Summary:'
	    else: heading = section+'s Summary:'
	    self.writeSectionStart(heading)
	    # Iterate through the children in this section
	    for child in config.sorter.children(section):
		# Check if need to add to detail list
		if comments[child].has_detail:
		    # Setup the linking stuff
		    self.set_link_detail(1)
		    child.accept(self)
		    self.set_link_detail(0)
		else:
		    # Just do it
		    child.accept(self)
	    # Finish off this section
	    self.writeSectionEnd(heading)
	self.write_end()


class Detail(Part):
    def __init__(self, page):
	Part.__init__(self, page)
	self._init_formatters('detail_formatters', 'detail')

    def _init_default_formatters(self):
	self.addFormatter( FormatStrategy.DetailAST )
	#self.addFormatter( ClassHierarchySimple )
	self.addFormatter( FormatStrategy.DetailCommenter )

    def getDetail(self, node):
	comment = config.comments[node].detail
	if comment and len(comment): return '<br>'+div('desc', comment)
	else: return ''

    def writeSectionStart(self, heading):
	"""Creates a table with one row. The row has a td of class 'heading'
	containing the heading string"""
	self.write('<table width="100%%" summary="%s">\n'%heading)
        self.write('<tr><td colspan="2" class="heading">' + heading + '</td></tr>\n')
        self.write('</table>')

    def writeSectionItem(self, text):
	"""Writes text and follows with a horizontal rule"""
	self.write(text + '\n<hr>\n')

    def process(self, decl):
	"Print out the details for the children of the given decl"
	comments = config.comments

	config.sorter.set_scope(decl)
	config.sorter.sort_section_names()

	# Iterate through the sections with details
	self.write_start()
	for section in config.sorter.sections():
	    # Write a heading
	    heading = section+' Details:'
	    started = 0 # Lazy section start incase no details for this section
	    # Iterate through the children in this section
	    for child in config.sorter.children(section):
		# Check if need to add to detail list
		if not comments[child].has_detail:
		    continue
		# Check section heading
		if not started:
		    started = 1
		    self.writeSectionStart(heading)
		child.accept(self)
	    # Finish the section
	    if started: self.writeSectionEnd(heading)
	self.write_end()
     
class Inheritance (Part):
    def __init__(self, page):
	Part.__init__(self, page)
	self._init_formatters('inheritance_formatters', 'inheritance')
	self.__start_list = 0

    def _init_default_formatters(self):
	self.addFormatter( FormatStrategy.Inheritance )

    def process(self, decl):
	"Walk the hierarchy to find inherited members to print."
	if not isinstance(decl, AST.Class): return
	self.write_start()
	names = decl.declarations()
	names = map(self._short_name, names)
	self._process_superclasses(decl, names)
	self.write_end()

    def _process_class(self, clas, names):
	"Prints info for the given class, and calls _process_superclasses after"
	config.sorter.set_scope(clas)
	config.sorter.sort_section_names()
	child_names = []

	# Iterate through the sections
	for section in config.sorter.sections():
	    # Write a heading
	    heading = section+'s Inherited from '+ Util.ccolonName(clas.name(), self.scope())
	    started = 0 # Lazy section start incase no details for this section
	    # Iterate through the children in this section
	    for child in config.sorter.children(section):
		child_name = self._short_name(child)
		if child_name in names:
		    continue
		# FIXME: This doesn't account for the inheritance type
		# (private etc)
		if child.accessibility() == AST.PRIVATE:
		    continue
		# Don't include constructors and destructors!
		if isinstance(child, AST.Function) and child.language() == 'C++' and len(child.realname())>1:
		    if child.realname()[-1] == child.realname()[-2]: continue
		    elif child.realname()[-1] == "~"+child.realname()[-2]: continue
		# FIXME: skip overriden declarations
		child_names.append(child_name)
		# Check section heading
		if not started:
		    started = 1
		    self.writeSectionStart(heading)
		child.accept(self)
	    # Finish the section
	    if started: self.writeSectionEnd(heading)
	
	self._process_superclasses(clas, names + child_names)
    
    def _short_name(self, decl):
	if isinstance(decl, AST.Function):
	    return decl.realname()[-1]
	return decl.name()[-1]
    
    def _process_superclasses(self, clas, names):
	"""Iterates through the superclasses of clas and calls _process_clas for
	each"""
	for inheritance in clas.parents():
	    parent = inheritance.parent()
	    if isinstance(parent, Type.Declared):
		parent = parent.declaration()
		if isinstance(parent, AST.Class):
		    self._process_class(parent, names)
		    continue
	    #print "Ignoring", parent.__class__.__name__, "parent of", clas.name()
	    pass #ignore
     
    def writeSectionStart(self, heading):
	"""Creates a table with one row. The row has a td of class 'heading'
	containing the heading string"""
	self.write('<table width="100%%" summary="%s">\n'%heading)
        self.write('<tr><td colspan="2" class="heading">' + heading + '</td></tr>\n')
	self.write('<tr><td class="inherited">')
	self.__start_list = 1

    def writeSectionItem(self, text):
	"""Adds a table row"""
	if self.__start_list:
	    self.write(text)
	    self.__start_list = 0
	else:
	    self.write(',\n'+text)

    def writeSectionEnd(self, heading):
	"""Closes the table entity and adds a break."""
	self.write('</td></tr></table>\n<br>\n')


