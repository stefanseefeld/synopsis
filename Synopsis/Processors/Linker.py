#
# Copyright (C) 2000 Stefan Seefeld
# Copyright (C) 2000 Stephen Davies
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

import string

from Synopsis.Processor import Composite, Parameter
from Synopsis import AST, Type

class Linker(Composite, AST.Visitor, Type.Visitor):
   """Visitor that removes duplicate declarations"""

   remove_empty_modules = Parameter(True, 'Remove empty modules.')

   def process(self, ast, **kwds):

      self.set_parameters(kwds)
      self.ast = self.merge_input(ast)

      root = AST.MetaModule("", "",[])
      self.__scopes = [root]
      global_dict = {}
      self.__dict_map = {id(root) : global_dict}
      self.__dicts = [global_dict]

      self.types = self.ast.types()
      
      declarations = self.ast.declarations()
      try:
         for decl in declarations: decl.accept(self)
         declarations[:] = root.declarations()
      except TypeError, e:
         print 'linker error :', e

      for file in self.ast.files().values():
         self.visitSourceFile(file)

      if self.remove_empty_modules:
         import ModuleFilter
         self.ast = ModuleFilter.ModuleFilter().process(self.ast)

      # now deal with the sub-processors, if any
      output = self.output
      self.ast = Composite.process(self, self.ast, input=[], output='')
      self.output = output
      
      return self.output_and_return_ast()

   def lookup(self, name):
      """look whether the current scope already contains
      a declaration with the given name"""

      if self.__dicts[-1].has_key(name):
         return self.__dicts[-1][name]
      return None

   def append(self, declaration):
      """append declaration to the current scope"""

      self.__scopes[-1].declarations().append(declaration)
      self.__dicts[-1][declaration.name()] = declaration

   def push(self, scope):
      """push new scope on the stack"""

      self.__scopes.append(scope)
      dict = self.__dict_map.setdefault(id(scope), {})
      self.__dicts.append(dict)

   def pop(self):
      """restore the previous scope"""

      del self.__scopes[-1]
      del self.__dicts[-1]

   def top(self):

      return self.__scopes[-1]

   def top_dict(self):

      return self.__dicts[-1]

   def link_type(self, type):
      "Returns the same or new proxy type"

      self.__type = type
      if type is not None: type.accept(self)
      return self.__type

   #################### Type Visitor ###########################################

   def visitBaseType(self, type):

      if self.types.has_key(type.name()):
         self.__type = self.types[type.name()]

   def visitUnknown(self, type):

      if self.types.has_key(type.name()):
         self.__type = self.types[type.name()]

   def visitDeclared(self, type):

      if self.types.has_key(type.name()):
         self.__type = self.types[type.name()]
      else:
         print "Couldn't find declared type:",type.name()

   def visitTemplate(self, type):

      # Should be a Declared with the same name
      if not self.types.has_key(type.name()):
         return
      declared = self.types[type.name()]
      if isinstance(declared, Type.Unknown):
         #the type was declared in a file for which no AST is retained
         return
      elif not isinstance(declared, Type.Declared):
         print "Warning: template declaration was not a declaration:",type.name(),declared.__class__.__name__
         return
      decl = declared.declaration()
      if not hasattr(decl, 'template'):
         #print "Warning: non-class/function template",type.name(), decl.__class__.__name__
         return
      if decl.template():
         self.__type = decl.template()
      else:
         print "Warning: template type disappeared:",type.name()

   def visitModifier(self, type):

      alias = self.link_type(type.alias())
      if alias is not type.alias():
         type.set_alias(alias)
      self.__type = type

   def visitArray(self, type):

      alias = self.link_type(type.alias())
      if alias is not type.alias():
         type.set_alias(alias)
      self.__type = type

   def visitParametrized(self, type):

      templ = self.link_type(type.template())
      if templ is not type.template():
         type.set_template(templ)
      params = tuple(type.parameters())
      del type.parameters()[:]
      for param in params:
         type.parameters().append(self.link_type(param))
      self.__type = type

   def visitFunctionType(self, type):

      ret = self.link_type(type.returnType())
      if ret is not type.returnType():
         type.set_returnType(ret)
      params = tuple(type.parameters())
      del type.parameters()[:]
      for param in params:
         type.parameters().append(self.link_type(param))
      self.__type = type

   #################### AST Visitor ############################################

   def visitSourceFile(self, file):
      """Resolves any duplicates in the list of declarations from this
      file"""

      types = self.types

      # Clear the list and refill it
      old_decls = list(file.declarations())
      new_decls = file.declarations()
      new_decls[:] = []

      for decl in old_decls:
         # Try to find a replacement declaration
         if types.has_key(decl.name()):
            declared = types[decl.name()]
            if isinstance(type, Type.Declared):
               decl = declared.declaration()
         new_decls.append(decl)
        
      # TODO: includes.

   def visitModule(self, module):

      #hmm, we assume that the parent node is a MetaModule. Can that ever fail ?
      metamodule = self.lookup(module.name())
      if metamodule is None:
         metamodule = AST.MetaModule(module.language(), module.type(),module.name())
         self.append(metamodule)

      elif not isinstance(metamodule, AST.MetaModule):
         raise TypeError, 'symbol type mismatch: Synopsis.AST.Module and %s both match "%s"'%(metamodule.__class__, '::'.join(module.name()))

      metamodule.module_declarations().append(module)
      self.merge_comments(metamodule.comments(), module.comments())
      self.push(metamodule)
      decls = tuple(module.declarations())
      del module.declarations()[:]
      for decl in decls: decl.accept(self)
      self.pop()

   def merge_comments(self, dest, src):
      """Merges the src comments into dest. Merge is just an append, unless
      src already exists inside dest!"""

      texter = lambda x: x.text()
      dest_str = map(texter, dest)
      src_str = map(texter, src)
      if dest_str[-len(src):] == src_str: return
      dest.extend(src)

   def visitMetaModule(self, module):        

      #hmm, we assume that the parent node is a MetaModule. Can that ever fail ?
      metamodule = self.lookup(module.name())
      if metamodule is None:
         metamodule = AST.MetaModule(module.language(), module.type(),module.name())
         self.append(metamodule)
      elif not isinstance(metamodule, AST.MetaModule):
         raise TypeError, 'symbol type mismatch: Synopsis.AST.MetaModule and %s both match "%s"'%(metamodule.__class__, '::'.join(module.name()))
         
      metamodule.module_declarations().extend(module.module_declarations())
      metamodule.comments().extend(module.comments())
      self.push(metamodule)
      decls = tuple(module.declarations())
      del module.declarations()[:]
      for decl in decls: decl.accept(self)
      self.pop()

   def add_declaration(self, decl):
      """Adds a declaration to the current (top) scope.
      If there is already a Forward declaration, then this replaces it
      unless this is also a Forward.
      """
      
      name = decl.name()
      dict = self.__dicts[-1]
      decls = self.top().declarations()
      if dict.has_key(name):
         prev = dict[name]
         if not isinstance(prev, AST.Forward):
            return
         if not isinstance(decl, AST.Forward):
            decls.remove(prev)
            decls.append(decl)
            dict[name] = decl # overwrite prev
         return
      decls.append(decl)
      dict[name] = decl

   def visitBuiltin(self, builtin):
      """preserve builtins unconditionally"""

      decls = self.top().declarations()
      decls.append(builtin)

   def visitNamed(self, decl):

      name = decl.name()
      if self.lookup(decl.name()): return
      self.add_declaration(decl)

   visitDeclaration = add_declaration
   visitForward = add_declaration
   visitEnum = add_declaration

   def visitFunction(self, func):

      if not isinstance(self.top(), AST.Class):
         for decl in self.top().declarations():
            if not isinstance(decl, AST.Function): continue
            if func.name() == decl.name():
               return
      ret = self.link_type(func.returnType())
      if ret is not func.returnType():
         func.set_returnType(ret)
      for param in func.parameters():
         self.visitParameter(param)
      self.top().declarations().append(func)

   visitOperation = visitFunction

   def visitVariable(self, var):

      #if not scopedNameOkay(var.name()): return
      vt = self.link_type(var.vtype())
      if vt is not var.vtype():
         var.set_vtype(vt)
      self.add_declaration(var)

   def visitTypedef(self, tdef):

      alias = self.link_type(tdef.alias())
      if alias is not tdef.alias():
         tdef.set_alias(alias)
      self.add_declaration(tdef)

   def visitClass(self, clas):

      name = clas.name()
      prev = self.lookup(name)
      if prev:
         if isinstance(prev, AST.Forward):
            # Forward declaration, replace it
            self.top().declarations().remove(prev)
            del self.top_dict()[name]
         elif isinstance(prev, AST.Class):
            # Previous class. Would ignore duplicate but clas may have
            # class declarations that prev doesn't. (forward declared
            # nested -- see ThreadData.hh for example)
            self.push(prev)
            for decl in clas.declarations():
               if isinstance(decl, AST.Class):
                  decl.accept(self)
            self.pop()
            return
         else:
            raise TypeError, 'symbol type mismatch: Synopsis.AST.Class and %s both match "%s"'%(prev.__class__, '::'.join(name))

      self.add_declaration(clas)
      for parent in clas.parents():
         parent.accept(self)
      self.push(clas)
      decls = tuple(clas.declarations())
      del clas.declarations()[:]
      for decl in decls: decl.accept(self)
      self.pop()

   def visitInheritance(self, parent):

      type = parent.parent()
      if isinstance(type, Type.Declared) or isinstance(type, Type.Unknown):
         ltype = self.link_type(type)
         if ltype is not type:
            parent.set_parent(ltype)
      elif isinstance(type, Type.Parametrized):
         ltype = self.link_type(type.template())
         if ltype is not type.template():
            # Must find a Type.Template from it
            if not isinstance(ltype, Type.Declared):
               # Error
               return
            decl = ltype.declaration()
            if isinstance(decl, AST.Class):
               type.set_template(decl.template())
      else:
         # Unknown type in class inheritance
         pass

   def visitParameter(self, param):

      type = self.link_type(param.type())
      if type is not param.type():
         param.set_type(type)

   def visitConst(self, const):

      ct = self.link_type(const.ctype())
      if ct is not const.ctype():
         const.set_ctype(ct)
      self.add_declaration(const)
