#
# Copyright (C) 2004 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

import os, sys, string

from distutils.command.build_ext import build_ext
from distutils.util import get_platform
from distutils.dir_util import mkpath
from distutils.spawn import spawn, find_executable
from shutil import *

class config(build_ext):
    """derive from build since we use almost all the same options"""

    description = "configure the package"

    user_options = build_ext.user_options[:] + [
        ('disable-gc', None,
         "whether or not to build the C++ parser with the garbage collector"),
        ('with-gc-prefix=', None,
         "the prefix to the garbage collector.")]
    boolean_options = build_ext.boolean_options[:] + ['disable-gc']

    def initialize_options (self):
        build_ext.initialize_options(self)
        self.disable_gc = 0
        self.with_gc_prefix = ''

    def finalize_options (self):
        build_ext.finalize_options(self)
        # only append the path once 
        self.extensions = []
        for e in self.distribution.ext_modules:
            if e[0] not in self.extensions:
                self.extensions.append(e[0])

    def run(self):

        self.config_extensions()

    def config_extensions(self):

        self.config_extension('src')
        if not self.disable_gc:
            if os.name == 'nt':
                # for the gc configuration on the win32 native platform
                # set 'CC' explicitely to 'gcc -mno-cygwin'
                os.environ['CC'] = "gcc -mno-cygwin"
            self.config_extension('src/Synopsis/gc')
        self.config_extension('Cxx-API')
        for ext in self.extensions:
            self.config_extension(ext)
        # not really an extension, but as far as configuration is concerned,
        # treated equally.
        self.config_extension('tests')
        self.config_extension('doc')
        self.config_extension('contrib')
            
    def config_extension(self, ext):

        self.announce("configuring '%s'" % ext)
        path = os.path.join(self.build_temp, ext)
        mkpath (path, 0777, self.verbose, self.dry_run)
        srcdir = os.path.abspath(ext)
        tempdir = os.path.abspath(os.path.join(self.build_temp, ext))
        builddir = os.path.abspath(os.path.join(self.build_lib, ext))

        cwd = os.getcwd()
        mkpath(tempdir, 0777, self.verbose, self.dry_run)
        os.chdir(tempdir)
        
        if os.name == 'nt':
            # the path to the configure script has to be expressed in posix style
            # because even if we are compiling for windows, this part is run within
            # a cygwin shell
            configure = ('../' * (self.build_temp.count('\\') + ext.count('/')  + 2)
                         + ext + '/configure')
            python = string.replace('`cygpath -a %s`'%os.path.dirname(sys.executable)
                                    + '/' + os.path.basename(sys.executable),
                                    '\\', '\\\\')
        else:
            configure = srcdir + '/configure'
            python = sys.executable

        command = "%s --with-python=%s"%(configure, python)
        self.announce(command)
        spawn(['sh', '-c', command], self.verbose, self.dry_run)
        os.chdir(cwd)
