//
// Copyright (C) 2000 Stephen Davies
// Copyright (C) 2000 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include <Synopsis/Python/Object.hh>
#include <Synopsis/Python/Module.hh>
#include <iostream>
#include <fstream>
#include <iterator>
#include <vector>
#include <set>
#include <map>
#include <string>
#include <limits.h>

#include <stdio.h>

using namespace Synopsis;

//. Static namespace for link module
namespace
{
//. Encapsulation of a link fragment. Links can be of several types, such
//. as start, end, or some other formatting type. They are split into
//. start and end so that spans can be nested properly
struct Link
{
  //. A scoped name type
  typedef std::vector<std::string> Name;

  //. Enumeration of the type of link. These should be in order of
  //. priority, so nested types should be nested in this list:
  //. <a><b>foo</b></a> means enum ordering: A_START,B_START,B_END,A_END
  enum Type
  {
    LINK_START, //.< Start of a label link
    REF_START, //.< Start of reference link
    SPAN_START,
    SPAN_END,
    REF_END,
    LINK_END //.< End of a link
  };

  //. Less-than functor that compares column and type
  struct lt_col
  {
    bool operator() (const Link* a, const Link* b) const
    {
      if (a->col != b->col)
	return a->col < b->col;
      return a->type < b->type;
    }
  };
  //. Set of links sorted by column
  typedef std::set<Link *, lt_col> Line;
  //. Map of Lines keyed by line number
  typedef std::map<int, Line> Map;

  //. The line and column of the link
  int line, col;
  //. The type of this link
  Type type;
  //. The scoped name of this link
  Name name;
  //. The description of this link
  std::string desc;

  //. Write link to the output stream for debugging
  std::ostream& write(std::ostream& o);
};

//. Debugging output operator for members of the Link Map
std::ostream& operator<<(std::ostream& o, const Link::Map::value_type& linepair)
{
  const Link::Line& line = linepair.second;
  o << "Line " << (*line.begin())->line << "\n";
  Link::Line::const_iterator iter = line.begin();
  while (iter != line.end()) (*iter++)->write(o) << "\n";
  return o;
}

//. Debugging output method for Links
std::ostream& Link::write(std::ostream& o)
{
  o << " " << col << " (" << type << ") ";
  Link::Name::const_iterator iter = name.begin();
  if (iter == name.end()) return o << "<no name>";
  o << *iter++;
  while (iter != name.end()) o << "::" << *iter++;
  return o;
}

//. A map of links to insert into the output
Link::Map links;

PyObject *error;

//. Writes some text to the output. It replaces chars that might be
//. confused by HTML, such as < and >. Tabs are expanded to 8-col tabstops (hence the
//. col argument)
void write(std::ostream& out, int col, char* buf, int len, int buflen)
{
  char* ptr = buf, *end = buf+len;
  while (ptr != end && col < buflen)
  {
    char c = *ptr++;
    switch (c)
    {
      case '<':
	out << "&lt;";
	break;
      case '>':
	out << "&gt;";
	break;
      case '"':
	out << "&quot;";
	break;
      case '&':
	out << "&amp;";
	break;
      case '\t':
      {
	int next = ((col/8)+1)*8;
	while (col++ < next) out << ' ';
	continue;
      }
      default:
	out << c;
    }
    col++;
  }
}

//. Writes the line number to the output
void write_lineno(std::ostream& out, int line)
{
  out << "<a name=\"" << line << "\"></a>";
  out << "<span class=\"lineno\">";
  int mag = 10000;
  while (mag > 1)
  {
    int digit = line / mag;
    if (digit)
      break;
    out << ' ';
    mag /= 10;
  }
  out << line << "</span>";
}

//. Undoes the %FF encoding
std::string decode(const std::string& str)
{
  std::string ret;
  std::string::const_iterator iter = str.begin(), end = str.end();
  while (iter != end)
  {
    char a, b, c = *iter++;
    if (c == '%')
    {
      a = *iter++;
      b = *iter++;
      if (a >= 'a') a -= 'a' - 10;
      else if (a >= 'A') a -= 'A' - 10;
      else a -= '0';
      
      if (b >= 'a') b -= 'a' - 10;
      else if (b >= 'A') b -= 'A' - 10;
      else b -= '0';
      
      c = a * 16 + b;
    }
    ret.push_back(c);
  }
  return ret;
}

//. Writes whatever indent there is in the buf to the output using the
//. special span. 'col' is modified to the first non-indent character.
void write_indent(std::ostream& out, char* buf, int& col, int buflen)
{
  int len = 0;
  char* ptr = buf;
  while (*ptr && (*ptr == ' ' || *ptr == '\t')) ptr++, len++;
  if (!len) return;
//   out << "<span class=\"file-indent\">";
  write(out, col, buf, len, buflen);
//   out << "</span>";
  col += len;
}

//. Returns true if the link is a duplicate.
bool is_duplicate(Link* link, int len)
{
  Link::Line& line = links[link->line];
  Link::Line::iterator iter = line.find(link);
  if (iter == line.end()) return false;

  // iter matches by col, but try to find match by name
  while ((*iter)->name != link->name)
  {
    ++iter;
    if (iter == line.end() || (*iter)->col != link->col)
      // Couldn't match by name for this column
      return false;
  }

  // Matched start by name.. try and find end too
  link->col += len;
  iter = line.find(link);
  if (iter == line.end()) return false;

  // iter matches by col, but try to find match by name
  while ((*iter)->name != link->name)
  {
    ++iter;
    if (iter == line.end() || (*iter)->col != link->col)
      // Couldn't match by name for this column
      return false;
  }
  // name matched at both ends
  return true;
}

//. Reads the links file into the 'links' map.
void read_links(const char *links_filename) throw (std::string)
{
  std::ifstream in(links_filename);
  char buf[4096];
  if (!in) return;
  // this is okay -- just means the file wont be linked
  std::string word, type;
  int line, len;
  while (in)
  {
    if (!(in >> line)) break;
    Link* link = new Link;
    link->line = line;
    in >> link->col >> len >> type;
    link->col--; // we count at zero, file counts at one
    if (len == -1) len = INT_MAX/2; // div 2 to prevent wrap-around
    if (type != "SPAN")
    {
      if (type == "DEF") link->type = Link::LINK_START;
      else if (type == "REF") link->type = Link::REF_START;
      else if (type == "CALL") link->type = Link::REF_START;
      else if (type == "IMPL") link->type = Link::REF_START;
      else if (type == "UDIR") link->type = Link::REF_START;
      else link->type = Link::REF_START;
      int c = -1;
      // Use up field sep ' '
      in.get();
      // Loop over scoped name till next ' '
      do
      {
	in >> word;
	// Replace '160's with spaces
	//for (std::string::size_type pos = word.find(160); pos != std::string::npos; pos = word.find(160, pos)) {
	//    word[pos] = ' ';
	//}
	word = decode(word);
	size_t start = 0, end;
	while ((end = word.find('\t', start)) != std::string::npos)
	{
	  link->name.push_back(word.substr(start, end-start));
	  start = end + 1;
	}
	link->name.push_back(word.substr(start, end-start));
      }
      while (in && (c = in.get()) != '\n' && c != ' ');
      // Read description
      if (!in.getline(buf, 4096)) break;
      link->desc = decode(buf);
    }
    else
    {
      link->type = Link::SPAN_START;
      in >> type;
      link->name.push_back(decode(type));
    }
    // Skip to next line if duplicate link
    if (is_duplicate(link, len)) continue;
    links[line].insert(link);
    // Add link end
    Link* end = new Link;
    end->line = line;
    end->col = link->col + len;
    switch (link->type)
    {
      case Link::LINK_START:
	end->type = Link::LINK_END;
	break;
      case Link::REF_START:
	end->type = Link::REF_END;
	break;
      case Link::SPAN_START:
	end->type = Link::SPAN_END;
	break;
      default:; // throw some error
    }
    links[line].insert(end);
  }
}

//. Debugging method to dump all links to cout.
void dump_links()
{
  std::copy(links.begin(), links.end(), std::ostream_iterator<Link::Map::value_type>(std::cout, "\n"));
}

std::string string_to_attribute(const std::string &o)
{
  std::string name;
  for (std::string::const_iterator i = o.begin(); i != o.end(); ++i)
    switch (*i)
    {
      case '&': name += "&amp;"; break;
      case '<': name += "&lt;"; break;
      case '>': name += "&gt;"; break;
      default: name += *i; break;
    }
  return name;
}

std::string scoped_name_to_attribute(const Python::List &o)
{
  std::string name;
  if (o.empty()) return name;
  for (Python::List::iterator i = o.begin(); i != o.end() - 1; ++i)
  {
    name = string_to_attribute(Python::Object::narrow<std::string>(*i)) + '.';
  }
  name += string_to_attribute(Python::Object::narrow<std::string>(*o.rbegin()));
  return name;
}

std::string scoped_name_to_href(const Python::List &o)
{
  std::string name;
  if (o.empty()) return name;
  for (Python::List::iterator i = o.begin(); i != o.end() - 1; ++i)
  {
    name = string_to_attribute(Python::Object::narrow<std::string>(*i)) + "::";
  }
  name += string_to_attribute(Python::Object::narrow<std::string>(*o.rbegin()));
  return name;
}

//. Reads the input file, inserts links, and writes the result to the
//. output. It uses the 'links' line map to iterate through the file
//. sequentially.
void link_file(const char *input_filename,
	       const char *output_filename,
	       Python::Object map) throw (std::string)
{
  Python::Object qname_module = Python::Module::import("Synopsis.QualifiedName");
  Python::Object qname_factory = qname_module.attr("QualifiedCxxName");

  std::ifstream in(input_filename);
  if (!in)
  {
    throw std::string("Error opening input file: ")+input_filename;
  }
  std::ofstream out(output_filename, std::ios::out);
  if (!out)
  {
    throw std::string("Error opening output file: ")+output_filename;
  }
  char buf[4096];
  int line = 1, buflen;
  Link::Map::iterator iter = links.begin(), end = links.end();
  out << "<pre class=\"sxr\">\n";
  while (in)
  {
    // Get line
    if (!in.getline(buf, 4096)) break;
    buflen = strlen(buf);
    write_lineno(out, line);
    out << "<span class=\"line\">";
    // Get Link::Line    
    while (iter != end && iter->first < line) ++iter;
    if (iter != end && iter->first == line)
    {
      // Insert links and write at same time
      int col = 0;
      write_indent(out, buf, col, buflen);
      Link::Line& line = iter->second;
      Link::Line::iterator link_i = line.begin();
      while (link_i != line.end())
      {
	Link* link = *link_i++;
	if (col < link->col)
	{
	  write(out, col, buf+col, link->col - col, buflen);
	  col = link->col;
	}
	switch (link->type)
	{
	  case Link::LINK_START:
	  case Link::REF_START:
	  {
	    Python::List name(link->name.begin(), link->name.end());
            Python::Object qname = qname_factory(name);
	    Python::Object retn = map(qname);
	    if (PyErr_Occurred())
	    {
	      PyErr_Print();
	      PyErr_Clear();
	      return;
	    }
	    else if (!retn)
	    {
	      if (link->type == Link::LINK_START)
		out << "<a name=\"" << scoped_name_to_attribute(name);
	      else
		out << "<a href=\"#" << scoped_name_to_href(name);
	    }
	    else
	    {
	      std::string href = Python::Object::narrow<std::string>(retn);
	      if (link->type == Link::LINK_START)
		out << "<a class=\"file-def\" name=\""
		    << scoped_name_to_attribute(name) << "\"";
	      else
		out << "<a class=\"file-ref\"";
	      out << " href=\"" << href;
	    }
	    out << "\" title=\"" << string_to_attribute(link->desc) << "\">";
	    break;
	  }
	  case Link::REF_END:
	  case Link::LINK_END:
	    out << "</a>";
	    break;
	  case Link::SPAN_START:
	    out << "<span class=\"" << link->name[0] << "\">";
	    break;
	  case Link::SPAN_END:
	    out << "</span>";
	    break;
	}
      }
      // Write any left-over buffer
      write(out, col, buf+col, -1, buflen);
    }
    else
    {
      // Write buf
      int col = 0;
      write_indent(out, buf, col, buflen);
      if (col < buflen) write(out, col, buf+col, -1, buflen);
    }
    out << "</span>\n";
    line++;
  }
  out << "</pre>\n";
}

//. The main python method, equivalent to the main() function
PyObject* py_link(PyObject* self, PyObject* args)
{
  PyObject *py_toc;
  char *input_filename = 0;
  char *output_filename = 0;
  char *links_filename = 0;

  if (!PyArg_ParseTuple(args, "Osss",
			&py_toc,
			&input_filename, &output_filename, &links_filename))
    return 0;
  try
  {
    Python::Object map(py_toc);
    read_links(links_filename);
    link_file(input_filename, output_filename, map);
    links.clear();
  }
  catch (const std::string &err)
  {
    std::cerr << "Error: " << err << std::endl;
    PyErr_SetString(error, err.c_str());
    links.clear();
    return 0;
  }
  
  return Python::Object().ref();
}

//. The list of methods in this python module
PyMethodDef methods[] = {{(char*)"link", py_link, METH_VARARGS},
			 {0, 0}};
}

//. The initialisation method for this module
extern "C" void initlink()
{
  Python::Module module = Python::Module::define("link", methods);
  module.set_attr("version", "0.1");
  error = PyErr_NewException("link.error", 0, 0);
  module.set_attr("error", error);
}

