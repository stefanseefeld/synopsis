//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#include "Formatter.hh"
#include <iostream>

namespace
{
char const *header =
  "<?xml version=\"1.0\"?>\n"
  "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\"\n"
  "   \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n"
  "<html xmlns=\"http://www.w3.org/1999/xhtml\" lang=\"en\">\n"
  "  <head>\n"
  "    <meta content=\"text/html; charset=iso-8859-1\" http-equiv=\"Content-Type\"/>\n"
  "    <link rel=\"stylesheet\" href=\"html.css\" type=\"text/css\">\n"
  "  </head>\n"
  "  <body>\n"
  "  <div class=\"source\">\n"
  "  <pre><a name=\"1\"></a><span class=\"linenum\">&#160;&#160;&#160;&#160;1|&#160;</span>";
  
char const *trailer =
  "  </pre>\n"
  "  </div>\n"
  "  </body>\n"
  "</html>\n";

char const *newline = "</pre>\n<pre>";

char const *comment_start = "<span class=\"comment\">";

//. Writes the line number to the output
void write_lineno(std::ostream& out, int line)
{
  // out << setw(4) << line << "| "; <-- but with &#160;'s
  out << "<a name=\"" << line << "\"></a>";
  out << "<span class=\"linenum\">";
  int mag = 10000;
  while (mag > 1)
  {
    int digit = line / mag;
    if (digit)
      break;
    out << "&#160;";
    mag /= 10;
  }
  out << line << "|&#160;";
  out << "</span>";
}
}

using namespace Synopsis;

Formatter::Formatter(std::ostream &os) 
  : my_os(os),
    my_lineno(1),
    my_comment(false),
    my_ccomment(false)
{
}

void Formatter::format(Buffer::iterator begin, Buffer::iterator end)
{
  my_os << header;
  unsigned long column = 0;
  while (begin != end)
  {
    switch (*begin)
    {
      case '<': my_os << "&lt;"; break;
      case '>': my_os << "&gt;"; break;
      case ' ': my_os << "&#160;"; break;
      case '"': my_os << "&quot;"; break;
      case '&': my_os << "&amp;"; break;
      case '\n':
	if (my_ccomment || my_comment)
	  my_os << "</span>";
	my_comment = false;
	my_os << newline;
	write_lineno(my_os, my_lineno);
	++my_lineno;
	column = 0;
	if (my_ccomment)
	  my_os << comment_start;
	break;
      case '/':
      {
	Buffer::iterator next = begin;
	++next; // Buffer::iterator is only a forward iterator, not random access...
	if (next != end)
	  switch (*next)
	  {
	    case '/':
	      my_os << comment_start << "//";
	      begin = next;
	      my_comment = true;
	      break;
	    case '*':
	      my_os << comment_start << "/*";
	      begin = next;
	      my_ccomment = true;
	      break;
  	    default:
	      my_os.put('/');
	      break;
	  }
	break;
      }
      case '*':
	my_os.put('*');
	if (my_ccomment)
	{
	  Buffer::iterator next = begin;
	  ++next;
	  if (next != end && *next == '/')
	    my_os << "/</span>";
	  my_ccomment = false;
	}
	break;
      default:
	if (static_cast<unsigned char>(*begin) >= 128)
	  my_os.put(static_cast<unsigned char>(*begin) - 128);
	else
	  my_os.put(*begin);
	break;
    }
    ++begin;
  }

  my_os << trailer;
}

