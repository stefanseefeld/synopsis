// vim: set ts=8 sts=2 sw=2 et:
// $Id: strace.hh,v 1.7 2002/01/28 13:17:24 chalky Exp $
//
// This file is a part of Synopsis.
// Copyright (C) 2000, 2001 Stephen Davies
// Copyright (C) 2000, 2001 Stefan Seefeld
//
// Synopsis is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
// 02111-1307, USA.
//
// $Log: strace.hh,v $
// Revision 1.7  2002/01/28 13:17:24  chalky
// More cleaning up of code. Combined xref into LinkStore. Encoded links file.
//
// Revision 1.6  2001/07/23 15:29:35  chalky
// Fixed some regressions and other mis-features
//
// Revision 1.5  2001/06/11 10:37:30  chalky
// Operators! Arrays! (and probably more that I forget)
//
// Revision 1.4  2001/06/10 07:17:37  chalky
// Comment fix, better functions, linking etc. Better link titles
//
// Revision 1.3  2001/05/23 05:08:47  stefan
// more std C++ issues. It still crashes...
//
// Revision 1.2  2001/05/06 20:15:03  stefan
// fixes to get std compliant; replaced some pass-by-value by pass-by-const-ref; bug fixes;
//
// Revision 1.1  2001/03/16 04:42:00  chalky
// SXR parses expressions, handles differences from macro expansions. Some work
// on function call resolution.
//
//

#ifndef H_SYNOPSIS_CPP_STRACE
#define H_SYNOPSIS_CPP_STRACE

#include <string>
#include <iostream>
#include <sstream>
#include <exception>
#include <list>

class Ptree;

#if 1 && defined(DEBUG)
#define DO_TRACE
//. A tracer class that can be used as a Guard around sections of code. The
//. instances are kept in a class-static stack, and enter/leave messages are
//. only printed when output is made at a given level. To work this, two
//. indices to the stack level are used: slevel and dlevel. The Stack Level
//. indicates the current depth of the stack, including non-displayed levels.
//. The Display Level indicates the level up to which enter statements have
//. been printed. Leave statements are only printed if slevel falls below
//. dlevel.
class STrace
{
  //. A convenient typedef
  typedef std::list<std::string> string_list;
public:
  //. Constructor
  STrace(const std::string &s)
  : m_scope(s)
  {
    m_list.push_back(indent() + "entering " + m_scope);
    ++slevel;
  }
  //. Destructor, called when leaving a scope. Only print out leaving messages
  //. if the enter was displayed.
  ~STrace()
  {
    if (dlevel > --slevel)
      {
        // 'enter' message was displayed, so display leave message too
        std::cout << indent() << "leaving  " << m_scope << std::endl;
        --dlevel;
      }
    else
      // 'enter' message wasn't displayed, so remove it from list
      m_list.pop_back();
  }
  //. Insertion operator, used to start logging a line
  std::ostream& operator <<(const std::string& s)
  {
    // Catch up on skipped enter messages
    while (dlevel < slevel)
      {
        std::cout << m_list.front() << "\n";
        m_list.pop_front();
        ++dlevel;
      }
    // Start current log message at correct indent
    std::cout << indent() << s;
    return std::cout;
  }
  //. Insertion operator. Logs a Ptree
  std::ostream& operator<<(Ptree* p); // defined in swalker.cc
  //. Creates a new stringstream for use in buffering output
  std::ostringstream& new_stream()
  {
    if (stream) delete stream;
    stream = new std::ostringstream;
    *stream << m_scope << ": ";
    return *stream;
  }
  //. Returns a string derived from the buffer for output
  std::string get_stream_str()
  {
    if (stream) return stream->str();
    return "";
  }
  
private:
  //. Returns a string representing the indent
  std::string indent() { return std::string(slevel, ' '); }
  //. The scope of this STrace object
  std::string m_scope;
  //. The Stack-Level and Display-Level indices
  static int slevel, dlevel;
  //. A StringStream used for buffering output
  static std::ostringstream* stream;
  //. The FIFO queue of skipped enter-messages
  static string_list m_list;
};

//. An exception object indicating errors in translating the Ptree to an AST.
//. Upon invocation, will grab the error message stored by the ERROR macros in
//. the STrace object
class TranslateError : public std::exception
{
public:
  //. The message
  std::string message;
  //. The node that was being translated
  Ptree* node;

  //. Constructor. Extracts the error message from the STracer (set by ERROR macros)
  TranslateError(STrace& trace, Ptree* p = 0)
  : node(p)
  { message = trace.get_stream_str(); trace << "Error: " << message << std::endl; }
  //. Copy constructor
  TranslateError(const TranslateError& e)
  : message(e.message), node(e.node)
  { }
  //. Destructor
  ~TranslateError() throw() {}

  //. overridden std::exception method (not that the original works too well)
  virtual const char* what() const throw () { return "TranslateError"; }
  //. Returns the error message
  std::string str() const { return message; }
  //. Sets a node for the error, if not already set.
  void set_node(Ptree* p) { if (!node) node = p; }
};
#define ERROR(message) (trace.new_stream() << message, TranslateError(trace))
#define nodeERROR(node, message) (trace.new_stream() << message, TranslateError(trace, node))
#define LOG(message) trace << message << std::endl
#define nodeLOG(message) trace << message;


#else // DEBUG

//. Dummy STrace guard for release code - should be optimized away
class STrace
{
public:
  STrace(const std::string &) { }
  ~STrace() { }
};

//. Exception thrown by errors when translating the Ptree into an AST
class TranslateError : public std::exception
{
public:
  char* str() { return ""; }
  virtual const char* what() const throw () { return "TranslateError"; }
  void set_node(Ptree*) { }
};
#define ERROR(message) TranslateError()
#define nodeERROR(node, message) TranslateError()
#define LOG(trash)
#define nodeLOG(trash)

#endif


#endif
