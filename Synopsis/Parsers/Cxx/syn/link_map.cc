// $Id: link_map.cc,v 1.1 2001/03/16 04:42:00 chalky Exp $
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
// $Log: link_map.cc,v $
// Revision 1.1  2001/03/16 04:42:00  chalky
// SXR parses expressions, handles differences from macro expansions. Some work
// on function call resolution.
//
//

#include "link_map.hh"

#include <map>
#include <set>
#include <string>

using namespace std;

namespace {
    struct Node {
	int start;
	int end;
	enum Type { START, END } type;
	int diff;
	bool operator <(const Node& other) const {
	    return start < other.start;
	}
    };
	
    typedef set<Node> Line;
    typedef map<int, Line> LineMap;
};

// ------------------------------------------
// Private struct
//
struct link_map::Private {
    LineMap lines;
};

link_map::link_map()
{
    m = new Private;
}

link_map& link_map::instance()
{
    static link_map* inst = NULL;
    if (!inst) inst = new link_map;
    return *inst;
}

void link_map::add(const char* name, int linenum, int start, int end, int diff)
{
    Line& line = m->lines[linenum];
    Node node;
    node.start = start;
    node.end = end;
    node.type = Node::START;
    node.diff = diff;
    line.insert(node);
}

int link_map::map(int linenum, int pos)
{
    LineMap::iterator line_iter = m->lines.find(linenum);
    if (line_iter == m->lines.end()) {
	return pos;
    }
    Line& line = line_iter->second;
    Line::iterator iter = line.begin(), end = line.end();
    int diff = 0;
    while (iter != end && iter->start < pos) {
	const Node& node = *iter++;
	if (pos < node.end) return -1;
	//cout << "link_map::map: line: "<<linenum<<" pos: "<<pos<<" start: "<<node.start<<endl;
	diff = node.diff;
    }
    //cout << "link_map::map: line: "<<linenum<<" pos: "<<pos<<" diff: "<<diff<<endl;
    return pos + diff;
}

extern "C" {
    //. This function is a callback from the ucpp code
    void synopsis_macro_hook(const char* name, int line, int start, int end, int diff)
    {
	link_map::instance().add(name, line, start, end, diff);
    }
};
