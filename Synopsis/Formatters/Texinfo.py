#  $Id: Texinfo.py,v 1.6 2003/11/11 12:50:56 stefan Exp $
#
#  This file is a part of Synopsis.
#  Copyright (C) 2000, 2001 Stefan Seefeld
#
#  Synopsis is free software; you can redistribute it and/or modify it
#  under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
#  02111-1307, USA.
#
# $Log: Texinfo.py,v $
# Revision 1.6  2003/11/11 12:50:56  stefan
# remove 'Core' module
#
# Revision 1.5  2001/07/26 08:22:20  chalky
# Fixes 'bug' caused by bad template support
#
# Revision 1.4  2001/07/19 04:03:05  chalky
# New .syn file format.
#
# Revision 1.3  2001/07/10 06:04:07  stefan
# added support for info files to the TexInfo formatter
#
# Revision 1.2  2001/06/15 18:07:48  stefan
# made TexInfo formatter useable. Removed all document structure stuff, such that the output can be embedded into any document level (as a chapter, a section, etc.)
#
# Revision 1.1  2001/06/11 19:45:15  stefan
# initial work on a TexInfo formatter
#
#
#
"""a TexInfo formatter """
# THIS-IS-A-FORMATTER

import sys, getopt, os, os.path, string, re
from Synopsis import AST, Type, Util

verbose = 0

class Struct:
    "Dummy class. Initialise with keyword args."
    def __init__(self, **keys):
	for name, value in keys.items(): setattr(self, name, value)

class CommentFormatter:
    """A class that takes a comment Struct and formats its contents."""

    def parse(self, comm):
	"""Parse the comment struct"""
	pass

class JavadocFormatter (CommentFormatter):
    """A formatter that formats comments similar to Javadoc @tags"""
    # @see IDL/Foo.Bar
    # if a line starts with @tag then all further lines are tags
    _re_see = '@see (([A-Za-z+]+)/)?(([A-Za-z_]+\.?)+)'
    _re_tags = '(?P<text>.*?)\n[ \t]*(?P<tags>@[a-zA-Z]+[ \t]+.*)'
    _re_see_line = '^[ \t]*@see[ \t]+(([A-Za-z+]+)/)?(([A-Za-z_]+\.?)+)(\([^)]*\))?([ \t]+(.*))?$'
    _re_param = '^[ \t]*@param[ \t]+(?P<name>(A-Za-z+]+)([ \t]+(?P<desc>.*))?$'

    def __init__(self):
	"""Create regex objects for regexps"""
	self.re_see = re.compile(self._re_see)
	self.re_tags = re.compile(self._re_tags,re.M|re.S)
	self.re_see_line = re.compile(self._re_see_line,re.M)
    def parse(self, comm):
	"""Parse the comm.detail for @tags"""
	comm.detail = self.parseText(comm.detail, comm.decl)
	comm.summary = self.parseText(comm.summary, comm.decl)
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

    def parseTags(self, str, joiner):
	"""Returns text, tags"""
	# Find tags
	mo = self.re_tags.search(str)
	if not mo: return str, ''
	str, tags = mo.group('text'), mo.group('tags')
	# Split the tag section into lines
	tags = map(string.strip, string.split(tags,'\n'))
	# Join non-tag lines to the previous tag
	tags = reduce(joiner, tags, [])
	return str, tags

    def parseText(self, str, decl):
	if str is None: return str
	#str, see = self.extract(self.re_see_line, str)
	see_tags, attr_tags, param_tags, return_tag = [], [], [], None
	joiner = lambda x,y: len(y) and y[0]=='@' and x+[y] or x[:-1]+[x[-1]+' '+y]
	str, tags = self.parseTags(str, joiner)
	# Parse each of the tags
	for line in tags:
	    tag, rest = string.split(line,' ',1)
	    if tag == '@see':
		see_tags.append(string.split(rest,' ',1))
	    elif tag == '@param':
		param_tags.append(string.split(rest,' ',1))
	    elif tag == '@return':
		return_tag = rest
	    elif tag == '@attr':
		attr_tags.append(string.split(rest,' ',1))
	    else:
		# Warning: unknown tag
		pass
	return "%s%s%s%s%s"%(
	    self.parse_see(str, decl),
	    self.format_params(param_tags),
	    self.format_attrs(attr_tags),
	    self.format_return(return_tag),
	    self.format_see(see_tags, decl)
	)
    def parse_see(self, str, decl):
	"""Parses inline @see tags"""
	# Parse inline @see's  #TODO change to link or whatever javadoc uses
	mo = self.re_see.search(str)
	while mo:
	    groups, start, end = mo.groups(), mo.start(), mo.end()
	    lang = groups[1] or ''
	    #tag = self.find_link(groups[2], decl)
	    tag = groups[2]
	    str = str[:start] + tag + str[end:]
	    end = start + len(tag)
	    mo = self.re_see.search(str, end)
	return str
    def format_params(self, param_tags):
	"""Formats a list of (param, description) tags"""
	if not len(param_tags): return ''
	table = '@table @samp\n%s@end table\n'
	return table%string.join(map(lambda p:'@item %s\n%s'%(p[0],p[1]), param_tags), '\n')

    def format_attrs(self, attr_tags):
	"""Formats a list of (attr, description) tags"""
	if not len(attr_tags): return ''
	table = '@table @samp\n%s@end table\n'
	row = '@item %s\n%s\n'
	return 'Attributes:\n' + table%string.join(map(lambda p,row=row: row%(p[0],p[1]), attr_tags))
    def format_return(self, return_tag):
	"""Formats a since description string"""
	if not return_tag: return ''
	return "Return:\n" + return_tag
    def format_see(self, see_tags, decl):
	"""Formats a list of (ref,description) tags"""
	if not len(see_tags): return ''
        #FIXME: add proper cross referencing
	seestr = "See Also:"
	seelist = []
	for see in see_tags:
	    ref,desc = see[0], len(see)>1 and see[1] or ''
	    #tag = self.find_link(ref, decl)
            tag = ref
	    seelist.append(tag+desc)
	return seestr + string.join(seelist,'\n')
    def find_link(self, ref, decl):
	"""Given a "reference" and a declaration, returns a HTML link.
	Various methods are tried to resolve the reference. First the
	parameters are taken off, then we try to split the ref using '.' or
	'::'. The params are added back, and then we try to match this scoped
	name against the current scope. If that fails, then we recursively try
	enclosing scopes.
	"""
	# Remove params
	index, label = string.find(ref,'('), ref
	if index >= 0:
	    params = ref[index:]
	    ref = ref[:index]
	else:
	    params = ''
	# Split ref
	ref = string.split(ref, '.')
	if len(ref) == 1:
	    ref = string.split(ref[0], '::')
	# Add params back
	ref = ref[:-1] + [ref[-1]+params]
	# Find in all scopes
	scope = list(decl.name())
	while 1:
	    entry = self._find_link_at(ref, scope)
	    if entry: return href(entry.link, label)
	    if len(scope) == 0: break
	    del scope[-1]
	# Not found
	return label+" "
    def _find_link_at(self, ref, scope):
	# Try scope + ref[0]
	entry = config.toc.lookup(scope+ref[:1])
	if entry:
	    # Found.
	    if len(ref) > 1:
		# Find sub-refs
		entry = self._find_link_at(ref[1:], scope+ref[:1])
		if entry:
		    # Recursive sub-ref was okay!
		    return entry 
	    else:
		# This was the last scope in ref. Done!
		return entry
	# Try a method name match:
	if len(ref) == 1:
	    entry = self._find_method_entry(ref[0], scope)
	    if entry: return entry
	# Not found at this scope
	return None
    def _find_method_entry(self, name, scope):
	"""Tries to find a TOC entry for a method adjacent to decl. The
	enclosing scope is found using the types dictionary, and the
	realname()'s of all the functions compared to ref."""
	try:
	    scope = config.types[scope]
	except KeyError:
	    #print "No parent scope:",decl.name()[:-1]
	    return None
	if not scope: return None
	if not isinstance(scope, Type.Declared): return None
	scope = scope.declaration()
	if not isinstance(scope, AST.Scope): return None
	for decl in scope.declarations():
	    if isinstance(decl, AST.Function):
		if decl.realname()[-1] == name:
		    return config.toc.lookup(decl.name())
	# Failed
	return None

def _replace(comm, old, new):
    comm.summary = string.replace(comm.summary, old, new)
    comm.detail = string.replace(comm.detail, old, new)

class Escapifier(CommentFormatter):
    """escapify the strings to become valid texinfo text.
    Only replace '@' by '@@' if these are not part of valid texinfo tags."""

    tags = ['table', 'item', 'samp', 'end'] #etc.
    special = ['{', '}',]
    def __init__(self):
        self.__re_texi = re.compile('@(?!(' + string.join(Escapifier.tags, '|') + '))')
    def parse(self, comm):
        comm.summary = self.__re_texi.sub('@@', comm.summary)
        comm.detail = self.__re_texi.sub('@@', comm.detail)
        comm.summary = reduce(lambda x,y: string.replace(x, y , '@' + y), Escapifier.special, comm.summary)
        comm.detail = reduce(lambda x,y: string.replace(x, y, '@' + y), Escapifier.special, comm.detail)

commentFormatters = {
    'none' : CommentFormatter,
    'javadoc' : JavadocFormatter,
    'escapify' : Escapifier,
}

class MenuMaker (AST.Visitor):
    """generate a texinfo menu for the declarations of a given scope"""
    def __init__(self, scope, os):
        self.__scope = scope
        self.__os = os
    def write(self, text): self.__os.write(text)
    def start(self): self.write('@menu\n')
    def end(self): self.write('@end menu\n')
    def visitDeclaration(self, node):
        name = reduce(lambda x,y: string.replace(x, y , '@' + y), Escapifier.special, Util.dotName(node.name(), self.__scope))
        self.write('* ' + name + '::\t' + node.type() + '\n')
    visitGroup = visitDeclaration
    visitEnum = visitDeclaration

class Formatter (Type.Visitor, AST.Visitor):
    """
    The type visitors should generate names relative to the current scope.
    The generated references however are fully scoped names
    """
    def __init__(self, os):
        self.__os = os
        self.__scope = []
        self.__indent = 0
        self.__comment_formatters = [JavadocFormatter(), Escapifier()]
    def scope(self): return self.__scope
    def write(self, text): self.__os.write(text)

    def escapify(self, label):
        return reduce(lambda x,y: string.replace(x, y , '@' + y), Escapifier.special, label)

    #def reference(self, ref, label):
    #    """reference takes two strings, a reference (used to look up the symbol and generated the reference),
    #    and the label (used to actually write it)"""
    #    location = self.__toc.lookup(ref)
    #    if location != "": return href("#" + location, label)
    #    else: return span("type", str(label))
        
    #def label(self, ref):
    #    location = self.__toc.lookup(Util.ccolonName(ref))
    #    ref = Util.ccolonName(ref, self.scope())
    #    if location != "": return name("\"" + location + "\"", ref)
    #    else: return ref

    def type_label(self): return self.escapify(self.__type_label)

    def decl_label(self, decl): return self.escapify(decl[-1])

    def formatType(self, type):
	"Returns a reference string for the given type object"
	if type is None: return "(unknown)"
        type.accept(self)
        return self.type_label()

    def formatComments(self, decl):
	strlist = map(lambda x:str(x), decl.comments())
        #doc = map(lambda c, this=self: this.__comment_formatter.parse(c), strlist)
	comm = Struct(detail=string.join(strlist), summary='', has_detail=1, decl=decl)
	if comm.detail: map(lambda f,c=comm: f.parse(c), self.__comment_formatters)
	else: comm.has_detail=0
        self.write(comm.detail + '\n')

    #################### Type Visitor ###########################################

    def visitBaseType(self, type):
        self.__type_ref = Util.ccolonName(type.name())
        self.__type_label = Util.ccolonName(type.name())
        
    def visitUnknown(self, type):
        self.__type_ref = Util.ccolonName(type.name())
        self.__type_label = Util.ccolonName(type.name(), self.scope())
        
    def visitDeclared(self, type):
        self.__type_label = Util.ccolonName(type.name(), self.scope())
        self.__type_ref = Util.ccolonName(type.name())
        
    def visitModifier(self, type):
        type.alias().accept(self)
        self.__type_ref = string.join(type.premod()) + " " + self.__type_ref + " " + string.join(type.postmod())
        self.__type_label = string.join(type.premod()) + " " + self.__type_label + " " + string.join(type.postmod())
            
    def visitParametrized(self, type):
	if type.template():
	    type.template().accept(self)
	    type_label = self.__type_label + "<"
	else: type_label = "(unknown)<"
        parameters_label = []
        for p in type.parameters():
            p.accept(self)
            parameters_label.append(self.__type_label)
        self.__type_label = type_label + string.join(parameters_label, ", ") + ">"

    def visitFunctionType(self, type):
	# TODO: this needs to be implemented
	self.__type_ref = 'function_type'
	self.__type_label = 'function_type'


    #################### AST Visitor ############################################
            
    def visitDeclarator(self, node):
        self.__declarator = node.name()
        for i in node.sizes():
            self.__declarator[-1] = self.__declarator[-1] + "[" + str(i) + "]"

    def visitTypedef(self, typedef):
        #self.write('@node ' + self.decl_label(typedef.name()) + '\n')
        self.write('@deftp ' + typedef.type() + ' {' + self.formatType(typedef.alias()) + '} {' + self.decl_label(typedef.name()) + '}\n')
        self.formatComments(typedef)
        self.write('@end deftp\n')
    def visitVariable(self, variable):
        #self.write('@node ' + self.decl_label(variable.name()) + '\n')
        self.write('@deftypevr {' + variable.type() + '} {' + self.formatType(variable.vtype()) + '} {' + self.decl_label(variable.name()) + '}\n')
        #FIXME !: how can values be represented in texinfo ?
        self.formatComments(variable)
        self.write('@end deftypevr\n')

    def visitConst(self, const):
        print "sorry, <const> not implemented"

    def visitModule(self, module):
        #self.write('@node ' + self.decl_label(module.name()) + '\n')
        self.write('@deftp ' + module.type() + ' ' + self.decl_label(module.name()) + '\n')
        self.formatComments(module)
        self.scope().append(module.name()[-1])
        #menu = MenuMaker(self.scope(), self.__os)
        #menu.start()
        #for declaration in module.declarations(): declaration.accept(menu)
        #menu.end()
        for declaration in module.declarations(): declaration.accept(self)
        self.scope().pop()
        self.write('@end deftp\n')

    def visitClass(self, clas):
        #self.write('@node ' + self.decl_label(clas.name()) + '\n')
        self.write('@deftp ' + clas.type() + ' ' + self.decl_label(clas.name()) + '\n')
        if len(clas.parents()):
            self.write('parents:')
            first = 1
            for parent in clas.parents():
                if not first: self.write(', ')
                else: self.write(' ')
                parent.accept(self)
            self.write('\n')
        self.formatComments(clas)
        self.scope().append(clas.name()[-1])
        #menu = MenuMaker(self.scope(), self.__os)
        #menu.start()
        #for declaration in clas.declarations(): declaration.accept(menu)
        #menu.end()
        for declaration in clas.declarations(): declaration.accept(self)
        self.scope().pop()
        self.write('@end deftp\n')

    def visitInheritance(self, inheritance):
        #map(lambda a, this=self: this.entity("modifier", a), inheritance.attributes())
        #self.entity("classname", Util.ccolonName(inheritance.parent().name(), self.scope()))
        self.write('parent class')

    def visitParameter(self, parameter):
        #map(lambda m, this=self: this.entity("modifier", m), parameter.premodifier())
        parameter.type().accept(self)
        label = self.write('{' + self.type_label() + '}')
        label = self.write(' @var{' + parameter.identifier() + '}')
        #map(lambda m, this=self: this.entity("modifier", m), parameter.postmodifier())

    def visitFunction(self, function):
        ret = function.returnType()
        if ret:
            ret.accept(self)
            ret_label = '{' + self.type_label() + '}'
        else:
            ret_label = '{}'
        #self.write('@node ' + self.decl_label(function.realname()) + '\n')
        self.write('@deftypefn ' + function.type() + ' ' + ret_label + ' ' + self.decl_label(function.realname()) + '(')
        first = 1
        for parameter in function.parameters():
            if not first: self.write(', ')
            else: self.write(' ')
            parameter.accept(self)
            first = 0
        self.write(')\n')
        #    map(lambda e, this=self: this.entity("exceptionname", e), operation.exceptions())
        self.formatComments(function)
        self.write('@end deftypefn\n')

    def visitOperation(self, operation):
        ret = operation.returnType()
        if ret:
            ret.accept(self)
            ret_label = '{' + self.type_label() + '}'
        else:
            ret_label = '{}'
        try:
            #self.write('@node ' + self.decl_label(operation.name()) + '\n')
            self.write('@deftypeop ' + operation.type() + ' ' + self.decl_label(self.scope()) + ' ' + ret_label + ' ' + self.decl_label(operation.realname()) + '(')
        except:
            print operation.realname()
            sys.exit(0)
        first = 1
        for parameter in operation.parameters():
            if not first: self.write(', ')
            else: self.write(' ')
            parameter.accept(self)
            first = 0
        self.write(')\n')
        #    map(lambda e, this=self: this.entity("exceptionname", e), operation.exceptions())
        self.formatComments(operation)
        self.write('@end deftypeop\n')

    def visitEnumerator(self, enumerator):
        self.write('@deftypevr {' + enumerator.type() + '} {} {' + self.decl_label(enumerator.name()) + '}')
        #FIXME !: how can values be represented in texinfo ?
        if enumerator.value(): self.write('\n')
        else: self.write('\n')
        self.formatComments(enumerator)
        self.write('@end deftypevr\n')
    def visitEnum(self, enum):
        #self.write('@node ' + self.decl_label(enum.name()) + '\n')
        self.write('@deftp ' + enum.type() + ' ' + self.decl_label(enum.name()) + '\n')
        self.formatComments(enum)
        for enumerator in enum.enumerators(): enumerator.accept(self)
        self.write('@end deftp\n')

def usage():
    """Print usage to stdout"""
    print \
"""
  -o <filename>                        Output filename"""

def __parseArgs(args):
    global output, verbose
    output = sys.stdout
    try:
        opts,remainder = getopt.getopt(args, "o:v")
    except getopt.error, e:
        sys.stderr.write("Error in arguments: " + str(e) + "\n")
        sys.exit(1)

    for opt in opts:
        o,a = opt
        if o == "-o": output = open(a, "w")
        elif o == "-v": verbose = 1

def format(args, ast, config):
    global output
    __parseArgs(args)
    #menu = MenuMaker([], output)
    #menu.start()
    #for d in declarations:
    #    d.accept(menu)
    #menu.end()
    formatter = Formatter(output)
    for d in ast.declarations():
        d.accept(formatter)
