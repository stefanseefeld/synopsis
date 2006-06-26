//
// Copyright (C) 1997-2000 Shigeru Chiba
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#include "Synopsis/Buffer.hh"
#include "Synopsis/Lexer.hh"
#include <streambuf>
#include <iterator>
#include <string>
#include <algorithm>
#include <stdexcept>

#if defined(PARSE_MSVC)
#define _MSC_VER	1100
#endif

using namespace Synopsis;

void Buffer::iterator::next()
{
  if (replacement_)
  {
    ++offset_;
    if (offset_ >= replacement_->patch.size())
    {
      cursor_ = replacement_->to - buffer_->ptr();
      replacement_ = 0;
      offset_ = 0;
    }
  }
  else
  {
    ++cursor_;
    for (Buffer::Replacements::const_iterator i = buffer_->replacements_.begin();
	 i != buffer_->replacements_.end();
	 ++i)
      if (i->from == buffer_->ptr(cursor_))
      {
	replacement_ = &*i;
	offset_ = 0;
	break;
      }
  }
}

Buffer::Buffer(std::streambuf *sb, std::string const &filename)
  : filename_(filename),
    cursor_(0)
{
  std::istreambuf_iterator<char> begin(sb), end;
  buffer_.append(begin, end);
}

void Buffer::replace(char const *from, char const *to,
		     char const *begin, unsigned long length)
{
  Replacements::iterator i = replacements_.begin();

  // Find the iterator after which to insert the new patch.
  while (i != replacements_.end() && i->from < from) ++i;
  
  // TODO: handle overlapping replacements.
  replacements_.insert(i, Replacement(from, to, std::string(begin, length)));
}

bool Buffer::is_replaced(char const *ptr)
{
  Replacements::iterator i = replacements_.begin();

  // Find the iterator after which to insert the new patch.
  while (i != replacements_.end() && i->to < ptr) ++i;
  if (i != replacements_.end() && i->from < ptr) return true;
  else return false;
}

unsigned long Buffer::origin(char const *ptr, std::string &filename) const
{
  // Determine pos in file
  unsigned long cursor = ptr - buffer_.data();
  if(cursor > buffer_.size())
    throw std::invalid_argument("pointer out of bound");

  long lines = 0;

  while(cursor)
  {
    // move back to the last line directive and report
    // the line number read there plus the number of lines in between
    switch(at(--cursor))
    {
      case '\n':
 	++lines;
 	break;
      case '#':
      {
 	unsigned long begin = 0, end = 0;
	long l = read_line_directive(cursor, -1, begin, end);
 	if(l >= 0)
 	{
 	  unsigned long line = static_cast<unsigned long>(l) + lines;
	  filename = std::string(buffer_.data() + begin, end - begin);
	  return line;
 	}
 	break;
      }
    }
  }

  // if we are here the input file wasn't preprocessed and
  // thus the first line doesn't start with a line directive
  filename = filename_;
  return 1 + lines;
}

void Buffer::write(std::streambuf *sb, std::string const &/* filename */) const
{
  // FIXME: what should we do with overlapping replacements ?
  Replacements replacements(replacements_);
  std::sort(replacements.begin(), replacements.end(), Replacement::smaller);
  std::ostreambuf_iterator<char> out(sb);
  char const *b = buffer_.data();
  for (Replacements::iterator r = replacements.begin();
       r != replacements.end();
       ++r)
  {
    std::copy(b, r->from, out);
    std::copy(r->patch.data(), r->patch.data() + r->patch.size(), out);
    b = r->to;
    if (*b == '\0') break;
  }
  std::copy(b, buffer_.data() + buffer_.length(), out);
}

long Buffer::read_line_directive(unsigned long cursor, long line,
				 unsigned long &begin, unsigned long &end) const
{
  char c;
  // skip leading whitespace
  do
  {
    c = at(++cursor);
  } while(is_blank(c));

  // gcc only generates '#' instead of '#line', so make the following check
  // optional.
  if(cursor + 4 <= buffer_.size() && buffer_.substr(cursor, 4) == "line")
  {
    // skip 'line' token and following whitespace
    cursor += 4;
    do
    {
      c = at(++cursor);
    } while(is_blank(c));
  }

  if(is_digit(c))
  {		/* # <line> <file> */
    // extract a decimal number
    unsigned long num = c - '0';
    while(true)
    {
      c = at(++cursor);
      if(is_digit(c)) num = num * 10 + c - '0';
      else break;
    }

    // as the line number reported reports for the *next* line, we set
    // it to one below as it gets incremented on the next newline
    long l = num - 1;

    // skip whitespace
    if(is_blank(c))
    {
      do
      {
 	c = at(++cursor);
      } while(is_blank(c));

      // now get the filename
      if(c == '"')
      {
 	unsigned long b = cursor;
 	do
 	{
 	  c = at(++cursor);
 	} while(c != '"');

 	if(cursor > b + 2)
 	{
	  // the line is well-formed, let's set the out parameters
 	  begin = b + 1;
 	  end = cursor;
	  line = l;
 	}
      }
    }
  }
  return line;
}

Buffer::Replacement::Replacement(char const *f, const char *t,
				 std::string const &p)
  : from(f), to(t), patch(p)
{
}
