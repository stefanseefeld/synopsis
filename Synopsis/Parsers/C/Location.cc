/* $Id: Location.cc,v 1.4 2003/12/30 07:33:06 stefan Exp $
 *
 * Copyright (C) 1998 Shaun Flisakowski
 * Copyright (C) 2003 Stefan Seefeld
 * All rights reserved.
 * Licensed to the public under the terms of the GNU LGPL (>= 2),
 * see the file COPYING for details.
 */

#include <Location.hh>
#include <iostream>

Location NoLocation(0,0,"");

void Location::printLocation(std::ostream& p) const
{
//     p << '"' << file << '"';
    p << ", " << line << ", " << column;
}

