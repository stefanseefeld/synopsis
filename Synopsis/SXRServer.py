#
# Copyright (C) 2006 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

import sys, os, os.path, fnmatch, cPickle

def escape(text):
    """escape special characters ('&', '\"', '<', '>')"""

    text = text.replace('&', '&amp;')
    text = text.replace('"', '&quot;')
    text = text.replace('<', '&lt;')
    text = text.replace('>', '&gt;')
    return text

default_template = """
<html>
  <head><title>Synopsis Cross-Reference</title></head>
  <body>
    <h1>Synopsis Cross-Reference</h1>
    @CONTENT@
  </body>
</html>
"""

file_search_form = """
<table class=\"form\">
  <tr>
    <td>Enter a file name to search:</td>
    <td>
      <form method=\"get\" action=\"%(script)s/file\">
        <input type="text" name="string" value="" size="15"/>
        <input type="submit" value="Find"/>
      </form>
    </td>
  </tr>
</table>
"""

ident_search_form = """
<table class=\"form\">
  <tr>
    <td>Enter a variable, type, or function name to search:</td>
    <td>
      <form method=\"get\" action=\"%(script)s/ident\">
        <input type="text" name="string" value="" size="15"/>
        <input type="submit" value="Find"/>
      </form>
    </td>
  </tr>
</table>
"""

class SXRServer:

    def __init__(self, root, cgi_url, src_url,
                 template_file = None,
                 sxr_prefix='/sxr'):

        self.cgi_url = cgi_url
        self.src_url = src_url
        self.src_dir = os.path.join(root, 'Source')
        xref_db = os.path.join(root, 'xref_db')
        self.data, self.index = cPickle.load(open(xref_db, 'rb'))

        if template_file:
            template = open(template_file).read()
        else:
            template = default_template
        self.template = template.split("@CONTENT@")


    def ident_ref(self, file, line, scope):

        if len(scope):
           text = '::'.join(scope)
        else:
           text = '<global scope>'
        return '<a href=\"%s/Source/%s.html#%s\">%s:%s: %s</a>'\
               %(self.src_url, file, line, file, line, escape(text))

        
    def file_ref(self, file, name = None):

        if not name: name = file
        name = name[:-5] # strip of trailing '.html'
        return "<a href=\"%s/Source/%s\">%s</a>"%(self.src_url, file, name)


    def list_refs(self, data, name):

        html = ''
        if not data.has_key(name): return '\n'
        html += '<h3>%s</h3>\n'%escape('::'.join(name))
        target_data = data[name]
        if target_data[0]:
            html += '<li>Defined at:<br/>\n'
            html += '<ul>\n'
            for file, line, scope in target_data[0]:
                html +=  '<li>%s</li>\n'%(self.ident_ref(file, line, scope))
            html += '</ul></li>\n'
        if target_data[1]:
            html += '<li>Called from:<br/>\n'
            html += '<ul>\n'
            for file, line, scope in target_data[1]:
                html += '<li>%s</li>\n'%(self.ident_ref(file, line, scope))
            html += '</ul></li>\n'
        if target_data[2]:
            html += '<li>Referenced from:<br/>\n'
            html += '<ul>\n'
            for file, line, scope in target_data[2]:
                html += '<li>%s</li>\n'%(self.ident_ref(file, line, scope))
            html += '</ul></li>\n'
        return html
    

    def search_file(self, pattern):
        """Generate a file search listing."""

        html = self.template[0]
        html += file_search_form%{'script': self.cgi_url}

        base_path_len = len(self.src_dir)
        def find(result, base, files):

           result.extend([os.path.join(base[base_path_len + 1:], file)
                          for file in files
                          if os.path.isfile(os.path.join(base, file)) and
                          fnmatch.fnmatch(os.path.splitext(file)[0], pattern)])

        result = []
        os.path.walk(self.src_dir, find, result)
        if result:
            html += '<ul>\n'
            for f in result:
                html += '<li>%s</li>\n'%(self.file_ref(f.strip()))
            html += '</ul>\n'
        html += self.template[1]

        return html
    

    def search_ident(self, name, qualified = False):
        """Generate an identifier listing."""

        html = self.template[0]
        html += ident_search_form%{'script' : self.cgi_url}

        if qualified:
            if '::' in name:
                name = tuple(name.split('::'))
            else:
                name = tuple(name.split('.'))
            found = False
            # Check for exact match
            if self.data.has_key(name):
                html += 'Found exact match:<br/>\n'
                html += self.list_refs(self.data, name)
                found = True
            # Search for last part of name in index
            if self.index.has_key(name[-1]):
                matches = self.index[name[-1]]
                html += 'Found (%d) possible matches:<br/>\n'%(len(matches))
                html += '<ul>\n'
                for name in matches:
                    html += self.list_refs(self.data, name)
                html += '</ul>\n'
                found = True
            if not found:
                html += "No matches found<br/>\n"

        elif self.index.has_key(name):
            matches = self.index[name]
            html += 'Found (%d) possible matches:<br/>\n'%(len(matches))
            html += '<ul>\n'
            for name in matches:
                html += self.list_refs(self.data, name)
            html += '</ul>\n'
        else:
            html += 'No matches found<br/>\n'
        html += self.template[1]
                
        return html
