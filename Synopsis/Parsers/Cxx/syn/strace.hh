// $Id: strace.hh,v 1.3 2001/05/23 05:08:47 stefan Exp $
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
#include <strstream>
#include <exception>
#include <list>

class Ptree;

#if 1 && defined(DEBUG)
#define DO_TRACE
class STrace
{
    typedef std::list<std::string> list;
public:
    STrace(const std::string &s) : scope(s) {
	m_list.push_back(indent() + "entering " + scope);
	++slevel;
    }
    ~STrace() {
	if (dlevel > --slevel) {
	    // 'enter' message was displayed, so display leave message too
	    std::cout << indent() << "leaving  " << scope << std::endl;
	    --dlevel;
	} else
	    // 'enter' message wasn't displayed, so remove it from list
	    m_list.pop_back();
    }
    std::ostream& operator <<(std::string s) {
	while (dlevel < slevel) {
	    std::cout << m_list.front() << "\n";
	    m_list.pop_front();
	    ++dlevel;
	}
	std::cout << indent() << s; return std::cout;
    }
    std::string scope;
    std::ostrstream& new_stream() {
	if (stream) delete stream;
	stream = new std::ostrstream;
	*stream << scope << ": ";
	return *stream;
    }
    std::string get_stream_str() {
	if (stream) return std::string(stream->str(), stream->pcount());
	return "";
    }
    
private:
    std::string indent() { return std::string(slevel, ' '); }
    static int slevel, dlevel;
    static std::ostrstream* stream;
    static list m_list;
};
class TranslateError : public std::exception {
public:
    std::string message;
    Ptree* node;
    TranslateError(STrace& trace, Ptree* p = 0) : node(p)
	{ message = trace.get_stream_str(); trace << "Error: " << message << std::endl; }
    ~TranslateError() throw() {}
    virtual const char* what() const throw () { return "TranslateError"; }
    std::string str() { return message; }
    void set_node(Ptree* p) { if (!node) node = p; }

};
#define ERROR(message) (trace.new_stream() << message, TranslateError(trace))
#define nodeERROR(node, message) (trace.new_stream() << message, TranslateError(trace, node))
#define LOG(message) trace << message << std::endl
#else
class STrace
{
public:
    STrace(const std::string &) {}
    ~STrace() {}
};
class TranslateError : public std::exception {
public:
    char* str() { return ""; }
    virtual const char* what() const throw () { return "TranslateError"; }
    void set_node(Ptree*) {}
};
#define ERROR(message) TranslateError()
#define nodeERROR(node, message) TranslateError()
#define LOG(trash)
#endif


#endif
