// vim: set ts=8 sts=2 sw=2 et:
// $Id: linkstore.cc,v 1.16 2002/11/03 05:41:30 chalky Exp $
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
// $Log: linkstore.cc,v $
// Revision 1.16  2002/11/03 05:41:30  chalky
// Fix crash in visit_parameterized
//
// Revision 1.15  2002/11/02 06:37:37  chalky
// Allow non-frames output, some refactoring of page layout, new modules.
//
// Revision 1.14  2002/10/29 02:39:57  chalky
// Changes to compile with g++-3.2
//
// Revision 1.13  2002/10/28 16:25:02  chalky
// Fix crash if using xref output but not syntax output
//
// Revision 1.12  2002/10/20 15:38:10  chalky
// Much improved template support, including Function Templates.
//
// Revision 1.11  2002/10/11 05:58:21  chalky
// Better memory management. Better comment proximity detection.
//
// Revision 1.10  2002/01/30 11:53:15  chalky
// Couple bug fixes, some cleaning up.
//
// Revision 1.9  2002/01/29 09:42:09  chalky
// Tabify xref scopes
//
// Revision 1.8  2002/01/28 13:17:24  chalky
// More cleaning up of code. Combined xref into LinkStore. Encoded links file.
//
// Revision 1.7  2002/01/25 14:24:33  chalky
// Start of refactoring and restyling effort.
//
// Revision 1.6  2001/07/28 02:40:02  chalky
// Updates for new source layout
//
// Revision 1.5  2001/07/23 15:29:35  chalky
// Fixed some regressions and other mis-features
//
// Revision 1.4  2001/07/23 11:51:22  chalky
// Better support for name lookup wrt namespaces.
//
// Revision 1.3  2001/06/13 14:04:37  chalky
// s/std::cout/LOG/ or so
//
// Revision 1.2  2001/06/10 07:17:37  chalky
// Comment fix, better functions, linking etc. Better link titles
//
// Revision 1.1  2001/06/10 00:31:39  chalky
// Refactored link storage, better comments, better parsing
//
//

#include "swalker.hh"
#include "linkstore.hh"
#include "ast.hh"
#include "type.hh"
#include "link_map.hh"
#include "dumper.hh"
#include "builder.hh"
#include "strace.hh"

#include <sstream>
#include <iomanip>

#include <occ/ptree.h>
#include <occ/parse.h>
#include <occ/buffer.h>


const char* LinkStore::m_context_names[] = {
  "REF",
  "DEF",
  "SPAN",
  "IMPL",
  "UDIR",
  "UDEC",
  "CALL"
};

LinkStore::LinkStore(std::ostream* syntax_stream, std::ostream* xref_stream, SWalker* swalker, const std::string& basename)
: m_syntax_stream(syntax_stream),
  m_xref_stream(xref_stream),
  m_walker(swalker),
  m_basename(basename)
{
  m_buffer_start = swalker->program()->Read(0);
  m_parser = swalker->parser();

  // Check size of array here to prevent later segfaults
  if (sizeof(m_context_names)/sizeof(m_context_names[0]) != NumContext)
    {
      std::cerr << "FATAL ERROR: Wrong number of context names in linkstore.cc" << std::endl;
      exit(1);
    }
}

SWalker* LinkStore::swalker()
{
  return m_walker;
}

int LinkStore::find_col(int line, const char* ptr)
{
  const char* pos = ptr;
  while (pos > m_buffer_start && *--pos != '\n')
    ; // do nothing inside loop
  int col = ptr - pos;
  // Resolve macro maps
  return link_map::instance().map(line, col);
}

void LinkStore::link(Ptree* node, Context context, const ScopedName& name, const std::string& desc, const AST::Declaration* decl)
{
  // Dont store records for included files
  if (!m_walker->is_main_file()) return;

  // Get info for storing an xref record
  int line = m_walker->line_of_ptree(node);
  if (decl != NULL)
    store_xref_record(decl, m_walker->current_file(), line, context);

  // Get info for storing a syntax record
  int col = find_col(line, node->LeftMost());
  if (col < 0) return; // inside macro
  int len = node->RightMost() - node->LeftMost();
  
  store_syntax_record(line, col, len, context, name, desc);
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
      { return Types::declared_cast<AST::Declaration>(type)->type(); }
    catch (const Types::wrong_type_cast&)
      { return links->swalker()->type_formatter()->format(type); }
  }

  // Visitor methods
  void visit_base(Types::Base* base)
  {
    // Not a link, but a keyword
    links->span(node, "file-keyword");
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
      if ( !(node = node->Rest()) ) break;
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
  encode(const char* s) : str(s) { }
  encode(const std::string& s) : str(s.c_str()) { }
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
  encode_name(const ScopedName& N) : name(N) { }
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
  if (!m_walker->is_main_file() || !type) return;
  TypeStorer storer(this, node, context);
  type->accept(&storer);
}

void LinkStore::link(Ptree* node, const AST::Declaration* decl)
{
  if (!m_walker->is_main_file() || !decl) return;
  link(node, Definition, decl->name(), decl->type(), decl);
}

void LinkStore::span(int line, int col, int len, const char* desc)
{
  if (!m_syntax_stream) return;
  std::ostream& out = *m_syntax_stream;
  out << line << FS << col << FS << len << FS;
  out << m_context_names[Span] << FS << encode(desc) << RS;
}

void LinkStore::span(Ptree* node, const char* desc)
{
  int line = m_walker->line_of_ptree(node);
  if (!m_walker->is_main_file()) return;
  int col = find_col(line, node->LeftMost());
  if (col < 0) return; // inside macro
  int len = node->RightMost() - node->LeftMost();
  
  span(line, col, len, desc);
}

void LinkStore::long_span(Ptree* node, const char* desc)
{
  // Find left edge
  int left_line = m_walker->line_of_ptree(node);
  if (!m_walker->is_main_file()) return;
  int left_col = find_col(left_line, node->LeftMost());
  if (left_col < 0) return; // inside macro
  int len = node->RightMost() - node->LeftMost();
  
  // Find right edge
  char* fname; int fname_len;
  int right_line = m_parser->LineNumber(node->RightMost(), fname, fname_len);
  
  if (right_line == left_line)
    // Same line, so normal output
    span(left_line, left_col, len, desc);
  else
    {
      // Must output one for each line
      int right_col = find_col(right_line, node->RightMost());
      for (int line = left_line; line < right_line; line++, left_col = 0)
        span(line, left_col, -1, desc);
      // Last line is a bit different
      span(right_line, 0, right_col, desc);
    }
}

// Store a link in the Syntax File
void LinkStore::store_syntax_record(int line, int col, int len, Context context, const ScopedName& name, const std::string& desc)
{
  if (!m_syntax_stream)
    return;
  std::ostream& out = *m_syntax_stream;
  out << line << FS << col << FS << len << FS;
  out << m_context_names[context] << FS;
  // Tab is used since :: is ambiguous as a scope separator unless real
  // parsing is done, and the syntax highlighter generates links into the docs
  // using the scope info
  out << encode_name(name) << FS;
  // Add the name to the description (REVISIT: This looks too complex!)
  std::vector<AST::Scope*> scopes;
  Types::Named* vtype;
  ScopedName short_name;
  if (m_walker->builder()->mapName(name, scopes, vtype))
    {
      for (size_t i = 0; i < scopes.size(); i++)
        {
          if (AST::Namespace* ns = dynamic_cast<AST::Namespace*>(scopes[i]))
            if (ns->type() == "function") {
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
void LinkStore::store_xref_record(const AST::Declaration* decl, const std::string& file, int line, Context context)
{
  if (!m_xref_stream)
    return;
  // Strip the basename from the filename
  std::string filename = file;
  if (filename.substr(0, m_basename.size()) == m_basename)
    filename.assign(file, m_basename.size(), std::string::npos);
  AST::Scope* container = m_walker->builder()->scope();
  //m->refs[decl->name()].push_back(AST::Reference(file, line, container->name(), context));
  std::string container_str = join(container->name(), "\t");
  if (!container_str.size()) container_str = "\t";
  (*m_xref_stream) << encode_name(decl->name()) << FS << filename << FS << line << FS
    << encode(container_str) << FS << m_context_names[context] << RS;
}
