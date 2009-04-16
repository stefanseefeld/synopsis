//
// Copyright (C) 2009 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#ifndef comments_hh_
#define comments_hh_

#include <vector>
#include <string>

// accumulate comments that may be macro docstrings
void add_ccomment(char const *);
void add_cxxcomment(char const *);
void clear_comment_cache();
void add_newline();

extern std::vector<std::string> comment_cache;

#endif
