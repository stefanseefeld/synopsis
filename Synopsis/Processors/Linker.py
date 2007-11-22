#
# Copyright (C) 2000 Stefan Seefeld
# Copyright (C) 2000 Stephen Davies
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Processor import Composite, Parameter
from Synopsis import ASG, Type

class Linker(Composite, ASG.Visitor, Type.Visitor):
   """Visitor that removes duplicate declarations"""

   remove_empty_modules = Parameter(True, 'Remove empty modules.')
   sort_modules = Parameter(True, 'Sort module content alphabetically.')

   def process(self, ir, **kwds):

      self.set_parameters(kwds)
      self.ir = self.merge_input(ir)

      root = ASG.MetaModule("", [])
      self.__scopes = [root]
      global_dict = {}
      self.__dict_map = {id(root) : global_dict}
      self.__dicts = [global_dict]

      self.types = self.ir.types
      
      try:
         for d in self.ir.declarations:
            d.accept(self)
         self.ir.declarations = root.declarations
      except TypeError, e:
         import traceback
         traceback.print_exc()
         print 'linker error :', e

      for file in self.ir.files.values():
         self.visit_source_file(file)

      if self.remove_empty_modules:
         import ModuleFilter
         self.ir = ModuleFilter.ModuleFilter().process(self.ir)

      if self.sort_modules:
         import ModuleSorter
         self.ir = ModuleSorter.ModuleSorter().process(self.ir)
         

      # now deal with the sub-processors, if any
      output = self.output
      self.ir = Composite.process(self, self.ir, input=[], output='')
      self.output = output
      
      return self.output_and_return_ir()

   def lookup(self, name):
      """look whether the current scope already contains
      a declaration with the given name"""

      return self.__dicts[-1].get(name)

   def append(self, declaration):
      """append declaration to the current scope"""

      self.__scopes[-1].declarations.append(declaration)
      self.__dicts[-1][declaration.name] = declaration

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

   def visit_base_type(self, type):

      if self.types.has_key(type.name):
         self.__type = self.types[type.name]

   def visit_unknown(self, type):

      if self.types.has_key(type.name):
         self.__type = self.types[type.name]

   def visit_declared(self, type):

      if self.types.has_key(type.name):
         self.__type = self.types[type.name]
      else:
         print "Couldn't find declared type:",type.name

   def visit_template(self, type):

      # Should be a Declared with the same name
      if not self.types.has_key(type.name):
         return
      declared = self.types[type.name]
      if isinstance(declared, Type.Unknown):
         #the type was declared in a file for which no ASG is retained
         return
      elif not isinstance(declared, Type.Declared):
         print "Warning: template declaration was not a declaration:",type.name,declared.__class__.__name__
         return
      decl = declared.declaration
      if not hasattr(decl, 'template'):
         #print "Warning: non-class/function template",type.name, decl.__class__.__name__
         return
      if decl.template:
         self.__type = decl.template
      else:
         print "Warning: template type disappeared:",type.name

   def visit_modifier(self, type):

      alias = self.link_type(type.alias)
      if alias is not type.alias:
         type.alias = alias
      self.__type = type

   def visit_array(self, type):

      alias = self.link_type(type.alias)
      if alias is not type.alias:
         type.alias = alias
      self.__type = type

   def visit_parametrized(self, type):

      templ = self.link_type(type.template)
      if templ is not type.template:
         type.template = templ
      type.parameters = [self.link_type(p) for p in type.parameters]
      self.__type = type

   def visit_function_type(self, type):

      ret = self.link_type(type.return_type)
      if ret is not type.return_type:
         type.return_type = ret
      type.parameters = [self.link_type(p) for p in type.parameters]
      self.__type = type

   #################### ASG Visitor ############################################

   def visit_source_file(self, file):
      """Resolves any duplicates in the list of declarations from this
      file"""

      types = self.types

      # Clear the list and refill it
      declarations = file.declarations
      file.declarations = []

      for d in declarations:
         # Try to find a replacement declaration
         if types.has_key(d.name):
            declared = types[d.name]
            if isinstance(type, Type.Declared):
               d = declared.declaration
         file.declarations.append(d)
        
      # TODO: includes.

   def visit_module(self, module):

      #hmm, we assume that the parent node is a MetaModule. Can that ever fail ?
      metamodule = self.lookup(module.name)
      if metamodule is None:
         metamodule = ASG.MetaModule(module.type,module.name)
         self.append(metamodule)
      elif not isinstance(metamodule, ASG.MetaModule):
         raise TypeError, 'symbol type mismatch: Synopsis.ASG.Module and %s both match "%s"'%(metamodule.__class__, '::'.join(module.name))

      metamodule.module_declarations.append(module)

      # Merge comments.
      self.merge_comments(metamodule, module)

      self.push(metamodule)
      for d in module.declarations:
         d.accept(self)
      module.declarations = []
      self.pop()


   def merge_comments(self, metamodule, module):
      """Append the module comments into the metamodule."""

      if module.annotations.has_key('comments'):
         new_comments = module.annotations['comments']
         metamodule.annotations.setdefault('comments', [])
         comments = metamodule.annotations['comments']
         if comments[-len(new_comments):] != new_comments:
            comments.extend(new_comments)


   def visit_meta_module(self, module):        

      #hmm, we assume that the parent node is a MetaModule. Can that ever fail ?
      metamodule = self.lookup(module.name)
      if metamodule is None:
         metamodule = ASG.MetaModule(module.type,module.name)
         self.append(metamodule)
      elif not isinstance(metamodule, ASG.MetaModule):
         raise TypeError, 'symbol type mismatch: Synopsis.ASG.MetaModule and %s both match "%s"'%(metamodule.__class__, '::'.join(module.name))
         
      metamodule.module_declarations.extend(module.module_declarations)
      self.merge_comments(metamodule, module)
      self.push(metamodule)
      for d in module.declarations:
         d.accept(self)
      module.declarations = []
      self.pop()


   def add_declaration(self, decl):
      """Adds a declaration to the current (top) scope.
      If there is already a Forward declaration, then this replaces it
      unless this is also a Forward.
      """

      name = decl.name
      dict = self.__dicts[-1]
      decls = self.top().declarations
      if dict.has_key(name):
         prev = dict[name]
         if not isinstance(prev, ASG.Forward):
            return
         if not isinstance(decl, ASG.Forward):
            decls.remove(prev)
            decls.append(decl)
            dict[name] = decl # overwrite prev
         return
      decls.append(decl)
      dict[name] = decl

   def visit_builtin(self, builtin):
      """preserve builtins unconditionally"""

      self.top().declarations.append(builtin)

   def visit_named(self, decl):

      name = decl.name
      if self.lookup(decl.name): return
      self.add_declaration(decl)

   visit_declaration = add_declaration
   visit_forward = add_declaration
   visit_enum = add_declaration

   def visit_function(self, func):

      if not isinstance(self.top(), ASG.Class):
         for d in self.top().declarations:
            if not isinstance(d, ASG.Function): continue
            if func.name == d.name:
               return
      ret = self.link_type(func.return_type)
      if ret is not func.return_type:
         func.return_type = ret
      for param in func.parameters:
         self.visit_parameter(param)
      self.top().declarations.append(func)


   visit_operation = visit_function

   def visit_variable(self, var):

      #if not scopedNameOkay(var.name): return
      vt = self.link_type(var.vtype)
      if vt is not var.vtype:
         var.vtype = vt
      self.add_declaration(var)

   def visit_typedef(self, tdef):

      alias = self.link_type(tdef.alias)
      if alias is not tdef.alias:
         tdef.alias = alias
      self.add_declaration(tdef)

   def visit_class(self, class_):

      prev = self.lookup(class_.name)
      if prev:
         if isinstance(prev, ASG.Forward):
            # Forward declaration, replace it
            self.top().declarations.remove(prev)
            del self.top_dict()[class_.name]
         elif isinstance(prev, ASG.Class):
            # Previous class. Would ignore duplicate but class_ may have
            # class declarations that prev doesn't. (forward declared
            # nested -- see ThreadData.hh for example)
            self.push(prev)
            for d in class_.declarations:
               if isinstance(d, ASG.Class):
                  d.accept(self)
            self.pop()
            return
         else:
            raise TypeError, 'symbol type mismatch: Synopsis.ASG.Class and %s both match "%s"'%(prev.__class__, '::'.join(class_.name))
      self.add_declaration(class_)
      for p in class_.parents:
         p.accept(self)
      declarations = class_.declarations
      class_.declarations = []
      self.push(class_)
      for d in declarations:
         d.accept(self)
      self.pop()

   def visit_inheritance(self, parent):

      type = parent.parent
      if isinstance(type, Type.Declared) or isinstance(type, Type.Unknown):
         ltype = self.link_type(type)
         if ltype is not type:
            parent.parent = ltype
      elif isinstance(type, Type.Parametrized):
         ltype = self.link_type(type.template)
         if ltype is not type.template:
            # Must find a Type.Template from it
            if not isinstance(ltype, Type.Declared):
               # Error
               return
            decl = ltype.declaration
            if isinstance(decl, ASG.Class):
               type.template = decl.template
      else:
         # Unknown type in class inheritance
         pass

   def visit_parameter(self, param):

      type = self.link_type(param.type)
      if type is not param.type:
         param.type = type

   def visit_const(self, const):

      ct = self.link_type(const.ctype)
      if ct is not const.ctype:
         const.ctype = ct
      self.add_declaration(const)
