import sys, os, string
from distutils.core import Command
from distutils.errors import DistutilsOptionError
from distutils.dir_util import mkpath
from distutils.file_util import copy_file
from distutils.util import change_root
from distutils import sysconfig

def header_collector(prefix):

    def collect_headers(arg, path, files):
        """Collect headers to be installed."""

        # For now at least the following are not part of the public API
        if os.path.split(path)[1] in ['Support', 'Python', 'AST']:
            return
        arg.extend([(os.path.join(path, f), os.path.join(path, f)[len(prefix) + 1:])
                    for f in files
                    if f.endswith('.hh') or f.endswith('.h')])

    return collect_headers


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
                                   ('home', 'home'))
        self.set_undefined_options('build_clib',
                                   ('build_clib', 'build_dir'))

        # the bdist commands set a 'root' attribute (equivalent to
        # make's DESTDIR), which we have to merge in, if it exists.
        install = self.distribution.get_command_obj('install')
        install.ensure_finalized()

        self.root = getattr(install, 'root')

    def run(self):

        if os.name == 'nt':
            LIBEXT = '.dll'
        else:
            LIBEXT = sysconfig.get_config_var('SO')
        target = 'libSynopsis%s'%LIBEXT
        if not os.path.isfile(os.path.join(self.build_dir, 'lib', target)):
            self.run_command('build_clib')
            
        if self.root:
            prefix = change_root(self.root, self.prefix)
        else:
            prefix = self.prefix

        libdir = os.path.join(prefix, 'lib')
        mkpath (libdir, 0777, self.verbose, self.dry_run)
        copy_file(os.path.join(self.build_dir, 'lib', target),
                  os.path.join(libdir, target),
                  1, 1, 0, None, self.verbose, self.dry_run)

        headers = []
        os.path.walk(os.path.join('src', 'Synopsis'),
                     header_collector('src'),
                     headers)
        os.path.walk(os.path.join(self.build_dir, 'include', 'Synopsis'),
                     header_collector(os.path.join(self.build_dir, 'include')),
                     headers)
        incdir = os.path.join(prefix, 'include')
        for src, dest in headers:
            target = os.path.join(incdir, dest)
            mkpath (os.path.dirname(target), 0777, self.verbose, self.dry_run)
            copy_file(src, target,
                      1, 1, 0, None, self.verbose, self.dry_run)
            
        pkgdir = os.path.join(libdir, 'pkgconfig')
        mkpath (pkgdir, 0777, self.verbose, self.dry_run)
        copy_file(os.path.join(self.build_dir, 'Synopsis.pc'),
                  os.path.join(pkgdir, 'Synopsis.pc'),
                  1, 1, 0, None, self.verbose, self.dry_run)

    def get_outputs(self):

        if self.root:
            prefix = change_root(self.root, self.prefix)
        else:
            prefix = self.prefix

        if os.name == 'nt':
            LIBEXT = '.dll'
        else:
            LIBEXT = sysconfig.get_config_var('SO')
        library = os.path.join(prefix, 'lib', 'libSynopsis%s'%LIBEXT)
        pkgconf = os.path.join(prefix, 'lib', 'pkgconfig', 'Synopsis.pc')
        headers = []
        os.path.walk(os.path.join('src', 'Synopsis'),
                     header_collector('src'),
                     headers)
        os.path.walk(os.path.join(self.build_dir, 'include', 'Synopsis'),
                     header_collector(os.path.join(self.build_dir, 'include')),
                     headers)
        include_dir = os.path.join(prefix, 'include')
        headers = [os.path.join(include_dir, dest) for (src, dest) in headers]
        return [library, pkgconf] + headers
        

