"""contains configuration data that are specific to a
particular release / installation such as version, data
paths, etc."""

import os, os.path, sys

if os.name == "posix":
   datadir = os.path.abspath(os.path.join(__file__,
                                          # lib / pythonx.y / site-packages / Synopsis / config.py
                                          os.pardir, os.pardir, os.pardir, os.pardir, os.pardir,
                                          'share', 'Synopsis'))
elif os.name == "nt":
   datadir = os.path.abspath(os.path.join(__file__,
                                          # lib / site-packages / Synopsis / config.py
                                          os.pardir, os.pardir, os.pardir, os.pardir,
                                          'share', 'Synopsis'))

elif os.name == "mac":
   datadir = os.path.abspath(os.path.join(__file__,
                                          # lib / site-packages / Synopsis / config.py
                                          os.pardir, os.pardir, os.pardir, os.pardir,
                                          'share', 'Synopsis'))

if not os.path.exists(datadir) or not os.path.isdir(datadir):
   # for now lets assume this is called from within the source tree
   # we need a more robust way to deal with this whole issue, though
   # --stefan

   datadir = os.path.abspath(os.path.join(__file__,
                                          os.pardir, os.pardir,
                                          'share', 'Synopsis'))
