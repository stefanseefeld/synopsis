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

    extensions = ['Synopsis/Parser/C', 'Synopsis/Parser/C++']

    def run(self):

        self.config_extensions()

    def config_extensions(self):

        for ext in config.extensions:
            self.config_extension(ext)

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
        spawn(['sh', '-c', command], self.verbose, self.dry_run)
        os.chdir(cwd)
