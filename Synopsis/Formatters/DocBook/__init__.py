#
# Copyright (C) 2007 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

"""a DocBook formatter (producing Docbook 4.5 XML output"""

from Synopsis.Processor import Processor, Parameter
from Synopsis import ASG, Type, Util, ScopeSorter
from Syntax import *
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

_summary_syntax = {
    'IDL': Syntax,
    'C++': CxxSummarySyntax,
    'C': CxxSummarySyntax,
    'Python': Syntax
    }

_detail_syntax = {
    'IDL': Syntax,
    'C++': CxxDetailSyntax,
    'C': CxxDetailSyntax,
    'Python': Syntax
    }

class FormatterBase:

    def __init__(self, processor, output):
        self.processor = processor
        self.output = output
        self.__scope = ()
        self.__scopestack = []
        self.__indent = 0
        self.__elements = []

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

        self.__elements.append(type)
        param_text = ''
        if params: param_text = ' ' + ' '.join(['%s="%s"'%(p[0].lower(), p[1]) for p in params.items()])
        self.write('<' + type + param_text + '>')
        self.__indent = self.__indent + 2
        self.write('\n')

    def end_element(self):
        """Write the end of an element, starting with a newline"""

        type = self.__elements.pop()
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



class SummaryFormatter(FormatterBase, ASG.Visitor):
    """A SummaryFormatter."""

    def process_doc(self, decl):

        if decl.annotations.get('doc', ''):
            summary = self.processor.documentation.summary(decl)
            self.write(summary)

    #################### ASG Visitor ###########################################

    def visit_declaration(self, declaration):

        language = declaration.file.annotations['language']
        syntax = _summary_syntax[language](self.output)
        declaration.accept(syntax)
        syntax.finish()
        self.process_doc(declaration)
        

    visit_module = visit_declaration
    visit_class = visit_declaration
    def visit_meta_module(self, meta):
        self.visit_module(meta.module_declarations[0])

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

    def visit_declaration(self, declaration):

        language = declaration.file.annotations['language']
        syntax = _detail_syntax[language](self.output)
        declaration.accept(syntax)
        syntax.finish()
        self.process_doc(declaration)
        

    def visit_module(self, module):

        self.start_element('section')
        title = module.name and '%s %s'%(module.type, module.name[-1]) or 'Global Module'
        self.write_element('title', title)
        self.write('\n')

        sorter = ScopeSorter.ScopeSorter()
        sorter.sort(module)

        self.start_element('section')
        self.write_element('title', 'Summary')
        self.write('\n')
        summary = SummaryFormatter(self.processor, self.output)
        for s in sorter.keys():
            if s[-1] == 's': title = s + 'es Summary'
            else: title = s + 's Summary'
            self.start_element('section')
            self.write_element('title', title)
            for d in sorter[s]:
                d.accept(summary)
            self.end_element()
        self.end_element()
        self.write('\n')
        self.start_element('section')
        self.write_element('title', 'Details')
        self.write('\n')
        self.process_doc(module)
        self.push_scope(module.name)
        for s in sorter.keys():
            if s[-1] == 's': title = s + 'es Details'
            else: title = s + 's Details'
            self.start_element('section')
            self.write_element('title', title)
            for d in sorter[s]:
                d.accept(self)
            self.end_element()
        self.pop_scope()
        self.end_element()
        self.write('\n')
        self.end_element()
        self.write('\n')


    def visit_class(self, class_):

        self.start_element('section')
        title = '%s %s'%(class_.type, class_.name[-1])
        self.write_element('title', title)
        self.write('\n')

        sorter = ScopeSorter.ScopeSorter()
        sorter.sort(class_)

        self.start_element('section')
        self.write_element('title', 'Summary')
        self.write('\n')
        summary = SummaryFormatter(self.processor, self.output)
        summary.process_doc(class_)
        for s in sorter.keys():
            if s[-1] == 's': title = s + 'es Summary'
            else: title = s + 's Summary'
            self.start_element('section')
            self.write_element('title', title)
            for d in sorter[s]:
                d.accept(summary)
            self.end_element()
        self.end_element()
        self.write('\n')
        self.start_element('section')
        self.write_element('title', 'Details')
        self.write('\n')
        self.process_doc(class_)
        self.push_scope(class_.name)
        for s in sorter.keys():
            if s[-1] == 's': title = s + 'es Details'
            else: title = s + 's Details'
            self.start_element('section')
            self.write_element('title', title)
            for d in sorter[s]:
                d.accept(self)
            self.end_element()
        self.pop_scope()
        self.end_element()
        self.write('\n')
        self.end_element()
        self.write('\n')


    def visit_inheritance(self, inheritance):

        for a in inheritance.attributes: self.element("modifier", a)
        self.element("classname", Util.ccolonName(inheritance.parent.name, self.scope()))

    def visit_parameter(self, parameter):

        parameter.type.accept(self)

    def visit_function(self, function):
        
        print "sorry, <function> not implemented"

    def visit_operation(self, operation):

        language = operation.file.annotations['language']
        syntax = _detail_syntax[language](self.output)
        operation.accept(syntax)
        syntax.finish()

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
    title = Parameter(None, 'title to be used in top-level section')
   
    def process(self, ir, **kwds):

        self.set_parameters(kwds)
        self.ir = self.merge_input(ir)

        self.documentation = DocCache(self, self.markup_formatters)

        output = open(self.output, 'w')
        output.write('<section>\n')
        if self.title:
            output.write('<title>%s</title>\n'%self.title)
        detail_formatter = DetailFormatter(self, output)
        for d in self.ir.declarations:
            d.accept(detail_formatter)

        output.write('</section>\n')
        output.close()
      
        return self.ir
