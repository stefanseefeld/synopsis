//
// Copyright (C) 1997-2000 Shigeru Chiba
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#include "Buffer.hh"
#include "Lexer.hh"
#include "Ptree.hh"
#include "Class.hh"
#include <streambuf>
#include <iterator>
#include <string>
#include <stdexcept>

#if defined(PARSE_MSVC)
#define _MSC_VER	1100
#endif

Buffer::Buffer(std::streambuf *sb, const std::string &filename)
  : my_filename(filename),
    my_cursor(0)
{
  std::istreambuf_iterator<char> begin(sb), end;
  my_buffer.append(begin, end);
}

void Buffer::substitute(const char *from, const char *to,
			const char *begin, unsigned long end)
{
  // FIXME: this method needs a lot of work:
  // * the list should be sorted
  // * we need to define the behavior in case of overlapping regions
  // etc.
  my_replacements.push_back(Replacement(from, to, begin, end));
}

unsigned long Buffer::origin(const char *ptr, std::string &filename) const
{
  // Determine pos in file
  unsigned long cursor = ptr - my_buffer.data();
  unsigned long start = cursor;
  if(cursor > my_buffer.size())
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
	  filename = std::string(my_buffer.data() + begin, end);
	  return line;
 	}
 	break;
      }
    }
  }

  // if we are here the input file wasn't preprocessed and
  // thus the first line doesn't start with a line directive
  filename = my_filename;
  return 1 + lines;
}

void Buffer::write(std::ostream &os, const std::string &filename)
{
  // FIXME: for now disregard replacements, as we don't have any means
  // to test correct behavior
  std::ostreambuf_iterator<char> out(os);
  std::copy(my_buffer.begin(), my_buffer.end(), out);
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

#if defined(_MSC_VER) || defined(IRIX_CC)
  if(cursor + 4 <= my_buffer.size() && my_buffer.substr(cursor, 4) == "line")
  {
    // skip 'line' token and following whitespace
    cursor += 4;
    do
    {
      c = at(++cursor);
    } while(is_blank(c));
  }
#endif

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
 	  begin = b;
 	  end = cursor;
	  line = l;
 	}
      }
    }
  }
  return line;
}

Buffer::Replacement::Replacement(const char *f, const char *t,
				 const char *b, unsigned long e)
  : from(f), to(t),
    begin(b), end(e)
{
}
