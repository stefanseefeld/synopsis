# -*- coding: utf-8 -*-

import os, sys

from docutils import nodes
from sphinx import addnodes
from sphinx.directives.other import Index
from Synopsis.Processor import Parameter

class Processor(Index):

    has_content = True
    required_arguments = 1
    optional_arguments = 0
    final_argument_whitespace = False
    option_spec = {}

    def import_processor(self):
        try:
            module, name = self.arguments[0].rsplit('.', 1)
            __import__(module)
            self.module = sys.modules[module]
            self.processor = getattr(self.module, name)
            return True
        except Exception, err:
            #if self.env.app and not self.env.app.quiet:
            #    self.env.app.info(traceback.format_exc().rstrip())
            #self.directive.warn(
            #    'autodoc can\'t import/find %s %r, it reported error: '
            #    '"%s", please check your spelling and sys.path' %
            #    (self.objtype, str(self.fullname), err))
            #self.env.note_reread()
            raise
            return False


    def run(self):

        if not self.import_processor():
            return []
        # For index generation we only want the submodule's name inside 
        # the common 'Synopsis' name
        package, category, name = self.arguments[0].split('.', 2)
        self.arguments[0] = '%s ; %s'%(category, name)

        name = self.module.__name__
        section = nodes.section(name, nodes.title(name, name))
        section['ids'].append(name)
        section['names'].append(name)
        section += Index.run(self)
        desc = nodes.paragraph('')
        desc += nodes.Text(self.processor.__doc__)
        section += [desc]
        deflist = nodes.definition_list()
        section += [deflist]
        for n, p in self.processor._parameters.items():
            item = nodes.definition_list_item()
            deflist += [item]
            term = nodes.term()
            item += [term]
            definition = nodes.definition('')
            item += definition
            term += [nodes.Text(n, n)]
            definition += [nodes.paragraph('', nodes.Text(p.doc))]
        return [section]
        

def setup(app):
    app.add_directive('processor', Processor)
