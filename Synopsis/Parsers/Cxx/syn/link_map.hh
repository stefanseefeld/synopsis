// $Id: link_map.hh,v 1.1 2001/03/16 04:42:00 chalky Exp $
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
// $Log: link_map.hh,v $
// Revision 1.1  2001/03/16 04:42:00  chalky
// SXR parses expressions, handles differences from macro expansions. Some work
// on function call resolution.
//
//

#ifndef H_SYNOPSIS_CPP_LINKMAP
#define H_SYNOPSIS_CPP_LINKMAP

//. link_map is a map from preprocessed file positions to input file positions
class link_map {
public:
    //. Constructor
    link_map();

    //. Returns a reference to a singleton instance of link_map
    static link_map& instance();

    //. Adds a map at the given line. out_{start,end} define the start and
    //. past-the-end markers for the macro in the output file. diff defines
    //. the signed difference to add to the pos.
    //. @see map(int, int)
    void add(const char* name, int line, int out_start, int out_end, int diff);

    //. Applies added maps to the given column number. The differentials of
    //. the various macros are applied in turn, and the final column position is
    //. returned. Returns -1 if col is inside a macro
    int map(int line, int col);

private:
    //. Compiler firewall
    struct Private;
    Private *m;
};

#endif
