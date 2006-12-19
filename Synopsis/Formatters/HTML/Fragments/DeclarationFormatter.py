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
from SourceLinker import SourceLinker
from XRefLinker import XRefLinker

class DeclarationFormatter(Fragment):
    """Base class for SummaryFormatter and DetailFormatter.
    
    The two classes SummaryFormatter and DetailFormatter are actually
    very similar in operation, and so most of their methods are defined here.
    Both of them print out the definition of the declarations, including type,
    parameters, etc. Some things such as exception specifications are only
    printed out in the detailed version.
    """

    def register(self, formatter):

        super(DeclarationFormatter, self).register(formatter)
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
            
    def format_parameters(self, parameters):
        """Returns formatted string for the given parameter list."""

        return ', '.join([self.format_parameter(p) for p in parameters])

    def format_declaration(self, decl):
        """The default is to return no type and just the declarations name for
        the name."""

        return div('synopsis', self.label(decl.name()))

    def format_forward(self, decl):

        return self.format_declaration(decl)

    def format_group(self, decl):

        return ''

    def format_scope(self, decl):
        """Scopes have their own views, so return a reference to it."""

        name = decl.name()
        link = rel(self.formatter.filename(),
                   self.directory_layout.scope(name))
        return href(link, escape(name[-1]))

    def format_module(self, decl):

        return self.format_scope(decl)

    def format_meta_module(self, decl):

        return self.format_module(decl)

    def format_class(self, decl):

        chunk = '%s'%div('synopsis', self.format_scope(decl))
        if self.xref: chunk += ' %s'%div('xref', self.xref.format_class(decl))
        if self.source: chunk += ' %s'%div('source', self.source.format_class(decl))
        return chunk

    def format_typedef(self, decl):
        "(typedef type, typedef name)"

        type = self.format_type(decl.alias())
        chunk = '%s'%div('synopsis', type + ' ' + self.label(decl.name()))
        if self.xref: chunk += ' %s'%div('xref', self.xref.format_class(decl))
        if self.source: chunk += ' %s'%div('source', self.source.format_class(decl))
        return chunk

    def format_enumerator(self, decl):
        """This is only called by formatEnum"""

        self.format_declaration(decl)

    def format_enum(self, decl):
        "(enum name, list of enumerator names)"

        type = self.label(decl.name())
        name = ', '.join([e.name()[-1] for e in decl.enumerators()])
        chunk = '%s'%div('synopsis', type + ' ' + name)
        if self.xref: chunk += ' %s'%div('xref', self.xref.format_class(decl))
        if self.source: chunk += ' %s'%div('source', self.source.format_class(decl))
        return chunk

    def format_variable(self, decl):

        # TODO: deal with sizes
        type = self.format_type(decl.vtype())
        chunk = '%s'%div('synopsis', type + ' ' + self.label(decl.name()))
        if self.xref: chunk += ' %s'%div('xref', self.xref.format_class(decl))
        if self.source: chunk += ' %s'%div('source', self.source.format_class(decl))
        return chunk

    def format_const(self, decl):
        "(const type, const name = const value)"

        type = self.format_type(decl.ctype())
        name = self.label(decl.name()) + " = " + decl.value()
        chunk = '%s'%div('synopsis', type + ' ' + name)
        if self.xref: chunk += ' %s'%div('xref', self.xref.format_class(decl))
        if self.source: chunk += ' %s'%div('source', self.source.format_class(decl))
        return chunk

    def format_function(self, decl):
        "(return type, func + params + exceptions)"

        premod = self.format_modifiers(decl.premodifier())
        type = self.format_type(decl.returnType())
        name = self.label(decl.name(), decl.realname())
        # Special C++ functions  TODO: maybe move to a separate AST formatter...
        if decl.file().annotations['language'] == 'C++' and len(decl.realname())>1:
            lt = decl.realname()[-2].find('<') # check whether this is a template
            sname = lt == -1 and decl.realname()[-2] or decl.realname()[-2][:lt]
            if decl.realname()[-1] == sname: type = '<i>constructor</i>'
            elif decl.realname()[-1] == '~'+sname: type = '<i>destructor</i>'
            elif decl.realname()[-1] == '(conversion)': name = '(%s)'%type
        params = self.format_parameters(decl.parameters())
        postmod = self.format_modifiers(decl.postmodifier())
        raises = self.format_exceptions(decl)
        # prepend the type by the premodifier(s)
        type = '%s %s'%(premod,type)
        # Prevent linebreaks on shorter lines
        if len(type) < 60:
            type = replace_spaces(type)
        if decl.type() == 'attribute': name = '%s %s %s'%(name, postmod, raises)
        else: name = '%s(%s) %s %s'%(name, params, postmod, raises)
        # treat template syntax like a premodifier
        if decl.template():
            templ = 'template &lt;%s&gt;'%(self.format_parameters(decl.template().parameters()),)
            templ = div('template', templ)
            type = '%s %s'%(templ, type)

        chunk = '%s'%div('synopsis', type + ' ' + name)
        if self.xref: chunk += ' %s'%div('xref', self.xref.format_class(decl))
        if self.source: chunk += ' %s'%div('source', self.source.format_class(decl))

        return chunk

    def format_operation(self, decl):

        # Default operation is same as function, and quickest way is to assign:
        return self.format_function(decl)

    def format_parameter(self, parameter):
        """Returns one string for the given parameter"""

        text = []
        # Premodifiers
        text.extend([span('keyword', m) for m in parameter.premodifier()])
        # Param Type
        id_holder = [parameter.identifier()]
        typestr = self.format_type(parameter.type(), id_holder)
        if typestr: text.append(typestr)
        # Postmodifiers
        text.extend([span('keyword', m) for m in parameter.postmodifier()])
        # Param identifier
        if id_holder and len(parameter.identifier()) != 0:
            text.append(' ' + span('variable', parameter.identifier()))
        # Param value
        if len(parameter.value()) != 0:
            text.append(' = ' + span('value', escape(parameter.value())))
        return ''.join(text)


class DeclarationSummaryFormatter(DeclarationFormatter):
    """Derives from BaseStrategy to provide summary-specific methods.
    Currently the only one is format_exceptions"""

    def format_exceptions(self, oper):
        """Returns a reference to the detail if there are any exceptions."""

        if len(oper.exceptions()):
            return self.reference(oper.name(), ' raises')
        return ''


class DeclarationDetailFormatter(DeclarationFormatter):
    """Provide detail-specific Declaration formatting."""

    def format_exceptions(self, oper):
        """Prints out the full exception spec"""

        if len(oper.exceptions()):
            raises = span('keyword', 'raises')
            exceptions = []
            for exception in oper.exceptions():
                exceptions.append(self.reference(exception.name()))
            exceptions = span('raises', ', '.join(exceptions))
            return '%s (%s)'%(raises, exceptions)
        else:
            return ''

    def format_enum(self, enum):

        name = span('keyword', 'enum') + ' ' + self.label(enum.name())
        enumors = ''.join([self.format_enumerator(e) for e in enum.enumerators()])
        return name + div('enum', enumors)

    def format_enumerator(self, enumerator):

        text = self.label(enumerator.name())
        if len(enumerator.value()):
            value = ' = ' + span('value', enumerator.value())
        else:
            value = ''
        doc = self.processor.documentation.details(enumerator, self.view)
        return div('enumerator','%s%s%s'%(text, value, doc))
