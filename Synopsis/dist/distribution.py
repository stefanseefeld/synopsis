#
# Copyright (C) 2007 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from distutils import dist
from Synopsis.dist.command.config import config
from Synopsis.dist.command.build_doc import build_doc
from Synopsis.dist.command.build_clib import build_clib
from Synopsis.dist.command.build_ext import build_ext
from Synopsis.dist.command.build_py import build_py
from Synopsis.dist.command.test import test
from Synopsis.dist.command.install_clib import install_clib
from Synopsis.dist.command.install_lib import install_lib
from Synopsis.dist.command.install import install
from Synopsis.dist.command.bdist_dpkg import bdist_dpkg
from Synopsis.dist.command.clean import clean

# patch distutils if it can't cope with the "classifiers" keyword
from distutils.dist import DistributionMetadata
if not hasattr(DistributionMetadata, 'classifiers'):
    DistributionMetadata.classifiers = None
    DistributionMetadata.download_url = None

class Distribution(dist.Distribution):
    """Add additional parameters and commands."""

    def __init__(self, attrs=None):

        self.revision = None
        dist.Distribution.__init__(self, attrs)

        self.cmdclass['config'] = config
        self.cmdclass['build_doc'] = build_doc
        self.cmdclass['build_clib'] = build_clib
        self.cmdclass['build_ext'] = build_ext
        self.cmdclass['build_py'] = build_py
        self.cmdclass['test'] = test
        self.cmdclass['install_clib'] = install_clib
        self.cmdclass['install_lib'] = install_lib
        self.cmdclass['install'] = install
        self.cmdclass['bdist_dpkg'] = bdist_dpkg
        self.cmdclass['clean'] = clean
