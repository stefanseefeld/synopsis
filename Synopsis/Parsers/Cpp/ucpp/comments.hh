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

// A file preamble is the first set of comments
// in a file, preceding any non-comment token.
void start_file_preamble();
void end_file_preamble();
bool may_be_file_preamble();
// accumulate comments that may be macro docstrings
void add_ccomment(char const *);
void add_cxxcomment(char const *);
void clear_comment_cache();
void add_newline();

extern std::vector<std::string> comment_cache;

#endif
