/* $Id: Location.hh,v 1.5 2003/12/30 07:33:06 stefan Exp $
 *
 * Copyright (C) 1998 Shaun Flisakowski
 * Copyright (C) 2003 Stefan Seefeld
 * All rights reserved.
 * Licensed to the public under the terms of the GNU LGPL (>= 2),
 * see the file COPYING for details.
 */

#ifndef _Location_hh
#define _Location_hh

#include <string>

struct Location
{
  Location(int l, int c, const std::string& f)
    : line(l), column(c), file(f) {}
  ~Location() {}

  void printLocation(std::ostream& p) const;

  const int line, column;
  const std::string file;
};

extern  Location NoLocation;

#endif

