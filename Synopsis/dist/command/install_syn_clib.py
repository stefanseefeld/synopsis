import sys, os, string
from distutils.core import Command
from distutils.errors import DistutilsOptionError
from distutils.dir_util import mkpath
from distutils.file_util import copy_file

class install_syn_clib(Command):

    description = "install libSynopsis.so"

    user_options = [('prefix=', None, "installation prefix"),
                    ('home=', None, "(Unix only) home directory to install under")]

    def initialize_options(self):
        self.prefix = None
        self.build_dir = None
        self.home = None

    def finalize_options(self):

        self.set_undefined_options('install',
                                   ('prefix', 'prefix'),
                                   ('home', 'home'),
                                   ('build_lib', 'build_dir'))

    def run(self):

        libfile = 'libSynopsis.so'
        if not os.path.isfile(os.path.join(self.build_dir, 'lib', libfile)):
            self.run_command('build_clib')

        libdir = os.path.join(self.prefix, 'lib')
        mkpath (libdir, 0777, self.verbose, self.dry_run)
        copy_file(os.path.join(self.build_dir, 'lib', libfile),
                  os.path.join(libdir, libfile),
                  1, 1, 0, None, self.verbose, self.dry_run)


        def add_headers(all, directory, files):

            headers = filter(lambda name:name.endswith('.hh'),
                             map(lambda x:os.path.join(directory, x), files))
            all.extend(headers)

        headers = []
        os.path.walk(os.path.join('src', 'Synopsis'), add_headers, headers)

        incdir = os.path.join(self.prefix, 'include')
        for header in headers:
            target = os.path.join(incdir, header[4:])
            mkpath (os.path.dirname(target), 0777, self.verbose, self.dry_run)
            copy_file(header, target,
                      1, 1, 0, None, self.verbose, self.dry_run)
            
