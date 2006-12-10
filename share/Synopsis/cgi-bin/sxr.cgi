#!/usr/bin/env python
#
# Copyright (C) 2004 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

import os, cgi, cgitb
from Synopsis.SXRServer import SXRServer

cgitb.enable()

if __name__ == '__main__':

    root = os.environ.get('SXR_ROOT_DIR', '.')
    src_url = os.environ.get('SXR_SRC_URL', '')
    cgi_url = os.environ.get('SXR_CGI_URL', '.')
    path = os.environ.get('PATH_INFO', '/').split('/')
    command = len(path) > 1 and path[1] or ''
    path_info = '/'.join(path[2:])
    script_name = os.environ.get('SCRIPT_NAME', None)
    server = SXRServer(root, cgi_url, src_url, os.path.join(root, 'sxr-template.html'))

    form = cgi.FieldStorage()
    if command == 'file':
       pattern = form.has_key('string') and form['string'].value or None
       print server.search_file(pattern)
    elif command == 'ident':
       name = form.has_key('string') and form['string'].value or None
       qualified = form.has_key('full')
       print server.search_ident(name, qualified)
