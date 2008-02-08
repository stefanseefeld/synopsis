#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis import ASG
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
            
    def format_name(self, qname):
        """Formats a qualified name such that each name component becomes a link
        to the respective scope."""

        scope, text = type(qname)(), []
        for name in qname[:-1]:
            scope = scope + (name,)
            text.append(self.reference(scope))
        text.append(escape(qname[-1]))
        return '%s\n'%(qname.sep).join(text) + '\n'

    def format_name_in_module(self, qname):
        """Formats a reference to each parent scope, starting at the first
        non-module scope."""

        types = self.processor.ir.types

        scope, text = [], []
        for name in qname[:-1]:
            scope.append(name)
            if types.has_key(scope):
                ns_type = types[scope]
                if isinstance(ns_type, ASG.Declared):
                    decl = ns_type.declaration
                    if isinstance(decl, ASG.Module):
                        # Skip modules
                        continue
            text.append(self.reference(scope))
        text.append(escape(qname[-1]))
        return '%s\n'%qname.sep.join(text) + '\n'

    def format_module_of_name(self, qname):
        """Formats a reference to each parent scope and this one."""

        types = self.processor.ir.types

        scope, text = type(qname)(), []
        last_decl = None
        for name in qname:
            scope += (name,)
            if types.has_key(scope):
                ns_type = types[scope]
                if isinstance(ns_type, ASG.Declared):
                    decl = ns_type.declaration
                    if isinstance(decl, ASG.Module):
                        # Only do modules
                        text.append(self.reference(scope))
                        last_decl = decl
                        continue
            break
        return last_decl, qname.sep.join(text) + '\n'

    def format_module(self, module):
        """Formats the module by linking to each parent scope in the name."""

        # Module details are only printed at the top of their view
        if not module.name:
            title = 'Global %s'%module.type.capitalize()
        else:
            title = '%s %s'%(module.type.capitalize(), self.format_name(module.name))
        return element('h1', title)

    def format_meta_module(self, module):
        """Calls format_module."""

        return self.format_module(module)

    def format_class(self, class_):
        """Formats the class by linking to each parent scope in the name."""

        # Calculate the module string
        decl, module = self.format_module_of_name(class_.name)
        if decl:
            module = '%s %s'%(decl.type, module)
            module = div('class-module', module)
        else:
            module = ''

        # Calculate class name string
        type = class_.type
        name = self.format_name_in_module(class_.name)
        name = div('class-name', '%s %s'%(type, name))

        # Calculate file-related string
        file_name = rel(self.processor.output, class_.file.name)
        # Try the file index view first
        file_link = self.directory_layout.file_index(class_.file.name)
        if self.processor.filename_info(file_link):
            file_ref = href(rel(self.formatter.filename(), file_link), file_name, target='detail')
        else:
            # Try source file next
            file_link = self.directory_layout.file_source(class_.file.name)
            if self.processor.filename_info(file_link):
                file_ref = href(rel(self.formatter.filename(), file_link), file_name)
            else:
                file_ref = file_name

        links = div('file', 'File: %s'%file_ref)
        if self.xref: links += ' %s'%div('xref', self.xref.format_class(class_))
        if self.source: links += ' %s'%div('source', self.source.format_class(class_))
        info = div('links', links)

        return '%s%s%s'%(module, name, info)

    def format_class_template(self, class_):
        """Formats the class template by linking to each parent scope in the name."""

        # Calculate the module string
        decl, module = self.format_module_of_name(class_.name)
        if decl:
            module = '%s %s'%(decl.type, module)
            module = div('class-module', module)
        else:
            module = ''

        # Calculate template string
        templ = class_.template
        params = templ.parameters
        params = ', '.join([self.format_parameter(p) for p in params])
        templ = div('class-template', "template &lt;%s&gt;"%params)

        # Calculate class name string
        type = class_.type
        name = self.format_name_in_module(class_.name)
        name = div('class-name', '%s %s'%(type, name))

        # Calculate file-related string
        file_name = rel(self.processor.output, class_.file.name)
        # Try the file index view first
        file_link = self.directory_layout.file_index(class_.file.name)
        if self.processor.filename_info(file_link):
            file_ref = href(rel(self.formatter.filename(), file_link), file_name, target='detail')
        else:
            # Try source file next
            file_link = self.directory_layout.file_source(class_.file.name)
            if self.processor.filename_info(file_link):
                file_ref = href(rel(self.formatter.filename(), file_link), file_name)
            else:
                file_ref = file_name

        links = div('file', 'File: %s'%file_ref)
        if self.xref: links += ' %s'%div('xref', self.xref.format_class(class_))
        if self.source: links += ' %s'%div('source', self.source.format_class(class_))
        info = div('links', links)

        return '%s%s%s%s'%(module, templ, name, info)

    def format_forward(self, forward):
        """Formats the forward declaration if it is a template declaration."""

        # Calculate the module string
        decl, module = self.format_module_of_name(forward.name)
        if decl:
            module = '%s %s'%(decl.type, module)
            module = div('class-module', module)
        else:
            module = ''

        # Calculate template string
        if not forward.template:
            return ''

        params = forward.template.parameters
        params = ', '.join([self.format_parameter(p) for p in params])
        templ = div('class-template', "template &lt;%s&gt;"%params)

        # Calculate class name string
        type = forward.type
        name = self.format_name_in_module(forward.name)
        name = div('class-name', '%s %s'%(type, name))

        # Calculate file-related string
        file_name = rel(self.processor.output, forward.file.name)
        # Try the file index view first
        file_link = self.directory_layout.file_index(forward.file.name)
        if self.processor.filename_info(file_link):
            file_ref = href(rel(self.formatter.filename(), file_link), file_name, target='detail')
        else:
            # Try source file next
            file_link = self.directory_layout.file_source(forward.file.name)
            if self.processor.filename_info(file_link):
                file_ref = href(rel(self.formatter.filename(), file_link), file_name)
            else:
                file_ref = file_name

        links = div('file', 'File: %s'%file_ref)
        info = div('links', links)

        return '%s%s%s%s'%(module, templ, name, info)

    def format_parameter(self, parameter):
        """Returns one string for the given parameter"""

        chunks = []
        # Premodifiers
        chunks.extend([span("keyword", escape(m)) for m in parameter.premodifier])
        # Param Type
        typestr = self.format_type(parameter.type)
        if typestr: chunks.append(typestr)
        # Postmodifiers
        chunks.extend([span("keyword", m) for m in parameter.postmodifier])
        # Param name
        if len(parameter.name) != 0:
            chunks.append(span("variable", parameter.name))
        # Param value
        if len(parameter.value) != 0:
            chunks.append(" = " + span("value", parameter.value))
        return ' '.join(chunks)
