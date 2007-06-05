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

from Synopsis.dist.distribution import Distribution
from distutils.core import setup
from distutils import sysconfig
import os, sys, re, glob, shutil

module_ext = sysconfig.get_config_var('SO')

def prefix(list, pref): return [pref + x for x in list]

version = '0.9.1'
revision = open('revision').read()[:-1]

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
if sys.platform == "win32":
    for script in scripts:
        filename = os.path.join('scripts', script)
        shutil.copyfile(filename, filename + '.py')
    scripts = [s + '.py' for s in scripts]

data_files = [('share/doc/synopsis-%s'%version, ('README', 'COPYING', 'NEWS'))]
data_files.append(('share/man/man1', glob.glob('share/man/man1/*.*')))
data_files.append(('share/synopsis-%s'%version, glob.glob('share/synopsis/*.*')))

#### add documentation

def add_documentation(all, directory, files):

    if '.svn' in files: files.remove('.svn')
    dest = directory.replace('share/doc/synopsis',
                             'share/doc/synopsis-%s'%version)
    all.append((dest,
                [os.path.join(directory, file)
                 for file in files
                 if os.path.isfile(os.path.join(directory, file))]))

documentation = []
os.path.walk('share/doc/synopsis', add_documentation, documentation)
data_files.extend(documentation)

setup(distclass=Distribution,
      name="synopsis",
      version=version,
      revision=revision,
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
