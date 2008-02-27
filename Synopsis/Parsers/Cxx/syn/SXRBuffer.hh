//
// Copyright (C) 2008 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef SXRBuffer_hh_
#define SXRBuffer_hh_

#include <fstream>
#include <string>
#include <iostream>

// An SXRBuffer generates a xml-ified version of the source file, with
// identifiers suitably annotated to allow linking.
// Since the processing of the parse tree is not monotonic (for example, 
// member functions are parsed only after the full class body has been seen),
// the entries aren't generated in-order. We collect them here, then write out
// the whole file once the translation has finished.
class SXRBuffer
{
public:
  struct Entry
  {
    enum Kind { SPAN, XREF};
    //. Less-than functor to order entries within lines by column.
    struct less
    {
      bool operator() (Entry const &a, Entry const &b) const
      { return a.column < b.column;}
    };

    Entry(unsigned int c, int l, Kind k, std::string const &n, std::string const &t,
          std::string const &f, std::string const &d, bool a)
      : column(c), length(l), kind(k), name(n), type(t), from(f), description(d),
        continuation(a) {}

    unsigned int column;
    int length;
    Kind kind;
    std::string name;
    std::string type;
    std::string from;
    std::string description;
    bool continuation;
  };

  SXRBuffer(std::string const &out, std::string const &in, std::string const &name)
  // line numbers are reported starting from 1
    : line_(1), column_(0), new_line_(true)
  {
    outbuf_.open(out.c_str(), std::ios_base::out);
    inbuf_.open(in.c_str(), std::ios_base::in);
    outbuf_.sputn("<sxr filename=\"", 15);
    outbuf_.sputn(name.c_str(), name.size());
    outbuf_.sputn("\">\n", 3);
  }
  ~SXRBuffer()
  {
    outbuf_.sputn("</sxr>", 6);
    inbuf_.close();
    outbuf_.close();
  }

  void insert_span(unsigned int line, unsigned int column, unsigned int length,
                   std::string const &type)
  {
    Line &l = lines_[line];
    l.insert(Entry(column, length, Entry::SPAN, "", type, "", "", false));
  }
  void insert_xref(unsigned int line, unsigned int column, unsigned int length,
                   std::string const &name, std::string const &type,
                   std::string const &from, std::string const &description,
                   bool continuation)
  {
    Line &l = lines_[line];
    l.insert(Entry(column, length, Entry::XREF, encode(name),
                   type, encode(from), encode(description), continuation));
  }

  void write()
  {
    unsigned int line = 0;
    while (advance(line, 0))
    {
      Line &l = lines_[line];
      Line::iterator i = l.begin();
      while (i != l.end())
      {
        Entry const &entry = *i;
        advance(line, entry.column);
        switch (entry.kind)
        {
          case Entry::SPAN:
            insert("<span class=\"", 13);
            insert(entry.type.data(), entry.type.size());
            insert("\">", 2);
            if (entry.length == -1) finish_line();
            else advance(line, entry.column + entry.length);
            insert("</span>", 7);
            break;
          case Entry::XREF:
            insert("<a href=\"", 9);
            insert(entry.name.data(), entry.name.size());
            insert("\" title=\"", 9);
            insert(entry.description.data(), entry.description.size());
            insert("\" from=\"", 8);
            insert(entry.from.data(), entry.from.size());
            insert("\" type=\"", 8);
            insert(entry.type.data(), entry.type.size());
            if (entry.continuation)
              insert("\" continuation=\"true", 20);
            insert("\">", 2);
            if (entry.length == -1) finish_line();
            else advance(line, entry.column + entry.length);
            insert("</a>", 4);
            break;
        }
        ++i;
      }
      ++line;
    }
  }

private:

  std::string encode(std::string const &n)
  {
    std::string retn;
    for (std::string::const_iterator i = n.begin(); i != n.end(); ++i)
      switch (*i)
      {
        case '<':
          retn += "&lt;";
          break;
        case '>':
          retn += "&gt;";
          break;
        case '&':
          retn += "&amp;";
          break;
        case '"':
          retn += "&quot;";
          break;
        default:
          retn += *i;
      }
    return retn;
  }

  void insert(char const *s, unsigned int l)
  {
    if (new_line_)
    {
      outbuf_.sputn("<line>", 6);
      new_line_ = false;
    }
    outbuf_.sputn(s, l);
  }

  void put(char c)
  {
    if (new_line_)
    {
      outbuf_.sputn("<line>", 6);
      new_line_ = false;
    }
    switch (c)
    {
      case '\n':
        outbuf_.sputn("</line>\n", 8);
        ++line_;
        column_ = 0;
        new_line_ = true;
        break;
      case '&':
        outbuf_.sputn("&amp;", 5);
        ++column_;
        break;
      case '<':
        outbuf_.sputn("&lt;", 4);
        ++column_;
        break;
      case '>':
        outbuf_.sputn("&gt;", 4);
        ++column_;
        break;
      default:
        outbuf_.sputc(c);
        ++column_;
        break;
    }
  }
  
  bool finish_line()
  {
    std::istreambuf_iterator<char> in(&inbuf_);
    std::istreambuf_iterator<char> end;
    while (in != end && *in != '\n') put(*in++);
    return in == end;
  }

  // Advance till the given input source file position.
  bool advance(unsigned int line, unsigned int column)
  {
    std::istreambuf_iterator<char> in(&inbuf_);
    std::istreambuf_iterator<char> end;
    while ((line_ < line || column_ < column) && in != end)
      put(*in++);
    return in != end;
  }

  typedef std::set<Entry, Entry::less> Line;
  typedef std::map<int, Line> Map;
  
  Map lines_;

  std::filebuf inbuf_;
  std::filebuf outbuf_;
  unsigned int line_;
  unsigned int column_;
  bool new_line_;
};

#endif
