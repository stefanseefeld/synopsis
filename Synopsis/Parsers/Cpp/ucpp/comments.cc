//
// Copyright (C) 2009 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#include "comments.hh"

extern "C" void synopsis_file_preamble_hook();

std::vector<std::string> comment_cache;
namespace
{
// A file preamble may contain file-specific metadata.
bool in_file_preamble = true;
// A value > 1 means the next comment is not to be concatenated to the previous.
int newlines = 1;
}

void start_file_preamble()
{
  in_file_preamble = true;
}

void end_file_preamble()
{
  if (in_file_preamble) synopsis_file_preamble_hook();
  in_file_preamble = false;
}

bool may_be_file_preamble()
{
  return in_file_preamble;
}

void add_ccomment(char const *comment)
{
  comment_cache.push_back(comment);
  newlines = 1;
}

void add_cxxcomment(char const *comment)
{
  std::string line = comment;
  if (newlines > 1 || comment_cache.size() == 0)
    comment_cache.push_back(line);
  else
  {
    std::string &last = comment_cache.back();
    last.append(line);
  }
  newlines = 0;
}

void add_newline()
{
  if (++newlines == 1 && comment_cache.size())
  {
    std::string &last = comment_cache.back();
    last.append("\n");
  }
}

void clear_comment_cache()
{
  comment_cache.clear();
  ++newlines;
}

