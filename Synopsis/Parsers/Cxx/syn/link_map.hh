// Synopsis C++ Parser: link_map.hh header file
// The LinkMap class which maps preprocessed file positions to input file
// positions

// $Id: link_map.hh,v 1.4 2002/12/12 17:25:34 chalky Exp $
//
// This file is a part of Synopsis.
// Copyright (C) 2002 Stephen Davies
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


#ifndef H_SYNOPSIS_CPP_LINKMAP
#define H_SYNOPSIS_CPP_LINKMAP

//. LinkMap is a map from preprocessed file positions to input file positions
class LinkMap
{
public:
    //. Constructor
    LinkMap();

    //. Returns a reference to a singleton instance of link_map
    static LinkMap* instance();

    //. Adds a map at the given line. out_{start,end} define the start and
    //. past-the-end markers for the macro in the output file. diff defines
    //. the signed difference to add to the pos.
    //. @see map(int, int)
    void add(const char* name, int line, int out_start, int out_end, int diff);

    //. Applies added maps to the given column number. The differentials of
    //. the various macros are applied in turn, and the final column position is
    //. returned. Returns -1 if col is inside a macro
    int map(int line, int col);

    //. Clears the map. Should be called at the very start of processing,
    //. since the map is stored statically and hence potentially across parser
    //. invocations.
    void clear();

private:
    //. Compiler firewall
    struct Private;
    Private *m;
};

#endif
