#!/usr/bin/env python
#
# Copyright (C) 2004 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

import sys, os, string, dircache, cgi, cPickle
import urllib, cgitb

cgitb.enable()

class Handler(object):

    def __init__(self):

        try: template = open(os.environ.get('SXR_TEMPLATE')).read()
        except:
            template = """
<html>
  <head><title>Synopsis Cross-Reference</title></head>
  <body>
    <h1>Synopsis Cross-Reference</h1>
    ###sxr-content###
  </body>
</html>"""

        self.template = string.split(template, "###sxr-content###")
        self.src_dir = os.environ.get('SXR_SRC_DIR', '.')
        self.xref_db = os.environ.get('SXR_XREF_DB', None)

        path = string.split(os.environ.get('PATH_INFO', '/'), '/')
        self.command = path[1]
        self.path_info = string.join(path[2:], '/')
        self.form = cgi.FieldStorage()
        self.script_name = os.environ.get('SCRIPT_NAME', None)

    def print_ident(self, file, line, text):
        return "<a href=\"/Source/%s.html#%s\">%s:%s: %s</a>"%(file, line,
                                                               file, line, text)

    def print_file(self, file, name = None):
        if not name: name = file
        name = name[:-5] # strip of trailing '.html'
        return "<a href=\"/Source/%s\">%s</a>"%(file, name)

    def dump(self, data, name):
        if not data.has_key(name): return
        print string.join(name, '::')
        target_data = data[name]
        if target_data[0]:
            print "<li>Defined at:<br>"
            print "<ul>"
            for file, line, scope in target_data[0]:
                print "<li>%s</li>"%(self.print_ident(file, line, string.join(scope,'::')))
            print "</ul></li>"
        if target_data[1]:
            print "<li>Called from:<br>"
            print "<ul>"
            for file, line, scope in target_data[1]:
                print "<li>%s</li>"%(self.print_ident(file, line, string.join(scope,'::')))
            print "</ul></li>"
        if target_data[2]:
            print "<li>Referenced from:<br>"
            print "<ul>"
            for file, line, scope in target_data[2]:
                print "<li>%s</li>"%(self.print_ident(file, line, string.join(scope,'::')))
            print "</ul></li>"

    def file(self):
        """Generate a file search listing"""

        print """
<table>
  <tr>
    <td colspan="2">
      Use this field to search for files by name.
    </td>
  </tr>
  <tr>
    <td valign="top">
      Find File:
    </td>
    <td>
      <form method="get" action="%(script)s/file">
        <input type="text" name="string" value="" size="15"/>
        <input type="submit" value="Find"/>
      </form>
    </td>
  </tr>
</table>
"""%{'script': self.script_name}

        if self.form.has_key('string'):
            file = self.form['string'].value

            command = 'cd %s && find . -name \'%s.html\' -print'%(self.src_dir, file)
            ins, outs = os.popen2(command)
            files = outs.readlines()
            if files:
                print '<ul>'
                for f in files:
                    print '<li>%s</li>'%(self.print_file(f[2:].strip()))
                print '</ul>'

    def ident(self):
        """Generate an identifier listing"""

        print """
<table>
  <tr>
    <td colspan="2">
      Use this field to find a particular function, variable, etc.
    </td>
  </tr>
  <tr>
    <td valign="top">
      Find Identifier:
    </td>
    <td>
      <form method="get" action="%(script)s/ident">
        <input type="text" name="string" value="" size="15"/>
        <input type="submit" value="Find"/>
      </form>
    </td>
  </tr>
</table>
"""%{'script' : self.script_name}

        if self.form.has_key('string'):
            ident = self.form['string']
            full = self.form.has_key('full')

            f = open(self.xref_db, 'rb')
            data, index = cPickle.load(f)
            f.close()

            if full:
                name = tuple(string.split(ident.value,'::'))
                found = False
                # Check for exact match
                if data.has_key(name):
                    print "Found exact match:<br>"
                    self.dump(data, name)
                    found = True
                # Search for last part of name in index
                if index.has_key(name[-1]):
                    matches = index[name[-1]]
                    print "Found (%d) possible matches:<br>"%(len(matches))
                    print '<ul>'
                    for name in matches:
                        self.dump(data, name)
                    print '</ul>'
                    found = True
                if not found:
                    print "No matches found<br>"

            elif index.has_key(ident.value):
                matches = index[ident.value]
                print "Found (%d) possible matches:<br>"%(len(matches))
                print '<ul>'
                for name in matches:
                    self.dump(data, name)
                print '</ul>'
            else:
                print "No matches found<br>"

    def run(self):
        
        print "Content-Type: text/html\n"
        print

        print self.template[0]

        if self.command == 'file': self.file()
        elif self.command == 'ident': self.ident()

        print self.template[1]

if __name__ == '__main__':

    handler = Handler()
    handler.run()
