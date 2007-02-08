#
# Copyright (C) 2007 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

"""This module contains configuration data that are specific to a
particular release / installation such as version, data
paths, etc."""

import os, os.path

# The following variables are adjusted during installation.
version = 'devel'
prefix = os.path.join(__file__, os.pardir, os.pardir)
datadir = os.path.join(prefix, 'share', 'Synopsis')
