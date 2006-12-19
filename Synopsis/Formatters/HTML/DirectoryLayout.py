#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis import Util, AST
from Synopsis.Formatters import TOC
from Tags import *

import os, os.path, sys, stat

class DirectoryLayout (TOC.Linker):
    """DirectoryLayout defines how the generated html files are organized.
    The default implementation uses a flat layout with all files being part
    of a single directory."""

    def init(self, processor):

        self.processor = processor
        self.base = self.processor.output
        if not os.path.exists(self.base):
            if processor.verbose: print "Warning: Output directory does not exist. Creating."
            try:
                os.makedirs(self.base, 0755)
            except EnvironmentError, reason:
                print "ERROR: Creating directory:",reason
                sys.exit(2)
        elif not os.path.isdir(self.base):
            print "ERROR: Output must be a directory."
            sys.exit(1)

        if os.path.isfile(processor.stylesheet):
            self.copy_file(processor.stylesheet, 'style.css')
        else:
            print "ERROR: stylesheet %s doesn't exist"%processor.stylesheet
            sys.exit(1)

    def copy_file(self, src, dest):
        """Copy src to dest, if dest doesn't exist yet or is outdated."""

        dest = os.path.join(self.base, dest)
        try:
            filetime = os.stat(src)[stat.ST_MTIME]
            if (not os.path.exists(dest)
                or filetime > os.stat(dest)[stat.ST_MTIME]):
                Util.open(dest).write(open(src, 'r').read())
        except EnvironmentError, reason:
            import traceback
            traceback.print_exc()
            print "ERROR: ", reason
            sys.exit(2)
    
    def _check_main(self, filename):
        """Checks whether the given filename is the main index view or not. If
        it is, then it returns the filename from index(), else it returns
        it unchanged"""

        if filename == self.processor.main_index: return self.index()
        return filename
	
    def scope(self, scope = None):
        """Return the filename of a scoped name (class or module).
        The default implementation is to join the names with '-' and append
        ".html" Additionally, special characters are Util.quoted in URL-style"""

        if not scope: return self._check_main(self.special('global'))
        return Util.quote('.'.join(scope)) + '.html'

    def _strip(self, filename):

        if len(filename) and filename[-1] == '/': filename = filename[:-1]
        return filename

    def file_index(self, filename):
        """Return the filename for the index of an input file.
        Default implementation is to join the path with '.', prepend '_file.'
        and append '.html' """

        filename = self._strip(filename)
        return '_file.%s.html'%filename.replace(os.sep, '.')

    def file_source(self, filename):
        """Return the filename for the source of an input file.
        Default implementation is to join the path with '.', prepend '_source.'
        and append '.html' """

        file = self._strip(filename)
        return '_source.'+file.replace(os.sep, '.')+'.html'

    def file_details(self, filename):
        """Return the filename for the details of an input file.
        Default implementation is to join the path with '.', prepend
        '_file_detail.' and append '.html' """

        return '_file_detail.'+self._strip(filename).replace(os.sep, '.')+'.html'

    def index(self):
        """Return the name of the main index file. Default is index.html"""

        return 'index.html'

    def special(self, name):
        """Return the name of a special file (tree, etc). Default is
        _name.html"""

        return self._check_main('_%s.html'%name)

    def scoped_special(self, name, scope, ext='.html'):
        """Return the name of a special type of scope file. Default is to join
        the scope with '.' and prepend '.'+name"""
        
        return "_%s%s%s"%(name, Util.quote('.'.join(scope)), ext)

    def xref(self, page):
        """Return the name of the xref file for the given page"""

        return '_xref%d.html'%page

    def module_tree(self):
        """Return the name of the module tree index. Default is
        _modules.html"""

        return '_modules.html'

    def module_index(self, scope):
        """Return the name of the index of the given module. Default is to
        join the name with '.', prepend '_module' and append '.html' """
        
        return Util.quote('_module' + '.'.join(scope)) + '.html'
	
    def link(self, decl):
        """Create a link to the named declaration. This method may have to
        deal with the directory layout."""
        
        if isinstance(decl, AST.Scope):
            # This is a class or module, so it has its own file
            return self.scope(decl.name())
        # Assume parent scope is class or module, and this is a <A> name in it
        filename = self.scope(decl.name()[:-1])
        anchor = escape(decl.name()[-1].replace(' ','.'))
        return filename + '#' + anchor

class NestedDirectoryLayout(DirectoryLayout):
    """Organizes files in a directory tree."""

    def scope(self, scope = None):

        if not scope: return self._check_main('Scopes/global.html')
        else: return Util.quote('Scopes/' + '/'.join(scope)) + '.html'

    def file_index(self, filename):

        return 'File/%s.html'%self._strip(filename)

    def file_source(self, filename):

        return 'Source/%s.html'%self._strip(filename)

    def file_details(self, filename):

        return 'FileDetails/%s.html'%self._strip(filename)

    def special(self, name):

        return self._check_main(name + '.html')
    
    def scoped_special(self, name, scope, ext='.html'):

        return Util.quote(name + '/' + '/'.join(scope)) + ext

    def xref(self, page):

        return 'XRef/xref%d.html'%page

    def module_tree(self):

        return 'Modules.html'

    def module_index(self, scope):

        if not len(scope):
            return self._check_main('Modules/global.html')
        else: return Util.quote('Modules/' + '/'.join(scope)) + '.html'
