#!/usr/bin/env python
#
# $Id: setup.py,v 1.21 2003/12/30 03:47:37 stefan Exp $
#
# Setup script for synopsis
#
# Usage: python setup.py install
#

from distutils.core import setup
from distutils import sysconfig

from Synopsis.config import *

from Synopsis.dist.command.config import config
from Synopsis.dist.command.build_doc import build_doc
from Synopsis.dist.command.build_syn_ext import build_syn_ext

# patch distutils if it can't cope with the "classifiers" keyword
from distutils.dist import DistributionMetadata
if not hasattr(DistributionMetadata, 'classifiers'):
   DistributionMetadata.classifiers = None
   DistributionMetadata.download_url = None

import os, sys, re, glob

module_ext = sysconfig.get_config_var('SO')

def prefix(list, pref): return map(lambda x: pref + x, list)

py_packages = ["Synopsis",
               "Synopsis.Parsers",
               "Synopsis.Parsers.IDL", "Synopsis.Parsers.Python",
               "Synopsis.Parsers.C", "Synopsis.Parsers.Cxx",
               "Synopsis.Processors", "Synopsis.Processors.Comments",
               "Synopsis.Formatters",
               "Synopsis.Formatters.HTML",
               "Synopsis.Formatters.HTML.Views",
               "Synopsis.Formatters.HTML.Parts",
               "Synopsis.Formatters.HTML.Comments",
               "Synopsis.Formatters.HTML.Fragments"]

#"Synopsis.Core",
#"Synopsis.UI",
#"Synopsis.UI.Qt"] 


ext_modules = [('Synopsis/Parsers/Cxx', 'occ' + module_ext),
               ('Synopsis/Parsers/Cxx', 'link' + module_ext),
               ('Synopsis/Parsers/C', 'ctool' + module_ext)]

scripts = ['synopsis']

#, 'synopsis-qt', 'compile-xref', 'search-xref']

data_files = []

data_files.append(('share/Synopsis', glob.glob('share/Synopsis/*.*')))

#### add documentation

def add_documentation(all, directory, files):
   if directory[-3:] == 'CVS': return
   all.append((directory,
               filter(os.path.isfile,
                      map(lambda x:os.path.join(directory, x), files))))
documentation = []
os.path.walk('share/doc/Synopsis', add_documentation, documentation)

data_files.extend(documentation)

setup(cmdclass={'config':config,
                'build_doc':build_doc,
                'build_ext':build_syn_ext},
      name="synopsis",
      version=version,
      maintainer="Stefan Seefeld",
      maintainer_email="stefan@fresco.org",
      description="source code introspection tool",
      url="http://synopsis.fresco.org",
      download_url = 'http://synopsis.fresco.org/download',
      classifiers = ['Development Status :: 5 - Production/Stable',
                     'Environment :: Console',
                     'Environment :: Web Environment',
                     'Intended Audience :: Developers',
                     'License :: OSI Approved :: GNU Library or Lesser General Public License (LGPL)',
                     'Operating System :: POSIX',
                     'Programming Language :: Python',
                     'Programming Language :: C++',
                     'Topic :: Software Development :: Documentation'],
      packages=py_packages,
      ext_modules=ext_modules,
      scripts=prefix(scripts, "bin/"),
      data_files=data_files)
