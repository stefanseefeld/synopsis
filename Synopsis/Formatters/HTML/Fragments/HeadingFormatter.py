# $Id: HeadingFormatter.py,v 1.1 2003/12/05 22:31:53 stefan Exp $
#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis import AST, Type
from Synopsis.Formatters.HTML.Fragment import Fragment
from Synopsis.Formatters.HTML.Tags import *
import string

class HeadingFormatter(Fragment):
   """Formats the top of a page - it is passed only the Declaration that the
   page is for (a Module or Class)."""

   def format_name(self, scoped_name):
      """Formats a reference to each parent scope"""

      scope, text = [], []
      for name in scoped_name[:-1]:
         scope.append(name)
         text.append(self.reference(scope))
      text.append(anglebrackets(scoped_name[-1]))
      return string.join(text, "::\n") + '\n'

   def format_name_in_namespace(self, scoped_name):
      """Formats a reference to each parent scope, starting at the first
      non-module scope"""

      types = self.processor.ast.types()

      scope, text = [], []
      for name in scoped_name[:-1]:
         scope.append(name)
         if types.has_key(scope):
            ns_type = types[scope]
            if isinstance(ns_type, Type.Declared):
               decl = ns_type.declaration()
               if isinstance(decl, AST.Module):
                  # Skip modules (including namespaces)
                  continue
         text.append(self.reference(scope))
      text.append(anglebrackets(scoped_name[-1]))
      return string.join(text, "::\n") + '\n'

   def format_namespace_of_name(self, scoped_name):
      "Formats a reference to each parent scope and this one"

      types = self.processor.ast.types()

      scope, text = [], []
      last_decl = None
      for name in scoped_name:
         scope.append(name)
         if types.has_key(scope):
            ns_type = types[scope]
            if isinstance(ns_type, Type.Declared):
               decl = ns_type.declaration()
               if isinstance(decl, AST.Module):
                  # Only do modules and namespaces
                  text.append(self.reference(scope))
                  last_decl = decl
                  continue
         break
      return last_decl, string.join(text, "::") + '\n'

   def format_module(self, module):
      """Formats the module by linking to each parent scope in the name"""

      # Module details are only printed at the top of their page
      if not module.name():
         type, name = "Global", "Namespace"
      else:
         type = string.capitalize(module.type())
         name = self.format_name(module.name())
      name = entity('h1', "%s %s"%(type, name))
      return name

   def format_meta_module(self, module):
      """Calls formatModule"""

      return self.format_module(module)

   def format_class(self, clas):
      """Formats the class by linking to each parent scope in the name"""

      # Calculate the namespace string
      decl, namespace = self.format_namespace_of_name(clas.name())
      if decl:
         namespace = '%s %s'%(decl.type(), namespace)
         namespace = div('class-namespace', namespace)
      else:
         namespace = ''

      # Calculate template string
      templ = clas.template()
      if templ:
         params = templ.parameters()
         params = string.join(map(self.format_parameter, params), ', ')
         templ = div('class-template', "template &lt;%s&gt;"%params)
      else:
         templ = ''

      # Calculate class name string
      type = clas.type()
      name = self.format_name_in_namespace(clas.name())
      name = div('class-name', "%s %s"%(type, name))

      # Calculate file-related string
      file_name = rel(self.processor.output, clas.file().filename())
      # Try the file index page first
      file_link = self.processor.file_layout.file_index(clas.file().filename())
      if self.processor.filename_info(file_link):
         file_ref = href(rel(self.formatter.filename(), file_link), file_name, target="index")
      else:
         # Try source file next
         file_link = self.processor.file_layout.file_source(clas.file().filename())
         if self.processor.filename_info(file_link):
            file_ref = href(rel(self.formatter.filename(), file_link), file_name)
         else:
            file_ref = file_name
      files = "Files: "+file_ref + "<br>"

      return '%s%s%s%s'%(namespace, templ, name, files)

   def format_parameter(self, parameter):
      """Returns one string for the given parameter"""

      str = []
      keyword = lambda m,span=span: span("keyword", m)
      # Premodifiers
      str.extend(map(keyword, parameter.premodifier()))
      # Param Type
      typestr = self.format_type(parameter.type())
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
