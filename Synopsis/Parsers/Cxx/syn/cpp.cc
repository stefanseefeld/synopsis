// $Id: cpp.cc,v 1.1 2003/12/17 15:07:24 stefan Exp $
//
// Copyright (C) 2003 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifdef __WIN32__

# include "cpp-win32.cc"

#else

# include "cpp-posix.cc"

#endif
