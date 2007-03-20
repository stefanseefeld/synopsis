#
# Copyright (C) 2007 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

import os, re
from distutils import sysconfig
from distutils.dir_util import mkpath
from distutils.file_util import copy_file
from distutils.util import get_platform

def reset_config_variables(config_file, **vars):
    """Reset specific variables in the given config file to new values.

    'config_file' -- The config file to modify.

    'vars' -- dict object containing variables to reset, with their new values.

    """

    script = open(config_file, 'r').read()
    for v in vars:
        script, found = re.subn('%s[ \t]*=.*'%v,'%s = %s'%(v, repr(vars[v])), script)
        if not found: script += '%s = %s'%(v, repr(vars[v]))
    c = open(config_file, 'w')
    c.write(script)
    c.close()

def copy_shared_library(name, version, src, dest, verbose, dry_run):
    """Copies shared library and assorted links from src to dest.

    'name' -- The name of the library.
    'version' -- The version of the library.
    'src' -- The source directory.
    'dest' -- The destination directory.
    'verbose' -- Whether or not to operate verbosely.
    'dry_run' -- Whether or not to operate in dry-run mode."""

    major, minor = version.split('.', 1)
    if os.name == 'nt':
        LIBEXT = '.dll'
    elif os.uname()[0] == 'Darwin':
        LIBEXT = '.dylib'
    else:
        LIBEXT = sysconfig.get_config_var('SO')
    library = 'lib%s%s'%(name, LIBEXT)

    mkpath (dest, 0777, verbose, dry_run)
    if os.name == 'posix' and not os.uname()[0].startswith('CYGWIN'):
        # Copy versioned DSO
        copy_file(os.path.join(src, 'lib', '%s.%s'%(library, version)),
                  os.path.join(dest, '%s.%s'%(library, version)),
                  1, 1, 0, None, verbose, dry_run)
        for target in '%s.%s'%(library, major), library:
            src_file = os.path.join(src, 'lib', target)
            if (os.path.islink(src_file)) and not dry_run:
                dest_file = os.path.join(dest, target)
                if os.path.exists(dest_file):
                    os.remove(dest_file)
                linkto = os.readlink(src_file)
                os.symlink(linkto, dest_file)
            else:
                copy_file(os.path.join(src, 'lib', target),
                          os.path.join(dest, target),
                          1, 1, 0, None, verbose, dry_run)
    else:
        # Copy unversioned DSO
        copy_file(os.path.join(src, 'lib', library),
                  os.path.join(dest, library),
                  1, 1, 0, None, verbose, dry_run)

