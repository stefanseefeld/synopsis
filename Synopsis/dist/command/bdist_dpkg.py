"""distutils.command.bdist_dpkg

Implements the Distutils 'bdist_dpkg' command (create a Debian package).
"""

# This module should be kept compatible with Python 1.5.2.

__revision__ = "$Id: bdist_dpkg.py,v 1.7 2004/06/16 14:56:27 akuchling Exp $"

import sys, os, string
import glob
from types import *
from distutils.core import Command
from distutils.debug import DEBUG
from distutils.util import get_platform, convert_path
from distutils.file_util import write_file, copy_file
from distutils.dir_util import mkpath
from distutils.errors import *
from distutils import log

import time

MARKER_STRING = "GENERATED BY BDIST_DPKG"

class bdist_dpkg (Command):

    description = "create a DPKG distribution"

    user_options = [
        ('gain-root-command=', 'r', 'command that allows to gain root rights'),
        # XXX this option name is a bit odd; what would be a better one?
        ('do-not-build', None, 'do not build the package, just create the debian/ stuff')
       ]

    boolean_options = [ 'do-not-build' ]

    def initialize_options (self):
        self.gain_root_command = None
        self.do_not_build = None

    # initialize_options()


    def finalize_options (self):
        if os.name != 'posix':
            raise DistutilsPlatformError, \
                  ("don't know how to create DPKG "
                   "distributions on platform %s" % os.name)

    # finalize_options()

    def run (self):

        if DEBUG:
            print "before _get_package_data():"

        self._create_dpkg_files()

        if not self.do_not_build:
            # build package
            log.info("building DPKG")

            build_command = [ "dpkg-buildpackage" ]

            if self.gain_root_command is not None:
                build_command.append ('-r%s' % self.gain_root_command)

            self.spawn(build_command)

    # run()

    def _create_dpkg_files (self):
        # Files to create:
        # changelog, control, 
        # Optional: docs, copyright
        #
        log.info("creating debian control files")
        if not os.path.exists('debian'):
            log.info("creating debian/ subdirectory")
            os.mkdir('debian')

        dist = self.distribution
        dirlist = ""
        for i in dist.packages or []:
            dir = i
            # XXX ugh! this replacement really needs to
            # be available in the Distutils API somewhere, but it's not.
            dirlist += ' ' + dir.replace('.', '/')

        pyversion = '%i.%i' % sys.version_info[0:2]
        # Temporary hack to build everything with 2.2 --
        # I'm running out of the CVS trunk, but don't have it
        # installed as python2.4.
        package_name = 'python%s-%s' % (pyversion, dist.get_name().lower())
        d = {'name':dist.get_name(),
             'package_name':package_name, 
             'pyversion': pyversion,
             'dirlist':dirlist,
             'license':dist.get_license(),
             'marker':MARKER_STRING,
             'maintainer':dist.get_maintainer(),
             'maintainer_email':dist.get_maintainer_email(),
             'now':time.strftime ("%a, %d %b %Y %H:%M:%S %z"),
             'author':dist.get_author(),
             'author_email':dist.get_author_email(),
             'url':dist.get_url()
             }
        if not self._is_user_file('changelog') or True:
            log.info('writing changelog file')
            output = self._write_file('changelog')
            output.write("""%s (%s-%i) unstable; urgency=low

  * Dummy changelog line

 -- %s <%s>  %s
""" % (package_name, dist.get_version(), 1, # XXX build version?
       dist.get_maintainer(), dist.get_maintainer_email(), d['now']))
            output.close()

        if not self._is_user_file('control') or True:
            log.info('writing control file')
            output = self._write_file('control')
            #output.write('XXX-Dummy-Field: %s\n' % MARKER_STRING)
            output.write("Source: %s\n" % package_name)
            output.write("Priority: optional\n")
            output.write("Maintainer: %s <%s>\n"
                         % (dist.get_maintainer(), dist.get_maintainer_email()))
            output.write("""Build-Depends: debhelper (>> 3.0.0)
Standards-Version: 3.5.8
Section: libs
""")
            output.write('\n')          # Separator line
            
            output.write("Package: %s\n" % package_name)

            #added missing Version information to control --- Vinzenz Feenstra (evilissimo@users.sf.net)
	    output.write("Version: %s\n" % dist.get_version() )
            output.write("""Section: libs
Architecture: any\n""")
            output.write("Depends: python%(pyversion)s\n" % d)
            output.write("Description: %s\n" % dist.get_description())
            s = dist.get_long_description()
            s = string.replace(s, '\n', '\n  ')
            output.write('  ' + s)
            output.close()
            
        if dist.has_pure_modules() and not self._is_user_file('postinst'):
            log.info('writing postinst file')
            output = self._write_file('postinst')
            output.write(POSTINST_FILE % d)
            output.close()
        if dist.has_pure_modules() and not self._is_user_file('prerm'):
            log.info('writing prerm file')
            output = self._write_file('prerm')
            output.write(PRERM_FILE % d)
            output.close()
            
        if not self._is_user_file('rules'):
            log.info('writing rules file')
            output = self._write_file('rules')
            output.write(RULES_FILE % d)
            output.close()
            os.chmod('debian/rules', 0755)

        if not self._is_user_file('copyright'):
            log.info('writing copyright file')
            output = self._write_file('copyright')
            output.write(COPYRIGHT_FILE % d)
            output.close()
        mkpath('debian/tmp/DEBIAN', 0755)
        copy_file('debian/control', 'debian/tmp/DEBIAN/control')

    def _write_file (self, filename):
        path = os.path.join('debian', filename)
        assert not self._is_user_file(path)
        output = open(path, 'w')
        return output
    
    def _is_user_file (self, filename):
        path = os.path.join('debian', filename)
        if not os.path.exists(path):
            return False
        input = open(path, 'r')
        while 1:
            L = input.readline()
            if L == "": break
            elif L.find(MARKER_STRING) != -1:
                return False

        return True
    
# class bdist_dpkg

PRERM_FILE = """#! /bin/sh 
# %(marker)s  -- remove this line if you edit this file
# prerm script for %(name)s

set -e

PACKAGE=%(name)s
VERSION=%(pyversion)s
LIB=/usr/lib/python$VERSION
DIRLIST="%(dirlist)s"

case "$1" in
    remove|upgrade|failed-upgrade)
        for i in $DIRLIST ; do
            find $LIB/site-packages/$i -name '*.py[co]' -exec rm \{\} \;
        done
    ;;

    *)
        echo "prerm called with unknown argument \`$1'" >&2
        exit 1
    ;;
esac

exit 0
"""

POSTINST_FILE = """#! /bin/sh 
# %(marker)s  -- remove this line if you edit this file
# postinst script for %(name)s
#
# see: dh_installdeb(1)

set -e

# summary of how this script can be called:
#        * <postinst> `configure' <most-recently-configured-version>
#        * <old-postinst> `abort-upgrade' <new version>
#        * <conflictor's-postinst> `abort-remove' `in-favour' <package>
#          <new-version>
#        * <deconfigured's-postinst> `abort-deconfigure' `in-favour'
#          <failed-install-package> <version> `removing'
#          <conflicting-package> <version>
# for details, see http://www.debian.org/doc/debian-policy/ or
# the debian-policy package
#
# quoting from the policy:
#     Any necessary prompting should almost always be confined to the
#     post-installation script, and should be protected with a conditional
#     so that unnecessary prompting doesn't happen if a package's
#     installation fails and the `postinst' is called with `abort-upgrade',
#     `abort-remove' or `abort-deconfigure'.

PACKAGE=%(name)s
VERSION=%(pyversion)s
LIB=/usr/lib/python$VERSION
DIRLIST="%(dirlist)s"

case "$1" in
    configure|abort-upgrade|abort-remove|abort-deconfigure)
        for i in $DIRLIST ; do
            /usr/bin/python$VERSION -O $LIB/compileall.py -q $LIB/site-packages/$i
            /usr/bin/python$VERSION $LIB/compileall.py -q $LIB/site-packages/$i
        done
    ;;

    *)
        echo "postinst called with unknown argument \`$1'" >&2
        exit 1
    ;;
esac

exit 0
"""

RULES_FILE = """#!/usr/bin/make -f
# %(marker)s  -- remove this line if you edit this file
# Sample debian/rules that uses debhelper.
# GNU copyright 1997 to 1999 by Joey Hess.

# Uncomment this to turn on verbose mode.
#export DH_VERBOSE=1

# This is the debhelper compatibility version to use.
export DH_COMPAT=4

build: build-stamp
	/usr/bin/python%(pyversion)s setup.py build
build-stamp: 
	touch build-stamp

configure:
	# Do nothing

clean:
	dh_testdir
	dh_testroot
	rm -f build-stamp

	-rm -rf build

	dh_clean

install: build
	dh_testdir
	dh_testroot
	dh_clean -k
	/usr/bin/python%(pyversion)s setup.py install --no-compile --prefix=$(CURDIR)/debian/%(package_name)s/usr

# Build architecture-independent files here.
binary-indep: install
	dh_testdir
	dh_testroot

	dh_installdocs
	dh_installdeb
	dh_gencontrol
	dh_md5sums
	dh_builddeb
# We have nothing to do by default.

binary: binary-indep 
.PHONY: build clean binary-indep binary install
"""

COPYRIGHT_FILE = '''This package was debianized by
        %(maintainer)s <%(maintainer_email)s> on %(now)s

It was downloaded from

  %(url)s

Upstream Author: %(author)s <%(author_email)s>

Copyright:

%(license)s
'''
