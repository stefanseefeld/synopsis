#!/usr/bin/env python
#
# $Id: setup.py,v 1.8 2003/11/05 01:56:59 stefan Exp $
#
# Setup script for synopsis
#
# Usage: python setup.py install
#

from distutils.core import setup
from distutils import sysconfig

import os, sys, re

from Synopsis.dist.command.config import config
from Synopsis.dist.command.build_doc import build_doc
from Synopsis.dist.command.build import build
from Synopsis.dist.command.build_ext import build_ext

# patch distutils if it can't cope with the "classifiers" keyword
from distutils.dist import DistributionMetadata
if not hasattr(DistributionMetadata, 'classifiers'):
    DistributionMetadata.classifiers = None
    DistributionMetadata.download_url = None

module_ext = sysconfig.get_config_var('SO')

def prefix(list, pref): return map(lambda x, p=pref: p + x, list)

py_packages = ["Synopsis",
               "Synopsis.Core",
               "Synopsis.Parser",
               "Synopsis.Parser.IDL", "Synopsis.Parser.Python",
               "Synopsis.Parser.C", "Synopsis.Parser.Cxx",
               "Synopsis.Linker",
               "Synopsis.Formatter",
               "Synopsis.Formatter.HTML",
               "Synopsis.UI",
               "Synopsis.UI.Qt"] 

ext_modules = [('Synopsis/Parser/C', 'ctool' + module_ext),
               ('Synopsis/Parser/Cxx', 'occ' + module_ext),
               ('Synopsis/Parser/Cxx', 'link' + module_ext),
               ]

scripts = ['synopsis', 'synopsis-qt', 'compile-xref', 'search-xref']

data_files = ["synopsis.jpg", "synopsis200.jpg", "syn-down.png", "syn-right.png", "syn-dot.png"]
from Synopsis import __version__

setup(cmdclass={'config':config,
                'build_doc':build_doc,
                'build_ext':build_ext,
                'build':build},
      name="synopsis",
      version=__version__,
      author="Stefan Seefeld & Stephen Davies",
      author_email="stefan@fresco.org",
      description="source code inspection tool",
      url="http://synopsis.fresco.org",
      download_url = 'http://synopsis.fresco.org/download',
      classifiers = ['Development Status :: 5 - Production/Stable',
                     'Environment :: Console',
                     'Environment :: Web Environment',
                     'Intended Audience :: Developers',
                     'License :: OSI Approved :: GNU General Public License (GPL)',
                     'Operating System :: POSIX',
                     'Programming Language :: Python',
                     'Programming Language :: C++',
                     'Topic :: Software Development :: Documentation'],
      packages=py_packages,
      ext_modules=ext_modules,
      scripts=prefix(scripts, "bin/"),
      data_files=[('share/Synopsis', prefix(data_files, "share/"))])
