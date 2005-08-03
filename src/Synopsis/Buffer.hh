//
// Copyright (C) 1997-2000 Shigeru Chiba
// Copyright (C) 2004 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#ifndef Synopsis_Buffer_hh_
#define Synopsis_Buffer_hh_

#include <streambuf>
#include <vector>

namespace Synopsis
{

//. Buffer holds the memory on top of which a parse tree / syntax tree is
//. constructed. Besides giving access to individual characters, it provides
//. the means to register replacements for buffer chunks, such that when
//. the Buffer's write method is executed the new file will contain the
//. modified source.
class Buffer
{
public:
  //. Iterate over the buffer content.
  class iterator;

  Buffer(std::streambuf *, const std::string & = std::string("unknown"));

  //. return the size of the buffer
  unsigned long size() const { return my_buffer.size();}

  //. report the character at the current position and advance one character
  char get() { return my_cursor < my_buffer.size() ? my_buffer[my_cursor++] : '\0';}
  //. undo the last get
  void unget() { --my_cursor;}
  //. reset the current position to position c
  void reset(unsigned long c = 0) { my_cursor = c;}

  //. report the current position
  unsigned long position() const { return my_cursor - 1;}
  //. report the character at position p
  char at(unsigned long p) const { return my_buffer[p];}
  //. report the pointer at position p
  char const *ptr(unsigned long p = 0) const { return my_buffer.c_str() + p;}

  //. replace the text between from and to by the text between
  //. begin and begin + length
  void replace(char const *from, char const *to,
	       char const *begin, unsigned long length);

  //. Return the origin of the given pointer (filename and line number)
  unsigned long origin(char const *, std::string &) const;
  //. Write the buffer into the given output stream
  //. The first line contains a line directive issuing the input file name;
  //. if filename is non-empty, use this to fake another one.
  void write(std::streambuf *, std::string const &) const;

  iterator begin() const;
  iterator end() const;

private:
  struct Replacement
  {
    Replacement(char const *from, char const *to, std::string const &patch);
    static bool smaller(Replacement const &r1, Replacement const &r2)
    { return r1.from < r2.from;}
    char const *  from;   //.< The start of the region to be replaced.
    char const *  to;     //.< The end of the region to be replaced.
    std::string   patch;  //.< The replacement patch.
  };
  typedef std::vector<Replacement> Replacements;

  //. read a line directive starting at position pos, and return
  //. the line number found. Also report the begin and end of the filename
  //. (with respect to the internal buffer).
  //. line is the default line number that gets reported on error (in
  //. which case begin and end remain unchanged)
  long read_line_directive(unsigned long cursor, long line,
			   unsigned long &begin, unsigned long &end) const;

  std::string   my_filename;
  std::string   my_buffer;
  unsigned long my_cursor;
  Replacements  my_replacements;
};
  
class Buffer::iterator : public std::iterator<std::forward_iterator_tag,
					      char const>
{
  friend class Buffer;
public:
  reference operator*() const 
  {
    if (my_replacement) return my_replacement->patch[my_offset];
    else return *my_buffer->ptr(my_cursor);
  }
  pointer operator->() const { return &(operator*());}
  iterator& operator++() { next(); return *this;}
  iterator operator++(int)
  {
    iterator tmp = *this;
    next();
    return tmp;
  }

  friend bool operator==(Buffer::iterator const& x, Buffer::iterator const& y)
  {
    return (x.my_cursor == y.my_cursor && 
	    x.my_replacement == y.my_replacement && 
	    x.my_offset == y.my_offset && 
	    x.my_buffer == y.my_buffer);
  }

private:
  iterator(Buffer const *b, unsigned long c)
    : my_buffer(b), my_cursor(c), my_replacement(0), my_offset(0) {}

  //. Advance an iterator to the next position.
  //. This takes replacement patches into account, so the iteration
  //. is done over the (potentially patched) buffer, not the original one.
  void next();
  void prev();

  Buffer const *             my_buffer;
  unsigned long              my_cursor;
  Buffer::Replacement const *my_replacement;
  unsigned long              my_offset;
  
};

inline bool operator!=(Buffer::iterator const& x, Buffer::iterator const& y)
{ return !(x == y);}

inline Buffer::iterator Buffer::begin() const { return iterator(this, 0);}
inline Buffer::iterator Buffer::end() const { return iterator(this, size());}

}

#endif
