#
# Copyright (C) 2009 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

import os, sys, string

from distutils.core import Command
from distutils.util import get_platform
from distutils.file_util import write_file, copy_file
from distutils.errors import *
from distutils.sysconfig import get_python_version

class bdist_rpm (Command):


    description = "create an RPM distribution"

    user_options = [
        ('bdist-base=', None,
         "base directory for creating built distributions"),
        ('rpm-base=', None,
         "base directory for creating RPMs (defaults to \"rpm\" under "
         "--bdist-base; must be specified for RPM 2)"),
        ('dist-dir=', 'd',
         "directory to put final RPM files in "
         "(and .spec files if --spec-only)"),
        ('python=', None,
         "path to Python interpreter to hard-code in the .spec file "
         "(default: \"python\")"),
        ('source-only', None,
         "only generate source RPM"),
        ('binary-only', None,
         "only generate binary RPM"),
        ('use-bzip2', None,
         "use bzip2 instead of gzip to create source distribution"),

        # Actions to take when building RPM
        ('keep-temp', 'k',
         "don't clean up RPM build directory"),
        ('no-keep-temp', None,
         "clean up RPM build directory [default]"),

        # Allow a packager to explicitly force an architecture
        ('force-arch=', None,
         "Force an architecture onto the RPM build process"),
       ]

    boolean_options = ['keep-temp']

    negative_opt = {'no-keep-temp': 'keep-temp'}


    def initialize_options(self):
        self.bdist_base = None
        self.rpm_base = None
        self.dist_dir = None
        self.binary_only = None
        self.source_only = None
        self.use_bzip2 = None

        self.keep_temp = 0

        self.force_arch = None

    # initialize_options()


    def finalize_options(self):
        self.set_undefined_options('bdist', ('bdist_base', 'bdist_base'))
        if self.rpm_base is None:
            self.rpm_base = os.path.join(self.bdist_base, "rpm")

        if os.name != 'posix':
            raise DistutilsPlatformError \
                  ("don't know how to create RPM "
                   "distributions on platform %s" % os.name)
        if self.binary_only and self.source_only:
            raise DistutilsOptionError \
                  ("cannot supply both '--source-only' and '--binary-only'")

        self.set_undefined_options('bdist', ('dist_dir', 'dist_dir'))

    def run(self):

        rpm_dir = {}
        for d in ('SOURCES', 'SPECS', 'BUILD', 'RPMS', 'SRPMS'):
            rpm_dir[d] = os.path.join(self.rpm_base, d)
            self.mkpath(rpm_dir[d])
        spec_dir = rpm_dir['SPECS']

        copy_file('synopsis.spec', '%s/synopsis.spec'%spec_dir)

        # Make a source distribution and copy to SOURCES directory
        saved_dist_files = self.distribution.dist_files[:]
        sdist = self.reinitialize_command('sdist')
        if self.use_bzip2:
            sdist.formats = ['bztar']
        else:
            sdist.formats = ['gztar']
        self.run_command('sdist')
        self.distribution.dist_files = saved_dist_files

        source = sdist.get_archive_files()[0]
        source_dir = rpm_dir['SOURCES']
        self.copy_file(source, source_dir)

        # build package
        self.announce("building RPMs")
        rpm_cmd = ['rpmbuild']
        if self.source_only: # what kind of RPMs?
            rpm_cmd.append('-bs')
        elif self.binary_only:
            rpm_cmd.append('-bb')
        else:
            rpm_cmd.append('-ba')

        rpm_cmd.extend(['--define',
                        '_topdir %s' % os.path.abspath(self.rpm_base)])
        if not self.keep_temp:
            rpm_cmd.append('--clean')
        rpm_cmd.append('%s/synopsis.spec'%spec_dir)

        self.spawn(rpm_cmd)
