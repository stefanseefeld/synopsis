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

    user_options = [o for o in build_ext.user_options
                    if o[0] in ['inplace', 'build-lib=', 'build-temp=']] + [
        ('prefix=', None,
         "installation prefix"),
        ('with-boost-version=', None,
         "the boost version."),
        ('with-boost-prefix=', None,
         "the prefix to the boost libraries."),
        ('with-boost-lib-suffix=', None,
         "the boost library suffix."),
        ('with-llvm-prefix=', None,
         "the prefix to the llvm libraries.")]
    boolean_options = build_ext.boolean_options[:] + ['disable-gc']

    def initialize_options(self):

        self.prefix = None
        build_ext.initialize_options(self)
        self.with_gc_prefix = ''
        self.with_boost_version = ''
        self.with_boost_prefix = ''
        self.with_boost_lib_suffix = ''
        self.with_llvm_prefix = ''

    def finalize_options(self):

        if not self.prefix:
            # default to default prefix used by the install command
            install = self.distribution.get_command_obj('install')
            install.ensure_finalized()
            self.prefix = install.prefix
        build_ext.finalize_options(self)
        # only append the path once 
        self.extensions = []
        for e in self.distribution.ext_modules:
            if e[0] not in self.extensions:
                self.extensions.append(e[0])

    def run(self):

        version = self.distribution.get_version()

        for ext in self.extensions:
            self.config(ext, self.build_temp, self.build_lib)

        #self.config('tests', self.build_temp, self.build_lib)
        self.config('doc', self.build_temp, self.build_lib)
        #self.config('sandbox', self.build_temp, self.build_lib)

            
    def config(self, component, build_temp, build_lib, args=''):
        """Configure a component.
        Depending on whether it depends on the Python C API, it will
        be compiled into 'build_lib' or 'build_clib'."""

        self.announce("configuring '%s'" % component)
        path = os.path.join(build_temp, component)
        mkpath (path, 0777, self.verbose, self.dry_run)
        srcdir = os.path.abspath(component)
        tempdir = os.path.abspath(os.path.join(build_temp, component))
        builddir = os.path.abspath(os.path.join(build_lib, component))

        cwd = os.getcwd()
        mkpath(tempdir, 0777, self.verbose, self.dry_run)
        os.chdir(tempdir)
        
        if os.name == 'nt':
            # the path to the configure script has to be expressed in posix style
            # because even if we are compiling for windows, this part is run within
            # a cygwin shell
            configure = ('../' * (build_temp.count('\\') + component.count('/')  + 2)
                         + component + '/configure')
            python = '`cygpath -a "%s"`'%os.path.dirname(sys.executable)
            python += '/' + os.path.basename(sys.executable)
            prefix = '`cygpath -a "%s"`'%sys.prefix
        
        else:
            configure = srcdir + '/configure'
            python = sys.executable
            prefix = self.prefix
        command = '%s --prefix="%s" --with-python="%s"'%(configure, prefix, python)
        if self.with_boost_version:
            command += ' --with-boost-version="%s"'%self.with_boost_version
        if self.with_boost_prefix:
            command += ' --with-boost-prefix="%s"'%self.with_boost_prefix
        if self.with_boost_lib_suffix:
            command += ' --with-boost-lib-suffix="%s"'%self.with_boost_lib_suffix
        if self.with_llvm_prefix:
            command += ' --with-llvm-prefix="%s"'%self.with_llvm_prefix
        command += ' %s'%args
        self.announce(command)
        # Work around a hack in distutils.spawn by an even more evil hack:
        # on NT, the whole command will get quoted, so we need to escape our own
        # quoting marks.
        if os.name == 'nt':
            command = command.replace('"', '\\"')
        spawn(['sh', '-c', command], self.verbose, self.dry_run)
        os.chdir(cwd)
