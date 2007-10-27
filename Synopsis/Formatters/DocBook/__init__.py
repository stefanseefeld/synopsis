#
# Copyright (C) 2007 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

"""a DocBook formatter (producing Docbook 4.5 XML output"""

from Synopsis.Processor import Processor, Parameter
from Synopsis import ASG, Type, Util
from Markup.Javadoc import Javadoc
try:
    from Markup.RST import RST
except ImportError:
    from Markup import Formatter as RST

import sys, getopt, os, os.path, re

def escape(text):

    for p in [('&', '&amp;'), ('"', '&quot;'), ('<', '&lt;'), ('>', '&gt;'),]:
        text = text.replace(*p)
    return text

languages = {
    'IDL': 'idl',
    'C++': 'c++',
    'C': 'c',
    'Python': 'idl' # Hack: Python isn't recognized by the docbook XSL stylesheets (as of 1.72)
    }

class FormatterBase:

    def __init__(self, processor, output):
        self.processor = processor
        self.output = output
        self.__scope = ()
        self.__scopestack = []
        self.__indent = 0

    def scope(self): return self.__scope
    def push_scope(self, newscope):

        self.__scopestack.append(self.__scope)
        self.__scope = newscope

    def pop_scope(self):

        self.__scope = self.__scopestack[-1]
        del self.__scopestack[-1]

    def write(self, text):
        """Write some text to the output stream, replacing \n's with \n's and
        indents."""

        indent = ' ' * self.__indent
        self.output.write(text.replace('\n', '\n'+indent))

    def start_element(self, type, **params):
        """Write the start of an element, ending with a newline"""

        param_text = ''
        if params: param_text = ' ' + ' '.join(['%s="%s"'%(p[0].lower(), p[1]) for p in params.items()])
        self.write('<' + type + param_text + '>')
        self.__indent = self.__indent + 2
        self.write('\n')

    def end_element(self, type):
        """Write the end of an element, starting with a newline"""

        self.__indent = self.__indent - 2
        self.write('\n</' + type + '>')

    def write_element(self, type, body, **params):
        """Write a single element on one line (though body may contain
        newlines)"""

        param_text = ''
        if params: param_text = ' ' + ' '.join(['%s="%s"'%(p[0].lower(), p[1]) for p in params.items()])
        self.write('<' + type + param_text + '>')
        self.__indent = self.__indent + 2
        self.write(body)
        self.__indent = self.__indent - 2
        self.write('</' + type + '>')

    def element(self, type, body, **params):
        """Return but do not write the text for an element on one line"""

        param_text = ''
        if params: param_text = ' ' + ' '.join(['%s="%s"'%(p[0].lower(), p[1]) for p in params.items()])
        return '<%s%s>%s</%s>'%(type, param_text, body, type)

    def reference(self, ref, label):
        """reference takes two strings, a reference (used to look up the symbol and generated the reference),
        and the label (used to actually write it)"""

        location = self.__toc.lookup(ref)
        if location != '': return href('#' + location, label)
        else: return span('type', str(label))


    def label(self, ref):

        location = self.__toc.lookup(Util.ccolonName(ref))
        ref = Util.ccolonName(ref, self.scope())
        if location != '': return name('"' + location + '"', ref)
        else: return ref



class SummaryFormatter(FormatterBase, Type.Visitor, ASG.Visitor):
    """A SummaryFormatter."""

    #################### Type Visitor ##########################################

    def visit_base_type(self, type):

        self.__type_ref = Util.ccolonName(type.name)
        self.__type_label = Util.ccolonName(type.name)

    def visit_unknown(self, type):

        self.__type_ref = Util.ccolonName(type.name)
        self.__type_label = Util.ccolonName(type.name, self.scope())
        
    def visit_declared(self, type):

        self.__type_label = Util.ccolonName(type.name, self.scope())
        self.__type_ref = Util.ccolonName(type.name)

    def visit_modifier(self, type):

        type.alias.accept(self)
        self.__type_ref = ''.join(type.premod) + ' ' + self.__type_ref + ' ' + ''.join(type.postmod)
        self.__type_label = escape(''.join(type.premod) + ' ' + self.__type_label + ' ' + ''.join(type.postmod))

    def visit_parametrized(self, type):

        type.template.accept(self)
        type_label = self.__type_label + '&lt;'
        parameters_label = []
        for p in type.parameters:
            p.accept(self)
            parameters_label.append(self.__type_label)
        self.__type_label = type_label + ', '.join(parameters_label) + '&gt;'

    def visit_function_type(self, type):

        # TODO: this needs to be implemented
        self.__type_ref = 'function_type'
        self.__type_label = 'function_type'

    def process_doc(self, decl):

        if decl.annotations.get('doc', ''):
            summary = self.processor.documentation.summary(decl)
            self.write(summary)

    #################### ASG Visitor ###########################################

    def visit_declarator(self, node):

        self.__declarator = node.name
        for i in node.sizes:
            self.__declarator[-1] = self.__declarator[-1] + '[%d]'%i

    def visit_typedef(self, typedef):

        print "sorry, <typedef> not implemented"

    def visit_variable(self, variable):

        variable.vtype.accept(self)
        text = self.__type_label
        text += ' %s;'%variable.name[-1]
        self.write_element('synopsis', text)
        self.write('\n')
        self.process_doc(variable)

    def visit_const(self, const):

        print "sorry, <const> not implemented"

    def visit_module(self, module):

        self.start_element('section')
        self.write_element('title', '%s %s'%(module.type, module.name[-1]))
        self.write('\n')
        self.process_doc(module)
        self.push_scope(module.name)
        for d in module.declarations:
            d.accept(self)
        self.pop_scope()
        self.end_element('section')

    def visit_class(self, class_):

        self.start_element('section')
        self.write_element('title', '%s %s'%(class_.type, class_.name[-1]))
        self.write('\n')
        # Summary
        self.process_doc(class_)
        self.start_element('section')
        self.write_element('title', 'Members')
        self.write('\n')
        self.push_scope(class_.name)
        for d in class_.declarations:
            d.accept(self)
        self.pop_scope()
        #for p in class_.parents:
        #    p.accept(self)
        self.end_element("section")
        self.end_element("section")

    def visit_inheritance(self, inheritance):

        for a in inheritance.attributes: self.element("modifier", a)
        self.element("classname", Util.ccolonName(inheritance.parent.name, self.scope()))

    def visit_parameter(self, parameter):

        parameter.type.accept(self)

    def visit_function(self, function):
        
        print "sorry, <function> not implemented"

    def visit_operation(self, operation):

        self.__language = operation.file.annotations['language']
        text = ''
        if self.__language == 'IDL' and operation.type == 'attribute':
            operation.return_type.accept(self)
            text = '%s attribute %s%s;'%(''.join(operation.premodifiers),
                                         escape(self.__type_label), operation.real_name)
        else:
            ret = operation.return_type
            if ret:
                ret.accept(self)
                text = '%s '%self.__type_label
            elif self.__language == 'Python':
                text = 'def '
            text += operation.real_name[-1]
            text += '('
            params = []
            for p in operation.parameters:
                p.accept(self)
                param_text = ' '.join(p.premodifier + [self.__type_label] + [p.name] + p.postmodifier)
                if p.value:
                    param_text += '= %s'%p.value
                params.append(param_text)
            text += ', '.join(params)
            text += ');'
        self.write_element('synopsis', text)
        self.write('\n')

    def visit_enumerator(self, enumerator):
        print "sorry, <enumerator> not implemented"

    def visit_enum(self, enum):
        print "sorry, <enum> not implemented"


class DetailFormatter(FormatterBase, Type.Visitor, ASG.Visitor):

    #################### Type Visitor ##########################################

    def visit_base_type(self, type):

        self.__type_ref = Util.ccolonName(type.name)
        self.__type_label = Util.ccolonName(type.name)

    def visit_unknown(self, type):

        self.__type_ref = Util.ccolonName(type.name)
        self.__type_label = Util.ccolonName(type.name, self.scope())
        
    def visit_declared(self, type):

        self.__type_label = Util.ccolonName(type.name, self.scope())
        self.__type_ref = Util.ccolonName(type.name)

    def visit_modifier(self, type):

        type.alias.accept(self)
        self.__type_ref = ''.join(type.premod) + ' ' + self.__type_ref + ' ' + ''.join(type.postmod)
        self.__type_label = escape(''.join(type.premod) + ' ' + self.__type_label + ' ' + ''.join(type.postmod))

    def visit_parametrized(self, type):

        type.template.accept(self)
        type_label = self.__type_label + '&lt;'
        parameters_label = []
        for p in type.parameters:
            p.accept(self)
            parameters_label.append(self.__type_label)
        self.__type_label = type_label + ', '.join(parameters_label) + '&gt;'

    def visit_function_type(self, type):

        # TODO: this needs to be implemented
        self.__type_ref = 'function_type'
        self.__type_label = 'function_type'

    def process_doc(self, decl):

        if not decl.annotations.get('doc', ''):
            return
        detail = self.processor.documentation.details(decl)
        self.write(self.element('para', detail or ''))

    #################### ASG Visitor ###########################################

    def visit_declarator(self, node):

        self.__declarator = node.name
        for i in node.sizes:
            self.__declarator[-1] = self.__declarator[-1] + '[%d]'%i

    def visit_typedef(self, typedef):

        print "sorry, <typedef> not implemented"

    def visit_variable(self, variable):

        variable.vtype.accept(self)
        text = self.__type_label
        text += ' %s;'%variable.name[-1]
        self.write_element('synopsis', text)
        self.write('\n')
        self.process_doc(variable)

    def visit_const(self, const):

        print "sorry, <const> not implemented"

    def visit_module(self, module):

        self.start_element('section')
        self.write_element('title', '%s %s'%(module.type, module.name[-1]))
        self.write('\n')
        self.start_element('section')
        self.write_element('title', 'Summary')
        self.write('\n')
        summary = SummaryFormatter(self.processor, self.output)
        for d in module.declarations:
            d.accept(summary)
        self.end_element('section')
        self.write('\n')
        self.start_element('section')
        self.write_element('title', 'Details')
        self.write('\n')
        self.process_doc(module)
        self.push_scope(module.name)
        for d in module.declarations:
            d.accept(self)
        self.pop_scope()
        self.end_element('section')
        self.write('\n')
        self.end_element('section')
        self.write('\n')

    def visit_class(self, class_):

        self.start_element('section')
        self.write_element('title', '%s %s'%(class_.type, class_.name[-1]))
        self.write('\n')
        self.start_element('section')
        self.write_element('title', 'Summary')
        self.write('\n')
        summary = SummaryFormatter(self.processor, self.output)
        for d in class_.declarations:
            d.accept(summary)
        self.end_element('section')
        self.write('\n')
        self.start_element('section')
        self.write_element('title', 'Details')
        self.write('\n')
        self.process_doc(class_)
        self.push_scope(class_.name)
        for d in class_.declarations:
            d.accept(self)
        self.pop_scope()
        self.end_element('section')
        self.write('\n')
        self.end_element('section')
        self.write('\n')


    def visit_inheritance(self, inheritance):

        for a in inheritance.attributes: self.element("modifier", a)
        self.element("classname", Util.ccolonName(inheritance.parent.name, self.scope()))

    def visit_parameter(self, parameter):

        parameter.type.accept(self)

    def visit_function(self, function):
        
        print "sorry, <function> not implemented"

    def visit_operation(self, operation):

        self.__language = operation.file.annotations['language']
        text = ''
        if self.__language == 'IDL' and operation.type == 'attribute':
            operation.return_type.accept(self)
            text = '%s attribute %s%s;'%(''.join(operation.premodifiers),
                                         self.__type_label, operation.real_name)
        else:
            ret = operation.return_type
            if ret:
                ret.accept(self)
                text = '%s '%self.__type_label
            elif self.__language == 'Python':
                text = 'def '
            text += operation.real_name[-1]
            text += '('
            params = []
            for p in operation.parameters:
                p.accept(self)
                param_text = ' '.join(p.premodifier + [self.__type_label] + [p.name] + p.postmodifier)
                if p.value:
                    param_text += '= %s'%p.value
                params.append(param_text)
            text += ', '.join(params)
            text += ');'
        self.write_element('synopsis', text)
        self.write('\n')

    def visit_enumerator(self, enumerator):
        print "sorry, <enumerator> not implemented"

    def visit_enum(self, enum):
        print "sorry, <enum> not implemented"

   
class DocCache:
    """"""

    def __init__(self, processor, markup_formatters):

        self._processor = processor
        self._markup_formatters = markup_formatters
        # Make sure we have a default markup formatter.
        if '' not in self._markup_formatters:
            self._markup_formatters[''] = Markup.Formatter()
        for f in self._markup_formatters.values():
            f.init(self._processor)
        self._doc_cache = {}


    def _process(self, decl):
        """Return the documentation for the given declaration."""

        key = id(decl)
        if key not in self._doc_cache:
            doc = decl.annotations.get('doc')
            if doc:
                formatter = self._markup_formatters.get(doc.markup,
                                                        self._markup_formatters[''])
                doc = formatter.format(decl)
            else:
                doc = Markup.Struct()
            self._doc_cache[key] = doc
            return doc
        else:
            return self._doc_cache[key]


    def summary(self, decl):
        """"""

        doc = self._process(decl)
        return doc.summary


    def details(self, decl):
        """"""

        doc = self._process(decl)
        return doc.details



class Formatter(Processor, Type.Visitor, ASG.Visitor):
    """The type visitors should generate names relative to the current scope.
    The generated references however are fully scoped names
    """

    markup_formatters = Parameter({'rst':RST()},
                                  'Markup-specific formatters.')
   
    def process(self, ir, **kwds):

        self.set_parameters(kwds)
        self.ir = self.merge_input(ir)

        self.documentation = DocCache(self, self.markup_formatters)

        output = open(self.output, 'w')
        output.write('<section>\n')
        detail_formatter = DetailFormatter(self, output)

        for d in self.ir.declarations:
            d.accept(detail_formatter)

        output.write('</section>\n')
        output.close()
      
        return self.ir
