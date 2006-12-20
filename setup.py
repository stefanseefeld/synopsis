#!/usr/bin/env python
#
# Copyright (C) 2004 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
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
from Synopsis.dist.command.build_syn_clib import build_syn_clib
from Synopsis.dist.command.build_syn_ext import build_syn_ext
from Synopsis.dist.command.test import test
from Synopsis.dist.command.install_syn_clib import install_syn_clib
from Synopsis.dist.command.install_syn import install_syn
from Synopsis.dist.command.bdist_dpkg import bdist_dpkg
from Synopsis.dist.command.clean_syn import clean_syn

# patch distutils if it can't cope with the "classifiers" keyword
from distutils.dist import DistributionMetadata
if not hasattr(DistributionMetadata, 'classifiers'):
   DistributionMetadata.classifiers = None
   DistributionMetadata.download_url = None

import os, sys, re, glob

module_ext = sysconfig.get_config_var('SO')

def prefix(list, pref): return [pref + x for x in list]
py_packages = ["Synopsis",
               "Synopsis.Parsers",
               "Synopsis.Parsers.IDL", "Synopsis.Parsers.Python",
               "Synopsis.Parsers.Cpp",
               "Synopsis.Parsers.C", "Synopsis.Parsers.Cxx",
               "Synopsis.Processors", "Synopsis.Processors.Comments",
               "Synopsis.Formatters",
               "Synopsis.Formatters.HTML",
               "Synopsis.Formatters.HTML.Views",
               "Synopsis.Formatters.HTML.Parts",
               "Synopsis.Formatters.HTML.Markup",
               "Synopsis.Formatters.HTML.Fragments"]

ext_modules = [('Synopsis/Parsers/Cpp/ucpp', 'ucpp' + module_ext),
               ('Synopsis/Parsers/IDL', '_omniidl' + module_ext),
               ('Synopsis/Parsers/C', 'ParserImpl' + module_ext),
               ('Synopsis/Parsers/Cxx', 'occ' + module_ext),
               ('Synopsis/Parsers/Cxx', 'link' + module_ext)]

scripts = ['synopsis', 'sxr-server']

data_files = [('share/doc/Synopsis', ('README', 'COPYING', 'NEWS'))]
data_files.append(('share/man/man1', glob.glob('share/man/man1/*.*')))
data_files.append(('share/Synopsis', glob.glob('share/Synopsis/*.*')))

#### add documentation

def add_documentation(all, directory, files):

   if '.svn' in files: files.remove('.svn')
   all.append((directory,
               [os.path.join(directory, file)
                for file in files
                if os.path.isfile(os.path.join(directory, file))]))

documentation = []
os.path.walk('share/doc/Synopsis', add_documentation, documentation)
data_files.extend(documentation)

setup(cmdclass={'config':config,
                'build_doc':build_doc,
                'build_clib':build_syn_clib,
                'build_ext':build_syn_ext,
                'test':test,
                'install_clib':install_syn_clib,
                'install':install_syn,
                'bdist_dpkg':bdist_dpkg,
                'clean':clean_syn},
      name="synopsis",
      version=version,
      author="Stefan Seefeld",
      maintainer="Stefan Seefeld",
      author_email="stefan@fresco.org",
      maintainer_email="stefan@fresco.org",
      description="Source-code Introspection Tool",
      long_description="""Synopsis is a multi-language source code introspection tool that
provides a variety of representations for the parsed code, to
enable further processing such as documentation extraction,
reverse engineering, and source-to-source translation.""",
      url="http://synopsis.fresco.org",
      download_url = 'http://synopsis.fresco.org/download',
      license = 'LGPL / GPL',
      classifiers = ['Development Status :: 5 - Production/Stable',
                     'Environment :: Console',
                     'Environment :: Web Environment',
                     'Intended Audience :: Developers',
                     'License :: OSI Approved :: GNU Library or Lesser General Public License (LGPL)',
                     'License :: OSI Approved :: GNU General Public License (GPL)',
                     'Operating System :: POSIX',
                     'Programming Language :: Python',
                     'Programming Language :: C++',
                     'Topic :: Software Development :: Documentation'],
      packages=py_packages,
      ext_modules=ext_modules,
      scripts=prefix(scripts, "scripts/"),
      data_files=data_files)
