#
# Copyright (C) 2007 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

import re

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
