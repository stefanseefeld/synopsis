#
# Copyright (C) 2000 Stefan Seefeld
# Copyright (C) 2000 Stephen Davies
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

"""
Outputs the AST in plain ascii format similar to input.
"""

from Synopsis.Processor import Processor, Parameter
from Synopsis import Type, AST, Util

import sys, getopt, os, os.path, string

class Formatter(Processor, AST.Visitor, Type.Visitor):
   """
   outputs as ascii. This is to test for features
   still missing. The output should be compatible
   with the input...
   """

   bold_docs = Parameter(False, 'Bold docs')
   doc_color = Parameter(None, 'Colored docs, color = 0 to 15')

   def process(self, ast, **kwds):

      self.set_parameters(kwds)
      self.ast = self.merge_input(ast)
      self.__os = open(self.output, "w")
      self.__indent = 0
      self.__istring = "  "
      self.__scope = []
      self.__axs = AST.DEFAULT
      self.__axs_stack = []
      self.__axs_string = ('default:\n','public:\n','protected:\n','private:\n')
      self.__enumers = []
      self.__id_holder = None
      self.__os.write('hi there')
      if self.doc_color is not None: 
         self.doc_str = "\033[3%d%sm// %%s\033[m\n"%(
            self.doc_color % 8,
            (self.doc_color >= 8) and ";1" or "")
      elif self.bold_docs:
         self.doc_str = "\033[1m// %s\033[m\n"
      else:
         self.doc_str = "// %s\n"

      for declaration in self.ast.declarations():
         declaration.accept(self)
      self.__os.close()
      
      return self.ast

   def indent(self): self.__os.write(self.__istring * self.__indent)
   def incr(self): self.__indent = self.__indent + 1
   def decr(self): self.__indent = self.__indent - 1
   def scope(self): return self.__scope
   def set_scope(self, name): self.__scope = list(name)
   def enter_scope(self, name): self.__scope.append(name),self.incr()
   def leave_scope(self): self.__scope.pop(),self.decr()
   def write(self, text): self.__os.write(text)

   def format_type(self, type, id_holder = None):

      if type is None: return '(unknown)'
      if id_holder: self.__id_holder = id_holder
      type.accept(self)
      if id_holder: self.__id_holder = None
      return self.__type
    
   #################### Type Visitor ##########################################

   def visitBaseType(self, type):

      self.__type = Util.ccolonName(type.name())
        
   def visitDependent(self, type):

      self.__type = type.name()[-1]
        
   def visitUnknown(self, type):

      self.__type = Util.ccolonName(type.name(), self.scope())
        
   def visitDeclared(self, type):

      self.__type = Util.ccolonName(type.name(), self.scope())
        
   def visitModifier(self, type):

      aliasStr = self.format_type(type.alias())
      premod = map(lambda x:x+" ", type.premod())
      self.__type = "%s%s%s"%(string.join(premod,''), aliasStr,
                              string.join(type.postmod(),''))
            
   def visitParametrized(self, type):

      temp = self.format_type(type.template())
      params = map(self.format_type, type.parameters())
      self.__type = "%s<%s>"%(temp,string.join(params, ", "))

   def visitFunctionType(self, type):

      ret = self.format_type(type.returnType())
      params = map(self.format_type, type.parameters())
      premod = string.join(type.premod(),'')
      if self.__id_holder:
         ident = self.__id_holder[0]
         del self.__id_holder[0]
      else:
         ident = ''
      self.__type = "%s(%s%s)(%s)"%(ret,premod,ident,string.join(params,", "))

   def visitTemplate(self, type):

      self.visitDeclared(type)
      #self.__type = "template<"+string.join(map(self.format_type, type.parameters()),",")+">"+self.__type
	
   ### AST visitor

   def visitDeclaration(self, decl):

      axs = decl.accessibility()
      if axs != self.__axs:
         self.decr(); self.indent(); self.incr()
         self.write(self.__axs_string[axs])
         self.__axs = axs
      self.write_docs(decl.annotations.get('doc',''))

   def visitMacro(self, macro):

      self.visitDeclaration(macro)
      self.indent()
      params = ''
      if macro.parameters() is not None:
         params = '(' + string.join(macro.parameters(), ', ') + ')'
      self.write("#define %s%s %s\n"%(macro.name()[-1], params, macro.text()))
    
   def write_docs(self, docs):

      if docs:
         lines = docs.split('\n')
         for line in lines:
            self.indent()
            self.write(self.doc_str%line)

   def visitTypedef(self, typedef):

      self.visitDeclaration(typedef)
      self.indent()
      dstr = ""
      # Figure out the type:
      alias = self.format_type(typedef.alias())
      # Figure out the declarators:
      # for declarator in typedef.declarators():
      #     dstr = dstr + declarator.name()[-1]
      #     if declarator.sizes() is None: continue
      #     for size in declarator.sizes():
      # 	dstr = dstr + "[%d]"%size
      self.write("typedef %s %s;\n"%(alias, typedef.name()[-1]))

   def visitModule(self, module):

      self.visitDeclaration(module)
      self.indent()
      self.write("%s %s {\n"%(module.type(),module.name()[-1]))
      self.enter_scope(module.name()[-1])
      #for type in module.types(): type.output(self)
      for declaration in module.declarations():
         declaration.accept(self)
      self.leave_scope()
      self.indent()
      self.write("}\n")

   def visitMetaModule(self, module):

      self.visitDeclaration(module)
      for decl in module.module_declarations():
         self.visitDeclaration(decl)
      # since no docs:
      self.visitModule(module)

   def visitClass(self, clas):

      self.visitDeclaration(clas)
      self.indent()
      self.write("%s %s"%(clas.type(),clas.name()[-1]))
      if len(clas.parents()):
         self.write(": ")
         p = []
         for parent in clas.parents():
            p.append(self.format_type(parent.parent()))
            #p.append("%s"%(Util.ccolonName(parent.parent().name(),clas.name()),))
         self.write(string.join(p, ", "))
      self.write(" {\n")
      self.enter_scope(clas.name()[-1])
      self.__axs_stack.append(self.__axs)
      if clas.type() == 'struct': self.__axs = AST.PUBLIC
      elif clas.type() == 'class': self.__axs = AST.PRIVATE
      else: self.__axs = AST.DEFAULT
      #for type in clas.types(): type.output(self)
      #for operation in clas.operations(): operation.output(self)
      for declaration in clas.declarations():
         declaration.accept(self)
      self.__axs = self.__axs_stack.pop()
      self.leave_scope()
      self.indent()
      self.write("};\n")

   def visitInheritance(self, inheritance):

      for attribute in inheritance.attributes(): self.write(attribute + " ")
      self.write(inheritance.parent().identifier())

   def visitParameter(self, parameter):

      spacer = lambda x: str(x)+" "
      premod = string.join(map(spacer,parameter.premodifier()),'')
      id_holder = [parameter.identifier()]
      type = self.format_type(parameter.type(), id_holder)
      postmod = string.join(map(spacer,parameter.postmodifier()),'')
      name = ""
      value = ""
      if id_holder and len(parameter.identifier()) != 0:
         name = " " + parameter.identifier()
      if len(parameter.value()) != 0:
         value = " = %s"%parameter.value()
      self.__params.append(premod + type + postmod + name + value)

   def visitFunction(self, function):

      self.visitOperation(function)

   def visitOperation(self, operation):

      self.visitDeclaration(operation)
      self.indent()
      for modifier in operation.premodifier(): self.write(modifier + " ")
      retStr = self.format_type(operation.returnType())
      name = operation.realname()
      if operation.language() == "IDL" and operation.type() == "attribute":
         self.write("attribute %s %s"%(retStr,name[-1]))
      else:
         if operation.language() == "C++" and len(name)>1 and name[-1] in [name[-2],"~"+name[-2]]:
            self.write("%s("%name[-1])
         else:
            if retStr: self.write(retStr+" ")
            self.write("%s("%name[-1])
         self.__params = []
         for parameter in operation.parameters(): parameter.accept(self)
         params = string.join(self.__params, ", ")
         self.write(params + ")")
      for modifier in operation.postmodifier(): self.write(modifier + " ")
      self.write(";\n")

   def visitVariable(self, var):

      self.visitDeclaration(var)
      name = var.name
      self.indent()
      self.write("%s %s;\n"%(self.format_type(var.vtype()),var.name()[-1]))

   def visitEnum(self, enum):

      self.visitDeclaration(enum)
      self.indent()
      istr = self.__istring * (self.__indent+1)
      self.write("enum %s {"%enum.name()[-1])
      self.__enumers = []
      comma = ''
      for enumer in enum.enumerators():
         self.write(comma+'\n'+istr)
         enumer.accept(self)
         comma = ','
      self.write("\n")
      self.indent()
      self.write("}\n")

   def visitEnumerator(self, enumer):

      self.write_docs(enumer.annotations.get('docs', ''))
      if enumer.value() == "":
         self.write("%s"%enumer.name()[-1])
      else:
         self.write("%s = %s"%(enumer.name()[-1], enumer.value()))
    
   def visitConst(self, const):

      self.visitDeclaration(const)
      ctype = self.format_type(const.ctype())
      self.indent()
      self.__os.write("%s %s = %s;\n"%(ctype,const.name()[-1],const.value()))

def print_types(types):
    keys = types.keys()
    keys.sort()
    for name in keys:
	type = types[name]
	clas = type.__class__
	if isinstance(type, Type.Declared):
	    clas = type.declaration().__class__
	try:
	    print "%s\t%s"%(string.split(clas.__name__,'.')[-1], Util.ccolonName(name))
	except:
	    print "name ==",name
	    raise

