# HTML2 Formatter by Stephen Davies
# Based on HTML Formatter by Stefan Seefeld
#
# Generates a page for each Module/Class and an inheritance tree
# Also 3-frame format inspired by Javadoc

# TODO:
# . TOC - Consider storing scoped name tuple instead of colonated name
#   o NOTE: TOC also stores types for some reason. investigate.
# . Unite Nodes/NamespaceBuilder into TOC

import sys, getopt, os, os.path, string, types
from Synopsis import Type, Util, Visitor

# Set this to true if your name is Chalky :)
debug=1

def k2a(keys):
    "Convert a keys dict to a string of attributes"
    return string.join(map(lambda item:' %s="%s"'%item, keys.items()), '')
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

def filename(basename, scope):
    """Calculate the filename from the given scope tuple"""
    scope = map(lambda x:'-'+x, scope)
    filename = basename + string.join(scope,'') + ".html"
    return filename

class Node:
    """
    A Node in the namespace tree. Each node has a name and a list of AST
    declarations where it is defined, as well as comments divided into summary
    and detail.
    """
    def __init__(self, name, declarations, scopeinfo = None):
	self.__name = tuple(name)
	if declarations is None:
	    self.__declarations = []
	    self.__type = 'module'
	elif type(declarations) == types.ListType:
	    self.__declarations = declarations
	    self.__type = declarations[0].type()
	else:
	    self.__declarations = [declarations]
	    self.__type = declarations.type()
	self.__type = string.capitalize(self.__type)
	self.__scopeinfo = scopeinfo or ScopeInfo(name, 0, 0)
	self.__scopeinfo.node = self
	self._calc_comments()
	toc.insert(self.__scopeinfo)

    def forceTypeTo(self, type):
	"""Force another type on this Node. This MUST be done before inserting
	into a namespace, or it will have no effect."""
	self.__type = type

    def _get_comments(self):
	"Template method to get the list of AST::Comments for this Node"
	comments = []
	for decl in self.__declarations:
	    comments.extend(decl.comments())
	return comments

    def _calc_comments(self):
	"Private method to calculate the comment/summary/detail of this Node"
	# Find all comments
	comments = self._get_comments()
	# Convert list to a single string
	block = filter(lambda s: s.text()[0:3] == "//.", comments)
	block = map(lambda s: s.text()[3:], block)
	self.__comment = comment = string.join(block,'\n')
	# Split into summary and detail
	index = string.find(comment, '.')
	if index != -1:
	    self.__summary = comment[:index]
	    self.__detail = comment
	    self.__scopeinfo.has_detail = 1
	else:
	    self.__summary = comment
	    self.__detail = None
	    self.__scopeinfo.has_detail = 0

    def name(self): return self.__name
    def type(self): return self.__type
    def scope_info(self): return self.__scopeinfo
    def comment(self): return self.__comment
    def summary(self): return self.__summary
    def detail(self): return self.__detail
    def declarations(self): return self.__declarations
    def add_declarations(self, decls):
	"Add further declarations for this Node"
	if type(decls) == types.ListType:
	    self.__declarations.extend(decls)
	else:
	    self.__declarations.append(decls)
	self._calc_comments()

class Namespace(Node):
    """
    A non-leaf Node, that contains other Nodes.
    This may be either a Module or a Class.
    """
    def __init__(self, name, declarations):
	Node.__init__(self, name, declarations, ScopeInfo(name, 1, 0))
	self.__types = {}
	self.__children = {}
	self.scope_info().is_file = 1
    def types(self):
	keys = self.__types.keys()
	keys.sort()
	return keys
    def has_child(self, name): return self.__children.has_key(tuple(name))
    def children(self, type=None):
	if type is None: return self.__children
	if self.__types.has_key(type): return self.__types[type]
	return {}
    def children_keys(self, type=None):
	keys = self.children(type).keys()
	keys.sort()
	return keys
    def add_child(self, child):
	if not self.__types.has_key(child.type()):
	    self.__types[child.type()] = {}
	self.__types[child.type()][child.name()] = child
	self.__children[child.name()] = child
    def get_or_create(self, name, declarations, Class):
	if self.has_child(name):
	    child = self.__children[tuple(name)]
	    child.add_declarations(declarations)
	    return child
	child = Class(name, declarations)
	self.add_child(child)
	return child

class DeclaratorNode(Node):
    """
    This is a special Node type for declarations whose comments are stored in
    declarator child nodes.
    FIXME: The AST is broken in this regard; fix it!
    """
    def __init__(self, name, declarations):
	Node.__init__(self, name, declarations)

    def _get_comments(self):
	comments = []
	for decl in self.declarations():
	    for dcor in decl.declarators():
		comments.extend(dcor.comments())
	return comments

class NamespaceBuilder(Visitor.AstVisitor):
    """
    A Linker the creates the Node/Namespace tree from the AST.
    """
    def __init__(self):
	self.__global = Namespace([], None)
	self.__scope = []
	self.__stack = [self.__global]
	toc.set_global(self.__global)
    def globalns(self): return self.__global
    def scope(self): return self.__scope
    def enterScope(self, subname): self.__scope.append(subname)
    def leaveScope(self): self.__scope.pop()
    def current(self): return self.__stack[-1]
    def enterNamespace(self, ns): self.__stack.append(ns)
    def leaveNamespace(self): self.__stack.pop()

    def visitAST(self, ast):
	for declaration in ast.declarations():
	    declaration.accept(self)
    def visitModule(self, module):
	name = module.name()[-1]
	self.enterScope(name)
	ns = self.current().get_or_create(self.scope(), module, Namespace)
	self.enterNamespace(ns)
	for declaration in module.declarations():
	    declaration.accept(self)
	self.leaveNamespace()
	self.leaveScope()
    def visitClass(self, clas):
	toc.add_class(clas.name())
	name = clas.name()[-1]
	self.enterScope(name)
	ns = Namespace(self.scope(), clas)
	self.current().add_child(ns)
	self.enterNamespace(ns)
	for declaration in clas.declarations():
	    declaration.accept(self)
	for parent in clas.parents():
	    toc.add_inheritance(parent.parent().name(), clas.name())
	self.leaveNamespace()
	self.leaveScope()
    def visitTypedef(self, typedef):
	for decl in typedef.declarators():
	    child = DeclaratorNode(decl.name(), typedef)
	    self.current().add_child(child)
    def visitOperation(self, oper):
	child = Node(oper.name(), oper)
	if oper.type() == 'attribute':
	    child.forceTypeTo('Attribute')
	self.current().add_child(child)
    def visitEnum(self, enum):
	self.current().add_child(Node(enum.name(), enum))
    def visitVariable(self, var):
	for decl in var.declarators():
	    child = DeclaratorNode(decl.name(), var)
	    self.current().add_child(child)
    def visitConst(self, const):
	self.current().add_child(Node(const.name(), const))


class ScopeInfo:
    """
    Simple struct that holds info about one scoped name in the TOC.
    Attributes:
      name -- scoped name
      is_file -- true if this scope links direct to a file
      has_detail -- true if this scope has detailed information
      link -- fully referenced link to this scope
    """
    def __init__(self, name, is_file, has_detail):
	self.name = name
	self.is_file = is_file
	self.has_detail = has_detail
	if is_file:
	    self.link = filename(basename, name)
	else:
	    link = filename(basename, name[:-1])
	    self.link = link + "#" + name[-1]

class TableOfContents(Visitor.AstVisitor):
    """
    Maintains a dictionary of all declarations which can be looked up to create
    cross references. Names are fully scoped.
    """
    def __init__(self):
	self.__toc = {}
	self.__global = None
	self.__subclasses = {}
	self.__classes = []
    
    def lookup(self, name):
        if self.__toc.has_key(name): return self.__toc[name]
	if string.find(name, ':') >= 0:
	    if debug: print "Warning: TOC lookup of",name,"failed!"
        return None

    def lookupName(self, name):
	return self.lookup(Util.ccolonName(name))

    def insert(self, info):
        self.__toc[Util.ccolonName(info.name)] = info

    def globalns(self): return self.__global
    def set_global(self, ns): self.__global = ns

    def add_inheritance(self, supername, subname):
	supername, subname = tuple(supername), tuple(subname)
	if not self.__subclasses.has_key(supername):
	    subs = self.__subclasses[supername] = []
	else:
	    subs = self.__subclasses[supername]
	if subname not in subs:
	    subs.append(subname)

    def subclasses(self, classname):
	classname = tuple(classname)
	if self.__subclasses.has_key(classname):
	    return self.__subclasses[classname]
	return []

    def classes(self): return self.__classes
    def add_class(self, name):
	name = tuple(name)
	if name not in self.__classes:
	    self.__classes.append(tuple(name))

class BaseFormatter:
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
    
    def is_summary(self): return 0 # FIXME: bad!
    # Access to generated values
    def type_ref(self): return self.__type_ref
    def type_label(self): return self.__type_label
    def declarator(self): return self.__declarator
    def parameter(self): return self.__parameter

    def reference(self, ref, label, **keys):
        """reference takes two strings, a reference (used to look up the symbol and generated the reference),
        and the label (used to actually write it)"""
	if ref is None: return label
        info = toc.lookup(ref)
	if info is None: return span('type', label)
        return apply(href, (info.link, label), keys)

    def referenceName(self, name, label=None, **keys):
	"""Same as reference but takes a tuple name"""
	ref = Util.ccolonName(name)
	if label is None: 
	    label = Util.ccolonName(name, self.scope()) or "Global Namespace"
	return apply(self.reference, (ref, label), keys)

    def label(self, ref):
	"""Create a label for the given scoped reference name"""
        scopeinfo = toc.lookup(Util.ccolonName(ref))
	if scopeinfo is None: return Util.ccolonName(ref)
	location = scopeinfo.link
        ref = Util.ccolonName(ref, self.scope())
	index = string.find(location, '#')
	if index >= 0: location = location[index+1:]
        return location and name(location, ref) or Util.ccolonName(ref)

    def formatModifiers(self, modifiers):
	"Returns a HTML string from the given list of modifiers"
	keyword = lambda m,span=span: span("keyword", m)
	return string.join(map(keyword, modifiers))

    def formatType(self, typeObj):
	"Returns a reference string for the given type object"
	if typeObj is None: return "(unknown)"
        typeObj.accept(self)
        return self.reference(self.type_ref(), self.type_label())

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
        self.__type_ref = type.name()
        self.__type_label = type.name()
        
    def visitForward(self, type):
        self.__type_ref = Util.ccolonName(type.name())
        self.__type_label = Util.ccolonName(type.name(), self.scope())
        
    def visitDeclared(self, type):
        self.__type_label = Util.ccolonName(type.name(), self.scope())
        self.__type_ref = Util.ccolonName(type.name())
        
    def visitModifier(self, type):
        alias = self.formatType(type.alias())
	pre = string.join(map(lambda x:x+" ", type.premod()), '')
	post = string.join(type.postmod(), '')
	self.__type_ref = None
        self.__type_label = "%s%s%s"%(pre,alias,post)
            
    def visitParametrized(self, type):
        type.template().accept(self)
        type_ref = self.__type_ref + "&lt;"
        type_label = self.__type_label + "&lt;"
        parameters_ref = []
        parameters_label = []
        for p in type.parameters():
            p.accept(self)
            parameters_ref.append(self.__type_ref)
            parameters_label.append(self.reference(self.__type_ref, self.__type_label))
        self.__type_ref = type_ref + string.join(parameters_ref, ", ") + "&gt;"
        self.__type_label = type_label + string.join(parameters_label, ", ") + "&gt;"

    #################### AST Visitor ############################################
    def visitTypedef(self, typedef):
	#FIXME: Only process given declarator
	#FIXME: This need to be done in AST
	type = self.formatType(typedef.alias())
        for i in typedef.declarators():
	    # if i == current_decl:
            name = self.label(self.getDeclaratorName(i))
	    self.writeSectionItem(type, name, i)

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
	str.append(self.formatType(parameter.type()))
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
	if oper.language() == 'C++':
	    if oper.name()[-1] == oper.name()[-2]: type = '<i>constructor</i>'
	    elif oper.name()[-1] == "~ "+oper.name()[-2]: type = '<i>destructor</i>'
	name = self.label(oper.name())
	params = self.formatParameters(oper.parameters())
	postmod = self.formatModifiers(oper.postmodifier())
	raises = self.formatOperationExceptions(oper)
	type = '%s %s'%(premod,type)
        if oper.type() == "attribute": name = '%s %s %s'%(name, postmod, raises)
	else: name = '%s(%s) %s %s'%(name, params, postmod, raises)
	self.writeSectionItem(type, name, oper)

    def visitVariable(self, variable):
        type = self.formatType(variable.vtype())
        for i in variable.declarators():
            # if i == current_decl
            name = self.label(self.getDeclaratorName(i))
	    self.writeSectionItem(type, name, i)


    def writeSectionStart(self, heading): pass
    def writeSectionEnd(self, heading): pass
    def writeSectionItem(self, type, name, node): pass

class SummaryFormatter(BaseFormatter):
    def __init__(self, toc):
	BaseFormatter.__init__(self, toc)

    def label(self, ref):
        scopeinfo = toc.lookup(Util.ccolonName(ref))
	if scopeinfo and scopeinfo.has_detail:
	    return self.referenceName(ref)
	return BaseFormatter.label(self, ref)
	
    def getSummary(self, node):
	name = node.name()
	info = toc.lookup(Util.ccolonName(name))
	if info is None: return ''
	comment = info.node.summary()
	if len(comment): return '<br>'+div('summary', comment)
	else: return ''

    def formatOperationExceptions(self, oper):
        if len(oper.exceptions()):
            return self.referenceName(oper.name(), " raises")
	return ''

    def writeSectionStart(self, heading):
	str = '<table border=1 width="100%%">'\
	'<col width="20%%"><col width="80%%">'\
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
	name = node.name()
	info = toc.lookup(Util.ccolonName(name))
	if info is None: return ''
	comment = info.node.detail()
	if len(comment): return '<br>'+div('desc', comment)
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
        return '%s %s'%( self.formatModifiers(inheritance.attributes()),
	    self.referenceName(inheritance.parent().name()))

    def writeSectionStart(self, heading):
	str = '<table border=1 width="100%%">'\
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
	info = toc.lookupName(module.name())
	if info and info.node.comment():
	    self.write(div('desc',info.node.comment())+'<br><br>')

    def visitClass(self, clas):
	# Class details are only printed at the top of their page
	type = string.capitalize(clas.type())
	name = self.formatParent(clas)
	self.write(entity('h1', "%s %s"%(type, name)))

	# Print any comments for this class
	info = toc.lookupName(clas.name())
	if info and info.node.comment():
	    self.write(div('desc',info.node.comment())+'<br>')

	# Print out a list of the parents
	if clas.parents():
	    self.write('<div class="superclasses">\n')
	    self.write("Superclasses:")
	    parents = map(self.formatInheritance, clas.parents())
	    self.write(string.join(parents, ", "))
	    self.write("</div>\n")

	# Print subclasses
	subs = toc.subclasses(clas.name())
	if subs:
	    self.write("Known subclasses: ")
	    refs = map(self.referenceName, subs)
	    self.write(string.join(refs, ", "))
	
	self.write("<br>")

    def visitFunction(self, function):
	# FIXME: Never seen a function used before (in IDL, anyway)
	print "Visiting Function"
	self.visitOperation(function)

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
        self.write("<div class=\"enum\">\n")
        for enumerator in enum.enumerators():
            enumerator.accept(self)
            self.write("<br>")
        self.write("</div>\n")

    def visitEnumerator(self, enumerator):
        self.write(self.label(enumerator.name()))
        if len(enumerator.value()):
            self.write(" = " + span("value", enumerator.value()))
	self.write(desc(enumerator.comments()))



class Paginator:
    """Visitor that takes the Namespace hierarchy and splits it into pages"""
    def __init__(self, globalns):
	self.__global = globalns
	self.summarizer = SummaryFormatter(toc)
	self.detailer = DetailFormatter(toc)
	self.__os = None
	self.__scope = []
	
	# Resolve start namespace using global namespace var
	self.__start = self.__global
	scope_names = string.split(namespace, "::")
	scope = []
	for scope_name in scope_names:
	    if not scope_name: break
	    scope.append(scope_name)
	    children = self.__start.children()
	    if children.has_key(tuple(scope)):
		child = children[tuple(scope)]
		if isinstance(child, Namespace):
		    self.__start = children[tuple(scope)]
		    continue
	    # Don't continue if scope traversal failed!
	    print "Fatal: Couldn't find child scope",scope
	    raise NameError, "I-cant-be-stuffed-making-my-own-exception"

    def scope(self): return self.__scope
    def write(self, text): self.__os.write(text)

    def getIndexFilename(self, module):
	"""Returns the filename for the index of the given module"""
	return filename(basename+'_index', module.name())

    def getModuleIndexFilename(self):
	"""Returns the filename for the module index"""
	return basename+"_index_all.html"

    def process(self):
	"""Creates all pages"""
	self.createNamespacePages()
	self.createModuleIndexFile()
	self.createModuleIndexes()
	self.createFramesFile()
	self.createInheritanceTree()

    def createNamespacePages(self):
	"""Creates a page for every Namespace"""
	self.__namespaces = [self.__start]
	while self.__namespaces:
	    ns = self.__namespaces.pop(0)
	    self.processNamespacePage(ns)

    def createModuleIndexFile(self):
	"""Create a page with an index of all modules"""
	self.startFile(self.getModuleIndexFilename(), "Module Index")
	tree = href(basename+"_tree.html", 'Tree', target='main')
	self.write(entity('b', "Module Index")+' | %s<br>'%tree)
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
	self.startFile(basename+"_frames.html", "Synopsis - Generated Documentation", body='')
	findex = self.getModuleIndexFilename()
	fmindex = self.getIndexFilename(self.__start)
	fglobal = filename(basename, self.__global.name())
	frame1 = solotag('frame', name='index', src=findex)
	frame2 = solotag('frame', name='module_index', src=fmindex)
	frame3 = solotag('frame', name='main', src=fglobal)
	frameset1 = entity('frameset', frame1+frame2, rows="30%,*")
	frameset2 = entity('frameset', frameset1+frame3, cols="200,*")
	self.write(frameset2)
	self.endFile()

    def createInheritanceTree(self):
	"""Creates a file with the inheritance tree"""
	classes = toc.classes()
	classes.sort()
	def root(name, toc=toc, Util=Util):
	    try:
		return not toc.lookup(Util.ccolonName(name)).node.declarations()[0].parents()
	    except:
		print "Filtering",Util.ccolonName(name),"failed:"
		print toc.lookup(Util.ccolonName(name)).node.declarations()[0].__dict__
		return 0
	try:
	    roots = filter(root, classes)
	except AttributeError:
	    # for some reason I get a declaration that has no parents() attribute
	    print "Failed."
	    return
	self.detailer.set_scope([])

	self.startFile(basename+"_tree.html", "Synopsis - Class Hierarchy")
	self.write(entity('h1', "Inheritance Tree"))
	self.write('<ul>')
	map(self.processClassInheritance, roots)
	self.write('</ul>')
	self.endFile()   

    def processClassInheritance(self, name):
	self.write('<li>')
	self.write(self.detailer.referenceName(name))
	parents = toc.lookup(Util.ccolonName(name)).node.declarations()[0].parents()
	if parents:
	    namer = lambda parent, Util=Util: Util.ccolonName(parent.parent().name())
	    self.write(' <i>(%s)</i>'%string.join(map(namer, parents), ", "))
	subs = toc.subclasses(name)
	if subs:
	    subs.sort()
	    self.write('<ul>')
	    map(self.processClassInheritance, subs)
	    self.write('</ul>\n')
	self.write('</li>')

    def processNamespacePage(self, ns):
	"""Creates a page for the given namespace"""
	details = {} # A hash of lists of detailed children by type

	# Open file and setup scopes
	self.startFileScope(ns.name())

	# Write heading
	if ns is self.__global: 
	    self.write(entity('h1', "Global Namespace"))
	else:
	    # Use the detailer to print an appropriate heading
	    ns.declarations()[0].accept(self.detailer)

	# Loop throught all the types of children
	self.printNamespaceSummaries(ns, details)
	self.printNamespaceDetails(details)
	self.endFile()

	# Queue child namespaces
	for child in ns.children().values():
	    if isinstance(child, Namespace):
		self.__namespaces.append(child)

    def printNamespaceSummaries(self, ns, details):
	"Print out the summaries from the given ns and note detailed items"
	for type in ns.types():
	    if type[-1] == 's': heading = type+'es Summary:'
	    else: heading = type+'s Summary:'
	    self.summarizer.writeSectionStart(heading)
	    # Get a sorted list of children of this type
	    dict, keys = ns.children(type), ns.children_keys(type)
	    for key in keys:
		# Print out summary for the child
		child = dict[key]
		# Print only for first declaration
		child.declarations()[0].accept(self.summarizer)
		# Check if need to add to detail list
		if child.detail() is not None and \
			not child.scope_info().is_file:
		    if not details.has_key(type): details[type] = []
		    details[type].append(child)
	    self.summarizer.writeSectionEnd(heading)

    def printNamespaceDetails(self, details):
	"Print out the details from the given dict of lists"
	types = details.keys()
	types.sort()
	for type in types:
	    heading = type+' Details:'
	    self.detailer.writeSectionStart(heading)
	    # Get the sorted list of children of this type
	    children = details[type]
	    for child in children:
		# Print only for first declaration
		child.declarations()[0].accept(self.detailer)
	    self.detailer.writeSectionEnd(heading)

    def indexModule(self, ns):
	"Write a link for this module and recursively visit child modules."
	# Print link to this module
	name = Util.ccolonName(ns.name()) or "Global Namespace"
	#if not name: name = "Global Namespace"
	link = self.getIndexFilename(ns)
	self.write(href(link, name, target='module_index') + '<br>')
	# Add children
	for key in ns.children_keys('Namespace'):
	    self.indexModule(ns.children('Namespace')[key])
	for key in ns.children_keys('Module'):
	    self.indexModule(ns.children('Module')[key])

    def processNamespaceIndex(self, ns):
	self.detailer.set_scope(ns.name())

	# Create file
	name = Util.ccolonName(ns.name()) or "Global Namespace"
	fname = self.getIndexFilename(ns)
	self.startFile(fname, name+" Index")
	self.write(entity('b', name+" Index"))

	# Make script to switch main frame upon load
	script = '<!--\nwindow.parent.frames["main"].location="%s";\n-->'
	script = script%filename(basename, ns.name())
	self.write(entity('script', script, language='Javascript'))

	# Loop throught all the types of children
	for type in ns.types():
	    if type[-1] == 's': heading = type+'es'
	    else: heading = type+'s'
	    self.write('<br>'+entity('i', heading)+'<br>')
	    # Get a sorted list of children of this type
	    dict, keys = ns.children(type), ns.children_keys(type)
	    for key in keys:
		# Print out summary for the child
		child = dict[key]
		self.write(self.detailer.referenceName(child.name(), target='main'))
		self.write('<br>')
	self.endFile()

	# Queue child namespaces
	for child in ns.children('Namespace').values():
	    self.__namespaces.append(child)
	for child in ns.children('Module').values():
	    self.__namespaces.append(child)

    def startFileScope(self, scope):
	"Start a new file from the given scope"
	fname = filename(basename, scope)
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

def __parseArgs(args):
    global basename, stylesheet, namespace
    basename = None
    stylesheet = ""
    namespace = ""
    try:
        opts,remainder = getopt.getopt(args, "o:s:n:")
    except getopt.error, e:
        sys.stderr.write("Error in arguments: " + e + "\n")
        sys.exit(1)

    for opt in opts:
        o,a = opt
        if o == "-o":
            basename = a #open(a, "w")
        elif o == "-s":
            stylesheet = a
	elif o == "-n":
	    namespace = a

def format(types, declarations, args):
    global basename, stylesheet, toc
    __parseArgs(args)
    if basename is None:
	print "ERROR: No output base specified. You must specify a base"\
	    "filename (not ending in .html!)"
	sys.exit(1)

    # Create table of contents index
    toc = TableOfContents()

    # Create a builder for the namespace
    nsb = NamespaceBuilder()

    if debug: print "HTML Formatter: Initialising TOC"
    # Add all declarations to the namespace tree
    for d in declarations:
        d.accept(nsb)
    
    if debug: print "HTML Formatter: Writing Pages..."
    # Create the pages
    Paginator(nsb.globalns()).process()
    if debug: print "HTML Formatter: Done!"

