# $Id: DeclarationFormatter.py,v 1.1 2003/12/05 22:31:53 stefan Exp $
#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis import AST, Type, Util
from Synopsis.Formatters.HTML.Fragment import Fragment
from Synopsis.Formatters.HTML.Tags import *

import string

class DeclarationFormatter(Fragment):
   """Base class for SummaryFormatter and DetailFormatter.
    
   The two classes SummaryFormatter and DetailFormatter are actually
   very similar in operation, and so most of their methods are defined here.
   Both of them print out the definition of the declarations, including type,
   parameters, etc. Some things such as exception specifications are only
   printed out in the detailed version.
   """
   col_sep = '<td class="summ-info">'
   row_sep = '</tr><tr><td class="summ-info">'
   whole_row = '<td class="summ-start" colspan="2">'

   def format_parameters(self, parameters):
      "Returns formatted string for the given parameter list"

      return string.join(map(self.format_parameter, parameters), ", ")

   def format_declaration(self, decl):
      """The default is to return no type and just the declarations name for
      the name"""

      return self.col_sep + self.label(decl.name())

   def format_forward(self, decl): return self.format_declaration(decl)
   def format_group(self, decl):

      return self.col_sep + ''

   def format_scope(self, decl):
      """Scopes have their own pages, so return a reference to it"""

      name = decl.name()
      link = rel(self.formatter.filename(),
                 self.processor.file_layout.scope(name))
      return self.col_sep + href(link, anglebrackets(name[-1]))

   def format_module(self, decl): return self.format_scope(decl)
   def format_meta_module(self, decl): return self.format_module(decl)
   def format_class(self, decl): return self.format_scope(decl)

   def format_typedef(self, decl):
      "(typedef type, typedef name)"

      type = self.format_type(decl.alias())
      return type + self.col_sep +  self.label(decl.name())

   def format_enumerator(self, decl):
      """This is only called by formatEnum"""

      self.format_declaration(decl)

   def format_enum(self, decl):
      "(enum name, list of enumerator names)"

      type = self.label(decl.name())
      name = map(lambda enumor:enumor.name()[-1], decl.enumerators())
      name = string.join(name, ', ')
      return type + self.col_sep + name

   def format_variable(self, decl):

      # TODO: deal with sizes
      type = self.format_type(decl.vtype())
      return type + self.col_sep + self.label(decl.name())

   def format_const(self, decl):
      "(const type, const name = const value)"

      type = self.format_type(decl.ctype())
      name = self.label(decl.name()) + " = " + decl.value()
      return type + self.col_sep + name

   def format_function(self, decl):
      "(return type, func + params + exceptions)"

      premod = self.format_modifiers(decl.premodifier())
      type = self.format_type(decl.returnType())
      name = self.label(decl.name(), decl.realname())
      # Special C++ functions  TODO: maybe move to a separate AST formatter...
      if decl.language() == 'C++' and len(decl.realname())>1:
         if decl.realname()[-1] == decl.realname()[-2]: type = '<i>constructor</i>'
         elif decl.realname()[-1] == "~"+decl.realname()[-2]: type = '<i>destructor</i>'
         elif decl.realname()[-1] == "(conversion)":
            name = "(%s)"%type
      params = self.format_parameters(decl.parameters())
      postmod = self.format_modifiers(decl.postmodifier())
      raises = self.format_exceptions(decl)
      type = '%s %s'%(premod,type)
      # Prevent linebreaks on shorter lines
      if len(type) < 60:
         type = replace_spaces(type)
      if decl.type() == "attribute": name = '%s %s %s'%(name, postmod, raises)
      else: name = '%s(%s) %s %s'%(name, params, postmod, raises)
      if decl.template():
         templ = 'template &lt;%s&gt;'%(self.format_parameters(decl.template().parameters()),)
         templ = div('template', templ)
         return self.whole_row + templ + self.row_sep + type + self.col_sep + name
      return type + self.col_sep + name

   # Default operation is same as function, and quickest way is to assign:
   def format_operation(self, decl): return self.format_function(decl)

   def format_parameter(self, parameter):
      """Returns one string for the given parameter"""

      str = []
      keyword = lambda m,span=span: span("keyword", m)
      # Premodifiers
      str.extend(map(keyword, parameter.premodifier()))
      # Param Type
      id_holder = [parameter.identifier()]
      typestr = self.format_type(parameter.type(), id_holder)
      if typestr: str.append(typestr)
      # Postmodifiers
      str.extend(map(keyword, parameter.postmodifier()))
      # Param identifier
      if id_holder and len(parameter.identifier()) != 0:
         str.append(span("variable", parameter.identifier()))
      # Param value
      if len(parameter.value()) != 0:
         str.append(" = " + span("value", parameter.value()))
      return string.join(str)

