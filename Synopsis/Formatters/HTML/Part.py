# $Id: Part.py,v 1.6 2001/02/13 06:55:23 chalky Exp $
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

This module contains ASTFormatter and the default base classes of it. It also
contains SummaryFormatter and DetailFormatter. Probably they should be moved
to another file, but that is for another day.
"""
# System modules
import types, os

# Synopsis modules
from Synopsis.Core import AST, Type, Util

# HTML modules
import Tags, core
from core import config
from Tags import *

class ASTFormatter (Type.Visitor):
    """This class formats the various AST elements. It includes facilities for
    formatting types with a single call, and formatting modifiers.

    The key concept of this class though is the format{AST types} methods. Any
    class derived from ASTFormatter that overrides one of the format methods
    will have that method called by the Summary and Detail formatters when
    they visit that AST type. Summary and Detail maintain a list of
    ASTFormatters, and a list for each AST type. For example, when
    SummaryFormatter visits a Function object, it calls the formatFunction
    method on all ASTFormatters registed with SummaryFormatter that
    implemented that method. Each of these format methods returns two
    strings referred to as 'type' and 'name', which comes from Summary sections
    having two columns. All the 'type' strings are joined and all the 'name'
    strings are joined. How the two are formatted depends on whether its
    Summary or Detail.

    @see BaseFormatter
    @see SummaryFormatter
    @see DetailFormatter
    """
    def __init__(self, formatter):
	"""Store formatter as self.format. The formatter is either a
	SummaryFormatter or DetailFormatter, and is used for things like
	reference() and label() calls. Local references to the formatter's
	reference and label methods are stored in self for more efficient use
	of them."""
	self.format = formatter
	self.label = formatter.label
	self.reference = formatter.reference

    #
    # Utility methods
    #
    def formatModifiers(self, modifiers):
	"""Returns a HTML string from the given list of string modifiers. The
	modifiers are enclosed in 'keyword' spans."""
	keyword = lambda m,span=span: span("keyword", m)
	return string.join(map(keyword, modifiers))

    #
    # Type Formatter/Visitor
    #
    def formatType(self, typeObj):
	"Returns a reference string for the given type object"
	if typeObj is None: return "(unknown)"
        typeObj.accept(self)
        return self.__type_label

    def visitBaseType(self, type):
        self.__type_label = self.reference(type.name())
        
    def visitUnknown(self, type):
        self.__type_label = self.reference(type.link())
        
    def visitDeclared(self, type):
        self.__type_label = self.reference(type.name())
        
    def visitModifier(self, type):
        alias = self.formatType(type.alias())
	pre = string.join(map(lambda x:x+"&nbsp;", type.premod()), '')
	post = string.join(type.postmod(), '')
        self.__type_label = "%s%s%s"%(pre,alias,post)
            
    def visitParametrized(self, type):
	if type.template():
	    type_label = self.reference(type.template().name())
	else:
	    type_label = "(unknown)"
        params = map(self.formatType, type.parameters())
        self.__type_label = "%s&lt;%s&gt;"%(type_label,string.join(params, ", "))

    def visitTemplate(self, type):
	self.__type_label = "template&lt;%s&gt;"%(
	    string.join(map(lambda x:"typename "+x, map(self.formatType, type.parameters())), ",")
	)

    def visitFunctionType(self, type):
	ret = self.formatType(type.returnType())
	params = map(self.formatType, type.parameters())
	pre = string.join(type.premod(), '')
	self.__type_label = "%s(%s)(%s)"%(ret,pre,string.join(params, ", "))


    #
    # AST Formatters
    #
    def formatDeclaration(self, decl):	pass
    def formatForward(self, decl):	pass
    def formatScope(self, decl):	pass
    def formatModule(self, decl):	pass
    def formatMetaModule(self, decl):	pass
    def formatClass(self, decl):	pass
    def formatTypedef(self, decl):	pass
    def formatEnum(self, decl):		pass
    def formatVariable(self, decl):	pass
    def formatConst(self, decl):	pass
    def formatFunction(self, decl):	pass
    def formatOperation(self, decl):	pass

class BaseASTFormatter(ASTFormatter):
    def formatParameters(self, parameters):
	"Returns formatted string for the given parameter list"
        return string.join(map(self.formatParameter, parameters), ", ")

    def formatDeclaration(self, decl):
	"""The default is to return no type and just the declarations name for
	the name"""
	return '',self.label(decl.name())

    def formatForward(self, decl): return self.formatDeclaration(decl)
    def formatScope(self, decl):
	"""Scopes have their own pages, so return a reference to it"""
	return '',self.reference(decl.name())
    def formatModule(self, decl): return self.formatScope(decl)
    def formatMetaModule(self, decl): return self.formatModule(decl)
    def formatClass(self, decl): return self.formatScope(decl)

    def formatTypedef(self, decl):
	"(typedef type, typedef name)"
	type = self.formatType(decl.alias())
	return type, self.label(decl.name())

    def formatEnumerator(self, decl):
	"""This is only called by formatEnum"""
	self.formatDeclaration(decl)

    def formatEnum(self, decl):
	"(enum name, list of enumerator names)"
	type = self.label(decl.name())
	name = map(lambda enumor:enumor.name()[-1], decl.enumerators())
	name = string.join(name, ', ')
	return type, name

    def formatVariable(self, decl):
	# TODO: deal with sizes
        type = self.formatType(decl.vtype())
	return type, self.label(decl.name())

    def formatConst(self, decl):
	"(const type, const name = const value)"
	type = self.formatType(decl.ctype())
        name = self.label(decl.name()) + " = " + decl.value()
	return type, name

    def formatFunction(self, decl):
	"(return type, func + params + formatFunctionExceptions)"
	premod = self.formatModifiers(decl.premodifier())
	type = self.formatType(decl.returnType())
	name = self.label(decl.name(), decl.realname())
	# Special C++ functions  TODO: maybe move to a separate AST formatter...
	if decl.language() == 'C++' and len(decl.realname())>1:
	    if decl.realname()[-1] == decl.realname()[-2]: type = '<i>constructor</i>'
	    elif decl.realname()[-1] == "~"+decl.realname()[-2]: type = '<i>destructor</i>'
	    elif decl.realname()[-1] == "(conversion)":
		name = "(%s)"%type
	params = self.formatParameters(decl.parameters())
	postmod = self.formatModifiers(decl.postmodifier())
	raises = self.formatOperationExceptions(decl)
	type = '%s %s'%(premod,type)
        if decl.type() == "attribute": name = '%s %s %s'%(name, postmod, raises)
	else: name = '%s(%s) %s %s'%(name, params, postmod, raises)
	return type, name

    # Default operation is same as function, and quickest way is to assign:
    def formatOperation(self, decl): return self.formatFunction(decl)

    def formatParameter(self, parameter):
	"""Returns one string for the given parameter"""
	str = []
	keyword = lambda m,span=span: span("keyword", m)
	# Premodifiers
	str.extend(map(keyword, parameter.premodifier()))
	# Param Type
	typestr = self.formatType(parameter.type())
	if typestr: str.append(typestr)
	# Postmodifiers
	str.extend(map(keyword, parameter.postmodifier()))
	# Param identifier
        if len(parameter.identifier()) != 0:
            str.append(span("variable", parameter.identifier()))
	# Param value
	if len(parameter.value()) != 0:
	    str.append(" = " + span("value", parameter.value()))
	return string.join(str)

class SummaryASTFormatter (BaseASTFormatter):
    """Derives from BaseASTFormatter to provide summary-specific methods.
    Currently the only one is formatOperationExceptions"""
    def formatOperationExceptions(self, oper):
	"""Returns a reference to the detail if there are any exceptions."""
        if len(oper.exceptions()):
            return self.reference(oper.name(), " raises")
	return ''

class DefaultASTFormatter (ASTFormatter):
    """A base AST formatter that calls formatDeclaration for all types"""
    # All these use the same method:
    def formatForward(self, decl): return self.formatDeclaration(decl)
    def formatScope(self, decl): return self.formatDeclaration(decl)
    def formatModule(self, decl): return self.formatDeclaration(decl)
    def formatMetaModule(self, decl): return self.formatDeclaration(decl)
    def formatClass(self, decl): return self.formatDeclaration(decl)
    def formatTypedef(self, decl): return self.formatDeclaration(decl)
    def formatEnum(self, decl): return self.formatDeclaration(decl)
    def formatVariable(self, decl): return self.formatDeclaration(decl)
    def formatConst(self, decl): return self.formatDeclaration(decl)
    def formatFunction(self, decl): return self.formatDeclaration(decl)
    def formatOperation(self, decl): return self.formatDeclaration(decl)
 
class SummaryASTCommenter (DefaultASTFormatter):
    """Adds summary comments to all declarations"""
    def formatDeclaration(self, decl):
	comm = config.comments[decl]
	return '', div('summary', comm.summary)
    

class DetailASTFormatter (BaseASTFormatter):
    """Provides detail-specific AST formatting."""

    def formatNameWithParents(self, node):
	"Formats a reference to each parent scope"
	# Figure out the enclosing scope
	scope, text = [], []
	for name in node.name()[:-1]:
	    scope.append(name)
	    text.append(self.reference(scope))
	text.append(node.name()[-1])
	return string.join(text, "::")

    def formatModule(self, module):
	# Module details are only printed at the top of their page
	type = string.capitalize(module.type())
	name = self.formatNameWithParents(module)
	name = entity('h1', "%s %s"%(type, name))
	return '', name

    def formatClass(self, clas):
	# Class details are only printed at the top of their page
	type = string.capitalize(clas.type())
	name = self.formatNameWithParents(clas)
	name = entity('h1', "%s %s"%(type, name))

	# Print template
	templ = clas.template()
	if templ:
	    templ = entity('h2', "template &lt;%s&gt;"%string.join(map(self.formatType, templ.parameters()),", "))
	else:
	    templ = ''

	# Print filename
	file = string.split(clas.file(), os.sep)
	files = "Files: "+href(config.files.nameOfFile(file),clas.file(),target='index')+"<br>"

	return '', '%s%s%s'%(name, templ, files)

    def formatOperationExceptions(self, oper):
        if len(oper.exceptions()):
            raises = span("keyword", "raises")
            exceptions = []
            for exception in oper.exceptions():
                exceptions.append(self.reference(exception.name()))
	    exceptions = span("raises", string.join(exceptions, ", "))
            return '%s (%s)'%(raises, exceptions)
	return ''

    def formatEnum(self, enum):
        name = span("keyword", "enum ") + self.label(enum.name())
        start = '<div class="enum">'
	enumors = string.join(map(self.formatEnumerator, enum.enumerators()))
        end = "</div>"
	return '', '%s%s%s%s'%(name, start, enumors, end)

    def formatEnumerator(self, enumerator):
        text = self.label(enumerator.name())
        if len(enumerator.value()):
            value = " = " + span("value", enumerator.value())
	else: value = ''
	comm = config.comments[enumerator]
	comments = desc(comm.summary or comm.detail or '')
	return '<div class="enumerator">%s%s%s</div>'%(text,value,comments)

class DetailASTCommenter (DefaultASTFormatter):
    """Adds summary comments to all declarations"""
    def formatDeclaration(self, decl):
	comm = config.comments[decl]
	if comm.detail is None: return '',''
	return '', desc(comm.detail)
    

class ClassHierarchySimple (ASTFormatter):
    "Prints a simple hierarchy for classes"
    def formatInheritance(self, inheritance):
	return '%s %s'%( self.formatModifiers(inheritance.attributes()),
	    self.formatType(inheritance.parent()))

    def formatClass(self, clas):
	# Print out a list of the parents
	super = sub = ''
	if clas.parents():
	    parents = map(self.formatInheritance, clas.parents())
	    super = string.join(parents, ", ")
	    super = div('superclasses', "Superclasses: "+super)

	# Print subclasses
	subs = config.classTree.subclasses(clas.name())
	if subs:
	    refs = map(self.reference, subs)
	    sub = string.join(refs, ", ")
	    sub = div('subclasses', "Known subclasses: "+sub) 
	
	return '', super + sub

class ClassHierarchyGraph (ClassHierarchySimple):
    "Prints a graphical hierarchy for classes, using the Dot formatter"
    def formatClass(self, clas):
        try:
            import tempfile
            from Synopsis.Formatter import Dot
        except:
            print "Can't load the Dot formatter, using a text based class hierarchy instead"
            return ClassHierarchySimple.formatClass(self, clas)
        tmp = tempfile.mktemp("synopsis")
        label = config.files.nameOfScopedSpecial('inheritance', clas.name())
        dot_args = ["-o", tmp, "-f", "html", "-s", "-t", label]
        #if core.verbose: args.append("-v")
        Dot.toc = config.toc
        Dot.nodes = {}
        Dot.format(config.types, [clas], dot_args, None)
        text = ''
        input = open(tmp + ".html", "r+")
        line = input.readline()
        while line:
            text = text + line
            line = input.readline()
        input.close()
	return '', text

class BaseFormatter(Type.Visitor, AST.Visitor):
    """
    Formatting visitor base. This class contains functionality for modularly
    formatting an AST for display. It is typically used to contruct Summary
    and Detail formatters. ASTFormatters are added to the BaseFormatter, which
    then checks which format methods they implement. For each AST declaration
    visited, the BaseFormatter asks all ASTFormatters that can to format that
    declaration. Actually writing the formatted info to file is left to
    writeSectionStart, writeSectionEnd, and writeSectionItem.
    """
    def __init__(self):
        self.__os = None
        self.__scope = []
	self.__formatters = []
	# Lists of format methods for each AST type
	self.__formatdict = {
	    'formatDeclaration':[], 'formatForward':[], 'formatScope':[],
	    'formatModule':[], 'formatMetaModule':[], 'formatClass':[],
	    'formatTypedef':[], 'formatEnum':[], 'formatVariable':[],
	    'formatConst':[], 'formatFunction':[], 'formatOperation':[], 
	}
    
    def scope(self): return self.__scope
    def write(self, text): self.__os.write(text)
    def set_scope(self, scope): self.__scope = list(scope)
    def set_ostream(self, os): self.__os = os
    def addFormatter(self, formatterClass):
	"""Adds the given formatter Class. An object is instantiated from the
	class passing self to the constructor. Stores the object, and stores
	which format methods it overrides"""
	formatter = formatterClass(self)
	self.__formatters.append(formatter)
	# For each method name:
	for method in self.__formatdict.keys():
	    no_func = getattr(ASTFormatter, method).im_func
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
	if not label: label = Util.ccolonName(name, self.scope())
	entry = config.toc[name]
	if entry: return apply(href, (entry.link, label), keys)
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


    def formatOperationExceptions(self, operation): pass

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
	# Convert to 2tuple of lists # [['type','type'...],['name','name',...]]
	type_name = apply(map, [None]+type_name)
	# Convert to two strings 'type type', 'name name'
	if type(type_name[0]) == types.TupleType:
	    type_name = map(string.join, type_name)
	# Strip strings from extra whitespace of empty types or names
	ftype, name = map(string.strip, type_name)
	# Write the two strings
	self.writeSectionItem(ftype, name)

    #################### AST Visitor ############################################
    def visitDeclaration(self, decl):	self.formatDeclaration(decl, 'formatDeclaration')
    def visitForward(self, decl):	self.formatDeclaration(decl, 'formatForward')
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

    # These are overridden in {Summary,Detail}Formatter
    def writeStart(self): pass
    def writeSectionStart(self, heading): pass
    def writeSectionEnd(self, heading): pass
    def writeSectionItem(self, type, name): pass
    def writeEnd(self): pass


class SummaryFormatter(BaseFormatter):
    """Formatting summary visitor. This formatter displays a summary for each
    declaration, with links to the details if there is one. All of this is
    controlled by the ASTFormatters."""
    def __init__(self):
	BaseFormatter.__init__(self)
	self.__link_detail = 0
	self._init_formatters()

    def _init_formatters(self):
	# TODO - load from config
	try:
	    for formatter in config.obj.ScopePages.summary_formatters:
		clas = core.import_object(formatter)
		if config.verbose: print "Using summary formatter:",clas
		self.addFormatter(clas)
	except AttributeError:
	    if config.verbose: print "Summary config failed. Using defaults."
	    self.addFormatter( SummaryASTFormatter )
	    self.addFormatter( SummaryASTCommenter )

    def set_link_detail(self, boolean):
	"""Sets link_detail flag to given value.
	@see label()"""
	self.__link_detail = boolean

    def label(self, ref, label=None):
	"""Override to check link_detail flag. If it's set, returns a reference
	instead - which will be to the detailed info"""
	if label is None: label = ref
	if self.__link_detail:
	    # Insert a reference instead
	    return span('name',self.reference(ref, Util.ccolonName(label, self.scope())))
	return BaseFormatter.label(self, ref, label)
	
    def getSummary(self, node):
	comment = config.comments[node].summary
	if len(comment): return div('summary', comment)
	else: return ''

    def writeSectionStart(self, heading):
	"""Starts a table entity. The heading is placed in a row in a td with
	the class 'heading'."""
	str = '<table width="100%%">'\
	'<col width="100px"><col width="100%%">'\
	'<tr><td colspan="2" class="heading">%s</td></tr>'
	self.write(str%heading)

    def writeSectionEnd(self, heading):
	"""Closes the table entity and adds a break."""
	str = '</table><br>'
	self.write(str)

    def writeSectionItem(self, type, name):
	"""Adds a table row with one or two data elements. If type is None
	then there is only one td with a colspan of 2."""
	if not len(type):
	    str = '<tr><td colspan="2">%s</td></tr>'
	    self.write(str%name)
	else:
	    str = '<tr><td valign="top">%s</td><td>%s</td></tr>'
	    self.write(str%(type,name))


class DetailFormatter(BaseFormatter):
    def __init__(self):
	BaseFormatter.__init__(self)
	self._init_formatters()

    def _init_formatters(self):
	# TODO - load from config
	try:
	    for formatter in config.obj.ScopePages.detail_formatters:
		clas = core.import_object(formatter)
		if config.verbose: print "Using summary formatter:",clas
		self.addFormatter(clas)
	except AttributeError:
	    self.addFormatter( DetailASTFormatter )
	    #self.addFormatter( ClassHierarchySimple )
	    self.addFormatter( ClassHierarchyGraph )
	    self.addFormatter( DetailASTCommenter )

    def getDetail(self, node):
	comment = config.comments[node].detail
	if comment and len(comment): return '<br>'+div('desc', comment)
	else: return ''

    def writeSectionStart(self, heading):
	"""Creates a table with one row. The row has a td of class 'heading'
	containing the heading string"""
	str = '<table width="100%%">'\
	'<tr><td colspan="2" class="heading">%s</td></tr></table>'
	self.write(str%heading)

    def writeSectionItem(self, text1, text2):
	"""Joins text1 and text2 and follows with a horizontal rule"""
	str = '%s %s<hr>'
	self.write(str%(text1, text2))


