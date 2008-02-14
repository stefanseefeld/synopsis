// Synopsis C++ Parser: strace.hh header file
// Defines the STrace class which can be used to provide detailed traces in
// debug mode

// $Id: strace.hh,v 1.9 2002/11/17 12:11:44 chalky Exp $
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

#ifndef H_SYNOPSIS_CPP_STRACE
#define H_SYNOPSIS_CPP_STRACE

#include <string>
#include <iostream>
#include <sstream>
#include <exception>
#include <list>

namespace Synopsis
{
namespace PTree { class Node;}
}
#if defined(DEBUG)
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
  std::ostream& operator<<(Synopsis::PTree::Node *p); // defined in swalker.cc
    //. Creates a new stringstream for use in buffering output
    std::ostringstream& new_stream()
    {
        if (stream)
            delete stream;
        stream = new std::ostringstream;
        *stream << m_scope << ": ";
        return *stream;
    }
    //. Returns a string derived from the buffer for output
    std::string get_stream_str()
    {
        if (stream)
            return stream->str();
        return "";
    }

private:
    //. Returns a string representing the indent
    std::string indent()
    {
        return std::string(slevel, ' ');
    }
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
  mutable Synopsis::PTree::Node *node;

    //. Constructor. Extracts the error message from the STracer (set by ERROR macros)
  TranslateError(STrace& trace, Synopsis::PTree::Node *p = 0)
            : node(p)
    {
        message = trace.get_stream_str();
        trace << "Error: " << message << std::endl;
    }
    //. Copy constructor
    TranslateError(const TranslateError& e)
            : message(e.message), node(e.node)
    { }
    //. Destructor
    ~TranslateError() throw()
    {}

    //. overridden std::exception method (not that the original works too well)
    virtual const char* what() const throw ()
    {
        return "TranslateError";
    }
    //. Returns the error message
    std::string str() const
    {
        return message;
    }
    //. Sets a node for the error, if not already set.
  void set_node(Synopsis::PTree::Node *p) const
    {
        if (!node)
            node = p;
    }
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
    STrace(const std::string &)
    { }
    ~STrace()
    { }
};

//. Exception thrown by errors when translating the Ptree into an AST
class TranslateError : public std::exception
{
public:
    char const * str()
    {
        return "";
    }
    virtual const char* what() const throw ()
    {
        return "TranslateError";
    }
  void set_node(Synopsis::PTree::Node *) const
    { }
};
#define ERROR(message) TranslateError()
#define nodeERROR(node, message) TranslateError()
#define LOG(trash)
#define nodeLOG(trash)

#endif


#endif
// vim: set ts=8 sts=4 sw=4 et:
