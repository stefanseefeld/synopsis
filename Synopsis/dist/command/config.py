import os, sys, string

from distutils.core import setup
from distutils.command import build, build_ext
from distutils.util import get_platform
from distutils.dir_util import mkpath
from distutils.spawn import spawn, find_executable
from shutil import *

class config(build.build):
    """derive from build since we use almost all the same options"""

    description = "configure the package"

    user_options = build.build.user_options[:] + [
        ('disable-gc', None,
         "whether or not to build the C++ parser with the garbage collector"),
        ('enable-c', None,
         "whether or not to build the C parser")
        ]
    boolean_options = build.build.boolean_options[:] + ['disable-gc', 'enable-c']

    def initialize_options (self):
        build.build.initialize_options(self)
        self.disable_gc = 0
        self.enable_c = 0

    def finalize_options (self):
        build.build.finalize_options(self)
        # only append the path once 
        self.extensions = []
        for e in self.distribution.ext_modules:
            if e[0] not in self.extensions:
                self.extensions.append(e[0])
        if not self.enable_c and 'Synopsis/Parser/C' in self.extensions:
            self.extensions.remove('Synopsis/Parser/C')

    def run(self):

        self.config_extensions()

    def config_extensions(self):

        for ext in self.extensions:
            self.config_extension(ext)
        if not self.disable_gc:
            self.config_extension('Synopsis/Parser/C++/gc')
            
    def config_extension(self, ext):

        self.announce("configuring '%s'" % ext)
        path = os.path.join(self.build_temp, ext)
        mkpath (path, 0777, self.verbose, self.dry_run)
        srcdir = os.path.abspath(ext)
        tempdir = os.path.abspath(os.path.join(self.build_temp, ext))
        builddir = os.path.abspath(os.path.join(self.build_lib, ext))
        configure = os.path.join(srcdir, 'configure')

        cwd = os.getcwd()
        mkpath(tempdir, 0777, self.verbose, self.dry_run)
        os.chdir(tempdir)

        command = "%s/configure --with-python=%s"%(srcdir, sys.executable)
        if ext == 'Synopsis/Parser/C++' and self.disable_gc:
            command += ' --disable-gc'
        self.announce(command)
        spawn(['sh', '-c', command], self.verbose, self.dry_run)
        os.chdir(cwd)
