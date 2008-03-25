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

  //. Return the size of the buffer.
  unsigned long size() const { return buffer_.size();}

  //. Report the character at the current position and advance one character.
  char get() { return cursor_ < buffer_.size() ? buffer_[cursor_++] : '\0';}
  //. Undo the last get.
  void unget() { --cursor_;}
  //. Reset the current position to position c.
  void reset(unsigned long c = 0) { cursor_ = c;}

  //. Report the current position.
  unsigned long position() const { return cursor_ - 1;}
  //. Report the character at position p.
  char at(unsigned long p) const { return buffer_[p];}
  //. Report the pointer at position p.
  char const *ptr(unsigned long p = 0) const { return buffer_.c_str() + p;}

  //. Replace the text between from and to by the text between
  //. begin and begin + length
  void replace(char const *from, char const *to,
	       char const *begin, unsigned long length);

  //. Return true if ptr points into a region that has been replaced.
  bool is_replaced(char const *ptr);

  //. Return the origin of the given pointer (filename, line number, column)
  void origin(char const *, std::string &, unsigned long &, unsigned long &) const;
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

  std::string   filename_;
  std::string   buffer_;
  unsigned long cursor_;
  Replacements  replacements_;
};
  
class Buffer::iterator : public std::iterator<std::forward_iterator_tag,
					      char const>
{
  friend class Buffer;
public:
  reference operator*() const 
  {
    if (replacement_) return replacement_->patch[offset_];
    else return *buffer_->ptr(cursor_);
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
    return (x.cursor_ == y.cursor_ && 
	    x.replacement_ == y.replacement_ && 
	    x.offset_ == y.offset_ && 
	    x.buffer_ == y.buffer_);
  }

private:
  iterator(Buffer const *b, unsigned long c)
    : buffer_(b), cursor_(c), replacement_(0), offset_(0) {}

  //. Advance an iterator to the next position.
  //. This takes replacement patches into account, so the iteration
  //. is done over the (potentially patched) buffer, not the original one.
  void next();
  void prev();

  Buffer const *             buffer_;
  unsigned long              cursor_;
  Buffer::Replacement const *replacement_;
  unsigned long              offset_;
  
};

inline bool operator!=(Buffer::iterator const& x, Buffer::iterator const& y)
{ return !(x == y);}

inline Buffer::iterator Buffer::begin() const { return iterator(this, 0);}
inline Buffer::iterator Buffer::end() const { return iterator(this, size());}

}

#endif
