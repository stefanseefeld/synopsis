// Synopsis C++ Parser: LinkMap.cc source file
// Implementation of the LinkMap class

// $Id: link_map.cc,v 1.6 2003/01/27 06:53:37 chalky Exp $
//
// This file is a part of Synopsis.
// Copyright (C) 2000-2002 Stephen Davies
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

// $Log: link_map.cc,v $
// Revision 1.6  2003/01/27 06:53:37  chalky
// Added macro support for C++.
//
// Revision 1.5  2002/12/12 17:25:34  chalky
// Implemented Include support for C++ parser. A few other minor fixes.
//
// Revision 1.4  2002/11/17 12:11:43  chalky
// Reformatted all files with astyle --style=ansi, renamed fakegc.hh
//
// Revision 1.3  2002/10/25 08:57:47  chalky
// Clear the link map (stores where expanded macros are for syntax highlighted
// files) after parsing
//
// Revision 1.2  2001/05/06 20:15:03  stefan
// fixes to get std compliant; replaced some pass-by-value by pass-by-const-ref; bug fixes;
//
// Revision 1.1  2001/03/16 04:42:00  chalky
// SXR parses expressions, handles differences from macro expansions. Some work
// on function call resolution.
//


#include "link_map.hh"
#include "filter.hh"
#include "ast.hh"

#include <map>
#include <set>
#include <string>

//. This vector is initialised on first use (if any) by synopsis_define_hook
std::vector<AST::Macro*>* syn_macro_defines = NULL;

namespace
{
struct Node
{
    int start;
    int end;
    enum Type { START, END } type;
    int diff;
    bool operator <(const Node& other) const
    {
        return start < other.start;
    }
};

typedef std::set<Node> Line;
typedef std::map<int, Line> LineMap;
};

// ------------------------------------------
// Private struct
//
struct LinkMap::Private
{
    LineMap lines;
};

LinkMap::LinkMap()
{
    m = new Private;
}

LinkMap* LinkMap::instance()
{
    static LinkMap* inst = 0;
    if (!inst)
        inst = new LinkMap;
    return inst;
}

void LinkMap::add(const char* name, int linenum, int start, int end, int diff)
{
    Line& line = m->lines[linenum];
    Node node;
    node.start = start;
    node.end = end;
    node.type = Node::START;
    node.diff = diff;
    line.insert(node);
}

int LinkMap::map(int linenum, int pos)
{
    LineMap::iterator line_iter = m->lines.find(linenum);
    if (line_iter == m->lines.end())
    {
        return pos;
    }
    Line& line = line_iter->second;
    Line::iterator iter = line.begin(), end = line.end();
    int diff = 0;
    while (iter != end && iter->start < pos)
    {
        const Node& node = *iter++;
        if (pos < node.end)
            return -1;
        //cout << "LinkMap::map: line: "<<linenum<<" pos: "<<pos<<" start: "<<node.start<<endl;
        diff = node.diff;
    }
    //cout << "LinkMap::map: line: "<<linenum<<" pos: "<<pos<<" diff: "<<diff<<endl;
    return pos + diff;
}

void LinkMap::clear()
{
    m->lines.clear();
}

extern "C"
{
    //. This function is a callback from the ucpp code to store macro
    //. expansions
    void synopsis_macro_hook(const char* name, int line, int start, int end, int diff)
    {
        LinkMap::instance()->add(name, line, start, end, diff);
    }

    //. This function is a callback from the ucpp code to store includes
    void synopsis_include_hook(const char* source_file, const char* target_file, int is_macro, int is_next)
    {
	// There is not enough code here to make another class..
	FileFilter* filter = FileFilter::instance();
	if (!filter)
	    return;

	// Add another Include to the source's SourceFile
	// We don't deal with duplicates here. The Linker's Unduplicator will
	// take care of that.
	//std::cout << "Include: " << source_file << " -> " << target_file << std::endl;
	AST::SourceFile* file = filter->get_sourcefile(source_file);
	AST::SourceFile* target = filter->get_sourcefile(target_file);
	AST::Include* include = new AST::Include(target, is_macro, is_next);
	file->includes().push_back(include);
    }

    //. This function is a callback from the ucpp code to store macro
    //. definitions
    void synopsis_define_hook(const char* filename, int line, const char* name, int num_args, const char** args, int vaarg, const char* text)
    {
	FileFilter* filter = FileFilter::instance();
	if (!filter)
	    return;
	AST::SourceFile* file = filter->get_sourcefile(filename);
	if (!file->is_main())
	    return;
	if (syn_macro_defines == NULL)
	    syn_macro_defines = new std::vector<AST::Macro*>;
	AST::Macro::Parameters* params = NULL;
	if (args != NULL)
	{
	    params = new AST::Macro::Parameters;
	    for (int i = 0; i < num_args; i++)
		params->push_back(args[i]);
	    if (vaarg)
		params->push_back("...");
	}
	ScopedName macro_name;
	macro_name.push_back(name);
	AST::Macro* macro = new AST::Macro(file, line, macro_name, params, text);
	//. Don't know global NS yet.. Will have to extract these later
	file->declarations().push_back(macro);
	syn_macro_defines->push_back(macro);
    }
};
// vim: set ts=8 sts=4 sw=4 et:
