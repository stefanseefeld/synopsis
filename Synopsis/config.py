"""contains configuration data that are specific to a
particular release / installation such as version, data
paths, etc."""

import os, os.path

datadir = os.path.abspath(os.path.join(__file__,
                                       os.pardir, os.pardir,
                                       'share', 'Synopsis'))
