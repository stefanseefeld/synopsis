#
# Copyright (C) 2005 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from distutils.core import Command
from distutils.errors import DistutilsOptionError
from distutils.dir_util import mkpath
from distutils.file_util import copy_file
from distutils.util import get_platform
from distutils.spawn import spawn
from distutils.util import change_root
from distutils import sysconfig
import os
from Synopsis.dist.command import shared_library_outputs, copy_shared_library

def header_collector(prefix):

    def collect_headers(arg, path, files):
        """Collect headers to be installed."""

        # For now at least the following are not part of the public API
        if os.path.split(path)[1] in ['Support', 'Python', 'ASG']:
            return
        arg.extend([(os.path.join(path, f), os.path.join(path, f)[len(prefix) + 1:])
                    for f in files
                    if f.endswith('.hh') or f.endswith('.h')])

    return collect_headers


class install_clib(Command):

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

        version = self.distribution.get_version()
        build_ctemp = os.path.join('build', 'ctemp.' + get_platform())
        if os.name == 'nt':
            # same as in config.py here: even on 'nt' we have to
            # use posix paths because we run in a cygwin shell at this point
            path = build_ctemp.replace('\\', '/') + '/src'
            LIBEXT = '.dll'
        else:
            path = os.path.join(build_ctemp, 'src')
            LIBEXT = sysconfig.get_config_var('SO')
        target = 'libSynopsis%s'%LIBEXT
        if not os.path.isfile(os.path.join(self.build_dir, 'lib', target)):
            self.run_command('build_clib')

        make = os.environ.get('MAKE', 'make')
        command = '%s -C "%s" %s DESTDIR=%s'%(make, path, 'install', self.root)
        spawn(['sh', '-c', command], self.verbose, self.dry_run)
            

    def get_outputs(self):

        if self.root:
            prefix = change_root(self.root, self.prefix)
        else:
            prefix = self.prefix

        version = self.distribution.get_version()
        libdir = os.path.join(prefix, 'lib')
        libs = shared_library_outputs('Synopsis', version, libdir)
        pkgconfig = os.path.join(prefix, 'lib', 'pkgconfig', 'synopsis.pc')
        headers = []
        os.path.walk(os.path.join('src', 'Synopsis'),
                     header_collector('src'),
                     headers)
        os.path.walk(os.path.join(self.build_dir, 'include', 'Synopsis'),
                     header_collector(os.path.join(self.build_dir, 'include')),
                     headers)
        include_dir = os.path.join(prefix, 'include')
        headers = [os.path.join(include_dir, dest) for (src, dest) in headers]
        return libs + [pkgconfig] + headers
        

