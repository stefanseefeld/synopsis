"""Dia Formatter.
This formatter outputs a file suitable for Dia. Currently diagram elements are
unpositioned.
"""

import sys, getopt, os, os.path, string
from Synopsis.Core import Type, AST, Util, Visitor

def k2a(keys):
    "Convert a keys dict to a string of attributes"
    return string.join(map(lambda item:' %s="%s"'%item, keys.items()), '')

def usage():
    print """\
Dia Formatter Usage:
  -m    hide methods/operations
  -a    hide attributes/variables
"""

class DiaFormatter (Visitor.AstVisitor, Visitor.TypeVisitor):
    """Outputs a Dia file
    """
    def __init__(self, filename):
        self.__indent = 0
        self.__istring = "  "
        self.__filename = filename
	self.__os = None
	self.__oidcount = 0
	self.__inherits = [] # list of from,to tuples
	self.__objects = {} #maps tuple names to object ids
    def indent(self): self.__os.write(self.__istring * self.__indent)
    def incr(self): self.__indent = self.__indent + 1
    def decr(self): self.__indent = self.__indent - 1
    def write(self, text): self.__os.write(text)
    def scope(self): return ()

    def startTag(self, tagname, **keys):
	"Starts a tag and indents, attributes using keyword arguments"
	self.indent()
	self.write("<%s%s>\n"%(tagname,k2a(keys)))
	self.incr()
    def startTagDict(self, tagname, attrs):
	"Starts a tag and indents, attributes using dictionary argument"
	self.indent()
	self.write("<%s%s>\n"%(tagname,k2a(attrs)))
	self.incr()
    def endTag(self, tagname):
	"Un-indents and closes tag"
	self.decr()
	self.indent()
	self.write("</%s>\n"%tagname)
    def soloTag(self, tagname, **keys):
	"Writes a solo tag with attributes from keyword arguments"
	self.indent()
	self.write("<%s%s/>\n"%(tagname,k2a(keys)))
    def attribute(self, name, type, value):
	"Writes an attribute with given name, type and value"
	self.startTag('attribute', name=name)
	if not value:
	    self.soloTag(type)
	elif type == 'string':
	    self.indent()
	    self.write('<string>#%s#</string>\n'%value)
	else:
	    self.soloTag(type, val=value)
	self.endTag('attribute')

    def output(self, declarations):
	"""Output declarations to file"""
	self.__os = open(filename, 'w')	

	self.write('<?xml version="1.0"?>\n')
	self.startTagDict('diagram', {'xmlns:dia':"http://www.lysator.liu.se/~alla/dia/"})

	self.doDiagramData()
	self.startTag('layer', name='Background', visible='true')
	for decl in declarations:
	    decl.accept(self)
	for inheritance in self.__inherits:
	    self.doInheritance(inheritance)
	self.endTag('layer')
	self.endTag('diagram')

	self.__os.close()

    def doDiagramData(self):
	"Write the stock diagramdata stuff"
	self.startTag('diagramdata')
	self.attribute('background', 'color', '#ffffff')
	self.startTag('attribute', name='paper')
	self.startTag('composite', type='paper')
	self.attribute('name','string','A4')
	self.attribute('tmargin','real','2.82')
	self.attribute('bmargin','real','2.82')
	self.attribute('lmargin','real','2.82')
	self.attribute('rmargin','real','2.82')
	self.attribute('is_portrait','boolean','true')
	self.attribute('scaling','real','1')
	self.attribute('fitto','boolean','false')
	self.endTag('composite')
	self.endTag('attribute')
	self.startTag('attribute', name='grid')
	self.startTag('composite', type='grid')
	self.attribute('width_x', 'real', '1')
	self.attribute('width_y', 'real', '1')
	self.attribute('visible_x', 'int', '1')
	self.attribute('visible_y', 'int', '1')
	self.endTag('composite')
	self.endTag('attribute')
	self.startTag('attribute', name='guides')
	self.startTag('composite', type='guides')
	self.soloTag('attribute', name='hguides')
	self.soloTag('attribute', name='vguides')
	self.endTag('composite')
	self.endTag('attribute')
	self.endTag('diagramdata')

    def doInheritance(self, inherit):
	"Create a generalization object for one inheritance"
	from_id, to_id = map(self.getObjectID, inherit)
	id = self.createObjectID(None)
	self.startTag('object', type='UML - Generalization', version='0', id=id)
	self.attribute('obj_pos', 'point', '1,0')
	self.attribute('obj_bb', 'rectangle', '0,0;2,2')
	self.startTag('attribute', name='orth_points')
	self.soloTag('point', val='1,0')
	self.soloTag('point', val='1,1')
	self.soloTag('point', val='0,1')
	self.soloTag('point', val='0,2')
	self.endTag('attribute')
	self.startTag('attribute', name='orth_orient')
	self.soloTag('enum', val='1')
	self.soloTag('enum', val='0')
	self.soloTag('enum', val='1')
	self.endTag('attribute')
	self.attribute('name', 'string', None)
	self.attribute('stereotype', 'string', None)
	self.startTag('connections')
	self.soloTag('connection', handle='0', to=from_id, connection='6')
	self.soloTag('connection', handle='1', to=to_id, connection='1')
	self.endTag('connections')
	self.endTag('object')
	

    def createObjectID(self, decl):
	"Creates a new object identifier, and remembers it with the given declaration"
	idstr = 'O'+str(self.__oidcount)
	if decl: self.__objects[decl.name()] = idstr
	self.__oidcount = self.__oidcount+1
	return idstr

    def getObjectID(self, decl):
	"Returns the stored identifier for the given object"
	try:
	    return self.__objects[decl.name()]
	except KeyError:
	    print "Warning: no ID for",decl.name()
	    return 0
    
    def formatType(self, type):
	"Returns a string representation for the given type"
	if type is None: return '(unknown)'
	type.accept(self)
	return self.__type
    
    #################### Type Visitor ###########################################

    def visitBaseType(self, type):
        self.__type = Util.ccolonName(type.name())
        
    def visitForward(self, type):
        self.__type = Util.ccolonName(type.name(), self.scope())
        
    def visitDeclared(self, type):
        self.__type = Util.ccolonName(type.name(), self.scope())
        
    def visitModifier(self, type):
        aliasStr = self.formatType(type.alias())
	premod = map(lambda x:x+" ", type.premod())
        self.__type = "%s%s%s"%(string.join(premod,''), aliasStr, string.join(type.postmod(),''))
            
    def visitParametrized(self, type):
	temp = self.formatType(type.template())
	params = map(self.formatType, type.parameters())
        self.__type = "%s<%s>"%(temp,string.join(params, ", "))

    def visitFunctionType(self, type):
	ret = self.formatType(type.returnType())
	params = map(self.formatType, type.parameters())
	self.__type = "%s(%s)(%s)"%(ret,string.join(type.premod(),''),string.join(params,", "))

    ################# AST visitor #################

    def visitDeclaration(self, decl):
	"Default is to not do anything with it"
	#print "Not writing",decl.type(), decl.name()
	pass

    def visitModule(self, decl):
	"Just traverse child declarations"
	# TODO: make a Package UML object and maybe link classes to it?
	for d in decl.declarations():
	    d.accept(self)

    def visitClass(self, decl):
	"Creates a Class object for one class, with attributes and operations"
	id = self.createObjectID(decl)
	self.startTag('object', type='UML - Class', version='0', id=id)
	self.attribute('objpos', 'point', '1,1')
	self.attribute('obj_bb', 'rectangle', '1,1;2,2')
	self.attribute('elem_corner', 'point', '1.05,1.05')
	self.attribute('elem_width', 'real', '1')
	self.attribute('elem_height', 'real', '5')
	self.attribute('name', 'string', Util.ccolonName(decl.name()))
	self.attribute('stereotype', 'string', None)
	self.attribute('abstract', 'boolean', 'false')
	self.attribute('suppress_attributes', 'boolean', 'false')
	self.attribute('suppress_operations', 'boolean', 'false')
	self.attribute('visible_attributes', 'boolean', hide_attributes and 'false' or 'true')
	self.attribute('visible_operations', 'boolean', hide_operations and 'false' or 'true')
	# Do attributes
	afilt = lambda d: d.type() == 'attribute' or d.type() == 'variable'
	attributes = filter(afilt, decl.declarations())
	if not len(attributes) or hide_attributes:
	    self.soloTag('attribute', name='attributes')
	else:
	    self.startTag('attribute', name='attributes')
	    for attr in attributes:
		self.startTag('composite', type='umlattribute')
		self.attribute('name', 'string', attr.name()[-1])
		if attr.type() == 'attribute':
		    self.attribute('type', 'string', self.formatType(attr.returnType()))
		else:
		    self.attribute('type', 'string', self.formatType(attr.vtype()))
		self.attribute('value', 'string', None)
		self.attribute('visibility', 'enum', '0')
		self.attribute('abstract', 'boolean', 'false')
		self.attribute('class_scope', 'boolean', 'false')
		self.endTag('composite')
	    self.endTag('attribute')
	# Do operations
	operations = filter(lambda d: d.type() == 'operation', decl.declarations())
	if not len(operations) or hide_operations:
	    self.soloTag('attribute', name='operations')
	else:
	    self.startTag('attribute', name='operations')
	    for oper in operations:
		self.startTag('composite', type='umloperation')
		self.attribute('name', 'string', oper.name()[-1])
		self.attribute('type', 'string', self.formatType(oper.returnType()))
		self.attribute('visibility', 'enum', '0')
		self.attribute('abstract', 'boolean', 'false')
		self.attribute('class_scope', 'boolean', 'false')
		self.soloTag('attribute', name='parameters')
		self.endTag('composite')
	    self.endTag('attribute')
	# Finish class object
	self.attribute('template', 'boolean', 'false')
	self.soloTag('attribute', name='templates')
	self.endTag('object')

	for inherit in decl.parents():
	    self.__inherits.append( (inherit.parent(), decl) )

def __parseArgs(args):
    global filename, hide_operations, hide_attributes
    filename = None
    hide_operations = hide_attributes = 0
    try:
        opts,remainder = getopt.getopt(args, "o:mah")
    except getopt.error, e:
        sys.stderr.write("Error in arguments: " + e + "\n")
        sys.exit(1)

    for opt in opts:
        o,a = opt

        if o == "-o":
            filename = a
	elif o == "-m":
	    hide_operations = 1
	elif o == "-a":
	    hide_attributes = 1
	elif o == "-h":
	    usage()
	    sys.exit(1)
    
    if filename is None:
	sys.stderr.write("Error: No output specified.\n")
	sys.exit(1)

def format(types, declarations, args):
    __parseArgs(args)

    formatter = DiaFormatter(filename)

    formatter.output(declarations)

