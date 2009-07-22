#
# Copyright (C) 2006 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Processor import Parameter
from Synopsis.Formatters.HTML.Tags import *
from Synopsis.Formatters.HTML.Markup import *
from docutils.nodes import *
from docutils.core import *
from docutils.parsers.rst import roles
import re, StringIO

def span(name, rawtext, text, lineno, inliner, options={}, content=[]):
    """Maps a rols to a <span class="role">...</span>."""

    node = inline(rawtext, text)
    node['classes'] = [name]
    return [node], []


class SummaryExtractor(NodeVisitor):
    """A SummaryExtractor creates a document containing the first sentence of
    a source document."""

    def __init__(self, document):

        NodeVisitor.__init__(self, document)
        self.summary = None
        

    def visit_paragraph(self, node):
        """Copy the paragraph but only keep the first sentence."""

        if self.summary is not None:
            return

        summary_pieces = []

        # Extract the first sentence.
        for child in node:
            if isinstance(child, Text):
                m = re.match(r'(\s*[\w\W]*?\.)(\s|$)', child.data)
                if m:
                    summary_pieces.append(Text(m.group(1)))
                    break
                else:
                    summary_pieces.append(Text(child))
            else:
                summary_pieces.append(child)
            
        self.summary = self.document.copy()
        para = node.copy()
        para[:] = summary_pieces
        self.summary[:] = [para]


    def unknown_visit(self, node):
        'Ignore all unknown nodes'

        pass


class RST(Formatter):
    """Format summary and detail documentation according to restructured text markup.
    """

    roles = Parameter({'equation':span},
                      "A (name->function) mapping of custom ReST roles.")

    def format(self, decl, view):

        formatter = self
        
        def ref(name, rawtext, text, lineno, inliner,
                options={}, content=[]):
            # This function needs to be local to be able to access
            # the current state in form of the formatter.

            name = utils.unescape(text)
            uri = formatter.lookup_symbol(name, decl.name[:-1])
            if uri:
                ref = rel(view.filename(), uri)
                node = reference(rawtext, name, refuri=ref, **options)
            else:
                node = emphasis(rawtext, name)
            return [node], []

        for r in self.roles:
            roles.register_local_role(r, self.roles[r])
        roles.register_local_role('', ref)

        errstream = StringIO.StringIO()
        settings = {}
        settings['halt_level'] = 2
        settings['warning_stream'] = errstream
        settings['traceback'] = True

        doc = decl.annotations.get('doc')
        if doc:
            try:
                doctree = publish_doctree(doc.text, settings_overrides=settings)
                # Extract the summary.
                extractor = SummaryExtractor(doctree)
                doctree.walk(extractor)

                reader = docutils.readers.doctree.Reader(parser_name='null')

                # Publish the summary.
                if extractor.summary:
                    pub = Publisher(reader, None, None,
                                    source=io.DocTreeInput(extractor.summary),
                                    destination_class=io.StringOutput)
                    pub.set_writer('html')
                    pub.process_programmatic_settings(None, None, None)
                    dummy = pub.publish(enable_exit_status=None)
                    summary = pub.writer.parts['html_body']
                    # Hack to strip off some redundant blocks to make the output
                    # more compact.
                    if (summary.startswith('<div class="document">\n') and
                        summary.endswith('</div>\n')):
                        summary=summary[23:-7]
                else:
                    summary = ''
                
                # Publish the details.
                pub = Publisher(reader, None, None,
                                source=io.DocTreeInput(doctree),
                                destination_class=io.StringOutput)
                pub.set_writer('html')
                pub.process_programmatic_settings(None, None, None)
                dummy = pub.publish(enable_exit_status=None)
                details = pub.writer.parts['html_body']
                # Hack to strip off some redundant blocks to make the output
                # more compact.
                if (details.startswith('<div class="document">\n') and
                    details.endswith('</div>\n')):
                    details=details[23:-7]
                
                return Struct(summary, details)
            
            except docutils.utils.SystemMessage, error:
                xx, line, message = str(error).split(':', 2)
                print 'In DocString attached to declaration at %s:%d:'%(decl.file.name,
                                                                        decl.line)
                print '  line %s:%s'%(line, message)

        return Struct('', '')
