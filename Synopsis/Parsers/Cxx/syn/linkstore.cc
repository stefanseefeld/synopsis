//
// Copyright (C) 2002 Stefan Seefeld
// Copyright (C) 2002 Stephen Davies
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include <Python.h>
#include <Synopsis/Path.hh>

#include "swalker.hh"
#include "linkstore.hh"
#include "ast.hh"
#include "type.hh"
#include "dumper.hh"
#include "builder.hh"
#include "strace.hh"
#include "filter.hh"

#include <sstream>
#include <iomanip>
#include <map>

#include <occ/AST.hh>
#include <occ/Parser.hh>
#include <occ/Buffer.hh>

namespace 
{

//. The field separator
const char* FS = " ";

//. The record separator
const char* RS = "\n";

const char* context_names[] =
{
    "REF",
    "DEF",
    "SPAN",
    "IMPL",
    "UDIR",
    "UDEC",
    "CALL"
};

}

using namespace Synopsis;

struct LinkStore::Private
{
    //. The start of the buffer
    const char* buffer_start;

    //. The filter
    FileFilter* filter;

    //. The Parser object
    Parser* parser;

    //. The SWalker object
    SWalker* walker;

    //. Struct for storing a pair of ofstream pointers
    struct Streams {
        std::ofstream* syntax;
        std::ofstream* xref;
        Streams() : syntax(NULL), xref(0) {}
        Streams(const Streams& o) : syntax(o.syntax), xref(o.xref) {}
        Streams& operator =(const Streams& o)
        {
            syntax = o.syntax;
            xref = o.xref;
            return *this;
        }
    };

    //. The type of a map of streams
    typedef std::map<AST::SourceFile*,Streams> StreamsMap;

    //. A map of streams for each file
    StreamsMap streams;
};

LinkStore::LinkStore(FileFilter* filter, SWalker* swalker)
{
    m = new Private;
    m->filter = filter;
    m->walker = swalker;
    m->buffer_start = swalker->buffer()->Read(0);
    m->parser = swalker->parser();

    // Check size of array here to prevent later segfaults
    if (sizeof(context_names)/sizeof(context_names[0]) != NumContext)
    {
        std::cerr << "FATAL ERROR: Wrong number of context names in linkstore.cc" << std::endl;
        exit(1);
    }
}

LinkStore::~LinkStore()
{
    // Delete (which closes) all the opened file streams
    Private::StreamsMap::iterator iter = m->streams.begin();
    while (iter != m->streams.end())
    {
        Private::Streams& streams = (iter++)->second;
        if (streams.syntax) delete streams.syntax;
        if (streams.xref) delete streams.xref;
    }
    delete m;
}

SWalker* LinkStore::swalker()
{
    return m->walker;
}

int LinkStore::find_col(AST::SourceFile *file, int line, const char* ptr)
{
    const char* pos = ptr;
    while (pos > m->buffer_start && *--pos != '\n')
        ; // do nothing inside loop
    int col = ptr - pos;
    // Resolve macro maps
    return file->macro_calls().map(line, col);
}

void LinkStore::link(Ptree* node, Context context, const ScopedName& name, const std::string& desc, const AST::Declaration* decl)
{
    AST::SourceFile* file = m->walker->current_file();

    // Dont store records for included files
    if (!m->filter->should_link(file))
        return;

    // Get info for storing an xref record
    int line = m->walker->line_of_ptree(node);
    if (decl != 0)
        store_xref_record(file, decl, file->filename(), line, context);

    // Get info for storing a syntax record
    int col = find_col(file, line, node->LeftMost());
    if (col < 0)
        return; // inside macro
    int len = node->RightMost() - node->LeftMost();

    store_syntax_record(file, line, col, len, context, name, desc);
}

//. A class which acts as a Types Visitor to store the correct link to a given
//. type
class TypeStorer : public Types::Visitor
{
    // Variables to pass to link()
    LinkStore* links;
    Ptree* node;
    LinkStore::Context context;
public:
    //. Constructor
    TypeStorer(LinkStore* ls, Ptree* n, LinkStore::Context c)
            : links(ls), node(n), context(c)
    { }

    //. Returns a suitable description for the given type
    std::string describe(Types::Type* type)
    {
        std::string desc;
        try
        {
            return Types::declared_cast<AST::Declaration>(type)->type();
        }
        catch (const Types::wrong_type_cast&)
        {
            return links->swalker()->type_formatter()->format(type);
        }
    }

    // Visitor methods
    void visit_base(Types::Base* base)
    {
        // Not a link, but a keyword
        links->span(node, "file-keyword");
    }
    void visit_dependent(Types::Dependent*)
    {
    }

    void visit_named(Types::Named* named)
    {
        // All other nameds get stored
        links->link(node, context, named->name(), describe(named));
    }
    void visit_declared(Types::Declared* declared)
    {
        // All other nameds get stored
        links->link(node, context, declared->name(), describe(declared), declared->declaration());
    }
    void visit_modifier(Types::Modifier* mod)
    {
        // We recurse on the mod's alias, but dont link the const bit
        if (mod->pre().size() && mod->pre().front() == "const")
            if (!node->IsLeaf() && node->First()->Eq("const"))
            {
                links->span(node->First(), "file-keyword");
                node = node->Last()->First();
            }
        mod->alias()->accept(this);
    }
    void visit_parameterized(Types::Parameterized* param)
    {
        // Sometimes there's a typename at the front..
        if (node->First()->IsLeaf() && node->First()->Eq("typename"))
            node = node->Second();
        // Some modifiers nest the identifier..
        while (!node->First()->IsLeaf())
            node = node->First();
        // For qualified template names the ptree is:
        //  [ std :: [ vector [ < ... , ... > ] ] ]
        // If the name starts with :: (global scope), skip it
        if (node->First() && node->First()->Eq("::"))
            node = node->Rest();
        // Skip the qualifieds (and just link the final name)
        while (node->Second() && node->Second()->Eq("::"))
            if (node->Third()->IsLeaf())
                node = node->Rest()->Rest();
            else
                node = node->Third();
        // Do template
        links->link(node->First(), param->template_type());
        // Do params
        node = node->Second();
        typedef Types::Type::vector::iterator iterator;
        iterator iter = param->parameters().begin();
        iterator end = param->parameters().end();
        // Could be leaf if eg: [SomeId const] node is now "const"
        while (node && !node->IsLeaf() && iter != end)
        {
            // Skip '<' or ','
            if ( !(node = node->Rest()) )
                break;
            if (node->Car() && node->Car()->Car() && !node->Car()->Car()->IsLeaf() && node->Car()->Car()->Car())
                links->link(node->Car()->Car()->Car(), *iter);
            ++iter;
            node = node->Rest();
        }
    }
    // Other types ignored, for now
};

// A class that allows easy encoding of strings
class LinkStore::encode
{
public:
    const char* str;
    encode(const char* s) : str(s)
    { }
    encode(const std::string& s) : str(s.c_str())
    { }
};

// Insertion operator for encode class
std::ostream& operator <<(std::ostream& out, const LinkStore::encode& enc)
{
#ifdef DEBUG
    // Encoding makes it very hard to read.. :)
    return out << enc.str;
#else

    for (const char* s = enc.str; *s; ++s)
        if (isalnum(*s) || *s == '`' || *s == ':')
            out << *s;
        else
            out << '%' << std::hex << std::setw(2) << std::setfill('0') << int(*s) << std::dec;
    return out;
#endif
}


// A class that allows easy encoding of ScopedNames
class LinkStore::encode_name
{
public:
    const ScopedName& name;
    encode_name(const ScopedName& N) : name(N)
    { }
};

// Insertion operator for encode class
std::ostream& operator <<(std::ostream& out, const LinkStore::encode_name& enc)
{
#ifdef DEBUG
    // Encoding makes it very hard to read.. :)
    return out << LinkStore::encode(join(enc.name, "::"));
#else

    return out << LinkStore::encode(join(enc.name, "\t"));
#endif
}

// Store if type is named
void LinkStore::link(Ptree* node, Types::Type* type, Context context)
{
    AST::SourceFile* file = m->walker->current_file();
    if (!type || !m->filter->should_link(file))
        return;
    TypeStorer storer(this, node, context);
    type->accept(&storer);
}

void LinkStore::link(Ptree* node, const AST::Declaration* decl)
{
    AST::SourceFile* file = m->walker->current_file();
    if (!decl || !m->filter->should_link(file))
        return;
    link(node, Definition, decl->name(), decl->type(), decl);
}

void LinkStore::span(int line, int col, int len, const char* desc)
{
    AST::SourceFile* file = m->walker->current_file();
    if (!m->filter->should_link(file))
        return;
    std::ostream& out = get_syntax_stream(file);

    out << line << FS << col << FS << len << FS;
    out << context_names[Span] << FS << encode(desc) << RS;
}

void LinkStore::span(Ptree* node, const char* desc)
{
    int line = m->walker->line_of_ptree(node);
    AST::SourceFile* file = m->walker->current_file();
    if (!m->filter->should_link(file))
        return;
    int col = find_col(file, line, node->LeftMost());
    if (col < 0)
        return; // inside macro
    int len = node->RightMost() - node->LeftMost();

    span(line, col, len, desc);
}

void LinkStore::long_span(Ptree* node, const char* desc)
{
    // Find left edge
    int left_line = m->walker->line_of_ptree(node);
    AST::SourceFile* file = m->walker->current_file();
    if (!m->filter->should_link(file))
        return;
    int left_col = find_col(file, left_line, node->LeftMost());
    if (left_col < 0)
        return; // inside macro
    int len = node->RightMost() - node->LeftMost();

    // Find right edge
    char* fname;
    int fname_len;
    int right_line = m->parser->LineNumber(node->RightMost(), fname, fname_len);

    if (right_line == left_line)
        // Same line, so normal output
        span(left_line, left_col, len, desc);
    else
    {
        // Must output one for each line
        int right_col = find_col(file, right_line, node->RightMost());
        for (int line = left_line; line < right_line; line++, left_col = 0)
            span(line, left_col, -1, desc);
        // Last line is a bit different
        span(right_line, 0, right_col, desc);
    }
}

// Store a link in the Syntax File
void LinkStore::store_syntax_record(AST::SourceFile* file, int line, int col, int len, Context context, const ScopedName& name, const std::string& desc)
{
    std::ostream& out = get_syntax_stream(file);

    // Start the record
    out << line << FS << col << FS << len << FS;
    out << context_names[context] << FS;
    // Tab is used since :: is ambiguous as a scope separator unless real
    // parsing is done, and the syntax highlighter generates links into the docs
    // using the scope info
    out << encode_name(name) << FS;
    // Add the name to the description (REVISIT: This looks too complex!)
    std::vector<AST::Scope*> scopes;
    Types::Named* vtype;
    ScopedName short_name;
    if (m->walker->builder()->mapName(name, scopes, vtype))
    {
        for (size_t i = 0; i < scopes.size(); i++)
        {
            if (AST::Namespace* ns = dynamic_cast<AST::Namespace*>(scopes[i]))
                if (ns->type() == "function")
                {
                    // Restart description at function scope
                    short_name.clear();
                    continue;
                }
            // Add the name to the short name
            short_name.push_back(scopes[i]->name().back());
        }
        // Add the final type name to the short name
        short_name.push_back(vtype->name().back());
    }
    else
    {
        STrace trace("LinkStore::link");
        LOG("WARNING: couldnt map name " << name);
        short_name = name;
    }
    out << encode(desc + " " + join(short_name,"::")) << RS;
}


// Store a link in the CrossRef File
void LinkStore::store_xref_record(
        AST::SourceFile* file, 
        const AST::Declaration* decl, 
        const std::string& filename, int line, Context context)
{
    std::ostream& out = get_xref_stream(file);

    AST::Scope* container = m->walker->builder()->scope();
    std::string container_str = join(container->name(), "\t");
    if (!container_str.size())
        container_str = "\t";
    out << encode_name(decl->name()) << FS << filename << FS << line << FS;
    out << encode(container_str) << FS << context_names[context] << RS;
}

// Gets the ofstream for a syntax file
std::ostream& LinkStore::get_syntax_stream(AST::SourceFile* file)
{
    // Find the stream to use
    Private::Streams& streams = m->streams[file];
    if (!streams.syntax)
    {
        std::string filename = m->filter->get_syntax_filename(file);
        makedirs(Path(filename).dirname());
        streams.syntax = new std::ofstream(filename.c_str());
    }
    return *streams.syntax;
}

// Gets the ofstream for a xref file
std::ostream& LinkStore::get_xref_stream(AST::SourceFile* file)
{
    // Find the stream to use
    Private::Streams& streams = m->streams[file];
    if (!streams.xref)
    {
        std::string filename = m->filter->get_xref_filename(file);
        makedirs(Path(filename).dirname());
        streams.xref = new std::ofstream(filename.c_str());
    }
    return *streams.xref;
}

// vim: set ts=8 sts=4 sw=4 et:
