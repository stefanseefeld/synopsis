import os, sys, string

from distutils.core import setup
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

        for ext in self.extensions:
            self.config_extension(ext)
        if not self.disable_gc:
            self.config_extension('Synopsis/Parsers/Cxx/gc')
            
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
        if ext == 'Synopsis/Parsers/Cxx':
            if self.disable_gc:
                command += ' --disable-gc'
            elif self.with_gc_prefix:
                command += ' --with-gc-prefix=%s'%self.with_gc_prefix
        self.announce(command)
        spawn(['sh', '-c', command], self.verbose, self.dry_run)
        os.chdir(cwd)
