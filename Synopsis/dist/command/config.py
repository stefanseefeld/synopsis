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
        ('libdir=', None,
         "library installation directory"),
        ('disable-threads', None,
         "whether or not to enable thread-support"),
        ('disable-gc', None,
         "whether or not to build the C++ parser with the garbage collector"),
        ('with-gc-prefix=', None,
         "the prefix to the garbage collector."),
        ('with-boost', None,
         'whether to use boost libraries in backends.'),
        ('with-boost-version=', None, 'the boost version.'),
        ('with-boost-prefix=', None, 'the prefix to the boost libraries.'),
        ('with-boost-lib-suffix=', None,
         'the library suffix to the used for boost libraries.')]
    boolean_options = build_ext.boolean_options[:] + ['disable-threads', 'disable-gc', 'with-boost']

    def initialize_options(self):

        self.prefix = None
        self.libdir = None
        build_ext.initialize_options(self)
        self.disable_threads = 0
        self.disable_gc = 0
        self.with_gc_prefix = ''
        self.with_boost = False
        self.with_boost_prefix = ''
        self.with_boost_version = ''
        self.with_boost_lib_suffix = ''

    def finalize_options(self):

        if not self.prefix:
            # default to default prefix used by the install command
            install = self.distribution.get_command_obj('install')
            install.ensure_finalized()
            self.prefix = install.prefix
        # set build_clib to build_lib if it was explicitely given,
        # overriding the default value.
        if self.build_temp:
            self.build_ctemp = self.build_temp
        else:
            self.build_ctemp = os.path.join('build', 'ctemp.' + get_platform())

        if self.build_lib:
            self.build_clib = self.build_lib
        else:
            self.build_clib = os.path.join('build', 'clib.' + get_platform())

        build_ext.finalize_options(self)
        # only append the path once 
        self.extensions = []
        for e in self.distribution.ext_modules:
            if e[0] not in self.extensions:
                self.extensions.append(e[0])
        if self.with_boost:
            self.extensions[0] = 'Synopsis/Parsers/Cpp/wave'

    def run(self):

        version = self.distribution.get_version()
        libdir_option = self.libdir and ' --libdir=%s'%self.libdir or ''
        self.config('src', self.build_ctemp, self.build_clib,
                    '--enable-version=%s%s'%(version, libdir_option))
        if not self.disable_gc and not self.with_gc_prefix:
            if os.name == 'nt':
                # for the gc configuration on the win32 native platform
                # set 'CC' explicitely to 'gcc -mno-cygwin'
                os.environ['CC'] = "gcc -mno-cygwin"
            self.config('src/Synopsis/gc', self.build_ctemp, self.build_clib)

        if os.name == 'nt':
            syn_cxx = '`cygpath -a "%s/src"`'%os.path.abspath(self.build_ctemp)
            #syn_cxx = syn_cxx.replace('\\', '\\\\\\\\\\\\\\\\')
        else:
            syn_cxx = '%s/src'%os.path.abspath(self.build_ctemp)    

        for ext in self.extensions:
            if not self.rpath:
                self.config(ext, self.build_temp, self.build_lib,
                            '--with-syn-cxx=%s'%syn_cxx)
            else:
                self.config(ext, self.build_temp, self.build_lib,
                            '--with-syn-cxx=%s LDFLAGS=-Wl,-rpath,%s'%(syn_cxx, self.rpath[0]))

        self.config('tests', self.build_temp, self.build_lib,
                    '--with-syn-cxx="%s"'%syn_cxx)
        self.config('doc', self.build_temp, self.build_lib)
        #self.config('sandbox', self.build_temp, self.build_lib,
        #            '--with-syn-cxx="%s"'%syn_cxx)

            
    def config(self, component, build_temp, build_lib, args=''):
        """Configure a component.
        Depending on whether it depends on the Python C API, it will
        be compiled into 'build_lib' or 'build_clib'."""

        self.announce("configuring '%s'" % component)
        path = os.path.join(build_temp, component)
        mkpath (path, int('777', 8), self.verbose, self.dry_run)
        srcdir = os.path.abspath(component)
        tempdir = os.path.abspath(os.path.join(build_temp, component))
        builddir = os.path.abspath(os.path.join(build_lib, component))

        cwd = os.getcwd()
        mkpath(tempdir, int('777', 8), self.verbose, self.dry_run)
        os.chdir(tempdir)
        
        if os.name == 'nt':
            # the path to the configure script has to be expressed in posix style
            # because even if we are compiling for windows, this part is run within
            # a cygwin shell
            configure = ('../' * (build_temp.count('\\') + component.count('/')  + 2)
                         + component + '/configure')
            python = '`cygpath -a "%s"`'%os.path.dirname(sys.executable)
            python += '/' + os.path.basename(sys.executable)
            #python.replace('\\', '\\\\\\\\\\\\\\\\')
            prefix = '`cygpath -a "%s"`'%sys.prefix
        
        else:
            configure = srcdir + '/configure'
            python = sys.executable
            prefix = self.prefix
        command = '%s --prefix="%s" --with-python="%s"'%(configure, prefix, python)
        if self.disable_threads:
            command += ' --disable-threads'
        if self.disable_gc:
            command += ' --disable-gc'
        elif self.with_gc_prefix:
            command += ' --with-gc-prefix="%s"'%self.with_gc_prefix
        if self.with_boost_version:
            command += ' --with-boost-version="%s"'%self.with_boost_version
        if self.with_boost_prefix:
            command += ' --with-boost-prefix="%s"'%self.with_boost_prefix
        if self.with_boost_lib_suffix:
            command += ' --with-boost-lib-suffix="%s"'%self.with_boost_lib_suffix
        command += ' %s'%args
        self.announce(command)
        # Work around a hack in distutils.spawn by an even more evil hack:
        # on NT, the whole command will get quoted, so we need to escape our own
        # quoting marks.
        if os.name == 'nt':
            command = command.replace('"', '\\"')
        spawn(['sh', '-c', command], self.verbose, self.dry_run)
        os.chdir(cwd)
