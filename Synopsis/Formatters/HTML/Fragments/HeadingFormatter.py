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
from SourceLinker import SourceLinker
from XRefLinker import XRefLinker

class HeadingFormatter(Fragment):
    """Formats the top of a view - it is passed only the Declaration that the
    view is for (a Module or Class)."""

    def register(self, formatter):

        super(HeadingFormatter, self).register(formatter)
        if self.processor.has_view('XRef'):
            self.xref = XRefLinker()
            self.xref.register(formatter)
        else:
            self.xref = None
        if self.processor.has_view('Source'):
            self.source = SourceLinker()        
            self.source.register(formatter)
        else:
            self.source = None
            
    def format_name(self, scoped_name):
        """Formats a reference to each parent scope"""

        scope, text = [], []
        for name in scoped_name[:-1]:
            scope.append(name)
            text.append(self.reference(scope))
        text.append(escape(scoped_name[-1]))
        return '::\n'.join(text) + '\n'

    def format_name_in_module(self, scoped_name):
        """Formats a reference to each parent scope, starting at the first
        non-module scope."""

        types = self.processor.ast.types()

        scope, text = [], []
        for name in scoped_name[:-1]:
            scope.append(name)
            if types.has_key(scope):
                ns_type = types[scope]
                if isinstance(ns_type, Type.Declared):
                    decl = ns_type.declaration()
                    if isinstance(decl, AST.Module):
                        # Skip modules
                        continue
            text.append(self.reference(scope))
        text.append(escape(scoped_name[-1]))
        return '::\n'.join(text) + '\n'

    def format_module_of_name(self, scoped_name):
        """Formats a reference to each parent scope and this one."""

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
                        # Only do modules
                        text.append(self.reference(scope))
                        last_decl = decl
                        continue
            break
        return last_decl, '::'.join(text) + '\n'

    def format_module(self, module):
        """Formats the module by linking to each parent scope in the name."""

        # Module details are only printed at the top of their view
        if not module.name():
            type, name = 'Global', 'Module'
        else:
            type = module.type().capitalize()
            name = self.format_name(module.name())
        name = entity('h1', '%s %s'%(type, name))
        return name

    def format_meta_module(self, module):
        """Calls format_module."""

        return self.format_module(module)

    def format_class(self, clas):
        """Formats the class by linking to each parent scope in the name."""

        # Calculate the module string
        decl, module = self.format_module_of_name(clas.name())
        if decl:
            module = '%s %s'%(decl.type(), module)
            module = div('class-module', module)
        else:
            module = ''

        # Calculate template string
        templ = clas.template()
        if templ:
            params = templ.parameters()
            params = ', '.join([self.format_parameter(p) for p in params])
            templ = div('class-template', "template &lt;%s&gt;"%params)
        else:
            templ = ''

        # Calculate class name string
        type = clas.type()
        name = self.format_name_in_module(clas.name())
        name = div('class-name', '%s %s'%(type, name))

        # Calculate file-related string
        file_name = rel(self.processor.output, clas.file().name)
        # Try the file index view first
        file_link = self.processor.file_layout.file_index(clas.file().name)
        if self.processor.filename_info(file_link):
            file_ref = href(rel(self.formatter.filename(), file_link), file_name, target="index")
        else:
            # Try source file next
            file_link = self.processor.file_layout.file_source(clas.file().name)
            if self.processor.filename_info(file_link):
                file_ref = href(rel(self.formatter.filename(), file_link), file_name)
            else:
                file_ref = file_name

        links = div('file', 'File: %s'%file_ref)
        if self.xref: links += ' %s'%div('xref', self.xref.format_class(clas))
        if self.source: links += ' %s'%div('source', self.source.format_class(clas))
        info = div('links', links)

        return '%s%s%s%s'%(module, templ, name, info)

    def format_parameter(self, parameter):
        """Returns one string for the given parameter"""

        chunks = []
        # Premodifiers
        chunks.extend([span("keyword", m) for m in parameter.premodifier()])
        # Param Type
        typestr = self.format_type(parameter.type())
        if typestr: chunks.append(typestr)
        # Postmodifiers
        chunks.extend([span("keyword", m) for m in parameter.postmodifier()])
        # Param identifier
        if len(parameter.identifier()) != 0:
            chunks.append(span("variable", parameter.identifier()))
        # Param value
        if len(parameter.value()) != 0:
            chunks.append(" = " + span("value", parameter.value()))
        return ' '.join(chunks)
