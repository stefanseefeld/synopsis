// vim: set ts=8 sts=2 sw=2 et:
// $Id: swalker.cc,v 1.62 2002/10/25 05:13:57 chalky Exp $
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
// $Log: swalker.cc,v $
// Revision 1.62  2002/10/25 05:13:57  chalky
// Include the 'operator' in operator names
//
// Revision 1.61  2002/10/25 03:43:37  chalky
// Close templates when there's an exception
//
// Revision 1.60  2002/10/25 02:49:51  chalky
// Support templated forward class declarations
//
// Revision 1.59  2002/10/20 16:49:26  chalky
// Fix bug with methods in templates
//
// Revision 1.58  2002/10/20 15:38:10  chalky
// Much improved template support, including Function Templates.
//
// Revision 1.57  2002/10/11 07:37:29  chalky
// Fixed problem with comments and setting a basename
//
// Revision 1.56  2002/10/11 05:58:21  chalky
// Better memory management. Better comment proximity detection.
//
// Revision 1.55  2002/09/20 09:51:13  chalky
// Don't keep comments originating from different file than declaration
// Work around bug in g++ 3.2 ? (*iter++ thing)
//
// Revision 1.54  2002/08/23 08:30:08  chalky
// Add ability to parse typeid constructs, for boost.
//
// Revision 1.53  2002/04/26 01:21:14  chalky
// Bugs and cleanups
//
// Revision 1.52  2002/03/07 14:12:44  chalky
// Better parsing of complex sources like boost
//
// Revision 1.51  2002/03/07 04:43:54  chalky
// Preliminary template specialization support.
//
// Revision 1.50  2002/02/13 11:17:21  chalky
// Slightly better ambiguous function resolution
//
// Revision 1.49  2002/01/30 11:53:15  chalky
// Couple bug fixes, some cleaning up.
//
// Revision 1.48  2002/01/28 13:17:24  chalky
// More cleaning up of code. Combined xref into LinkStore. Encoded links file.
//
// Revision 1.47  2002/01/25 14:24:33  chalky
// Start of refactoring and restyling effort.
//
// Revision 1.46  2002/01/13 09:32:48  chalky
// Small change to mark template classes as "template class" rather than "class"
//
// Revision 1.45  2001/08/09 00:56:50  chalky
// Moved char*<0 ugliness to Decoder::isName with proper casting
//
// File: swalker.cc
//
// SWalker class

#include <iostream>
#include <string>
#include <typeinfo>
#include <strstream>
#include <algorithm>

#include <occ/ptree.h>
#include <occ/parse.h>

#include "swalker.hh"
#include "strace.hh"
#include "type.hh"
#include "ast.hh"
#include "builder.hh"
#include "decoder.hh"
#include "dumper.hh"
#include "link_map.hh"
#include "linkstore.hh"
#include "lookup.hh"

using namespace AST;

// ------------------------------------------------------------------
//                 -- CLASS STrace --
// ------------------------------------------------------------------

#ifdef DO_TRACE
// Class variable definitions
int STrace::slevel = 0, STrace::dlevel = 0;
std::ostringstream* STrace::stream = 0;
STrace::string_list STrace::m_list;
std::ostream& STrace::operator <<(Ptree* p)
{
  std::ostream& out = operator <<("-");
  p->Display2(out);
  return out;
}
#endif

// ------------------------------------------------------------------
//                 -- CLASS SWalker --
// ------------------------------------------------------------------

SWalker *SWalker::g_swalker = 0;

// ------------------------------------
// SWalker Constructor
SWalker::SWalker(const std::string &source, Parser* parser, Builder* builder, Program* program, const std::string& basename)
: Walker(parser),
  m_parser(parser),
  m_program(program),
  m_builder(builder),
  m_decoder(new Decoder(m_builder)),
  m_declaration(0),
  m_template(0),
  m_filename_ptr(0),
  m_lineno(0),
  m_source(source),
  m_basename(basename),
  m_extract_tails(false),
  m_links(0),
  m_store_decl(false),
  m_type_formatter(new TypeFormatter()),
  m_operation(0),
  m_type(0),
  m_scope(0),
  m_postfix_flag(Postfix_Var)
{
  g_swalker = this; // FIXME: is this needed?
  m_builder->set_swalker(this);
  m_lookup = m_builder->lookup();
}

// Destructor
SWalker::~SWalker()
{
  delete m_decoder;
  delete m_type_formatter;
  if (m_links)
    delete m_links;
}

// The name returned is just the node's text if the node is a leaf. Otherwise,
// the ToString method of Ptree is used, which is rather expensive since it
// creates a temporary write buffer and reifies the node tree into it.
std::string
SWalker::parse_name(Ptree *node) const
{
    // STrace trace("SWalker::parse_name");
    if (node && node->IsLeaf())
        return std::string(node->GetPosition(), node->GetLength());
    return node->ToString();
}
void
SWalker::set_store_links(bool value, std::ostream* storage, std::ostream* xref)
{
  // Create a new LinkStore
  if (value)
    m_links = new LinkStore(storage, xref, this);
  else
    m_links = 0;
}

int SWalker::line_of_ptree(Ptree* node)
{
  update_line_number(node);
  return m_lineno;
}

// Updates the line number stored in this SWalker instance, and the filename
// stored in the Builder instance at m_builder. The filename is only set if
// the actual char* changed (which only happens when the preprocessor places
// another #line directive)
void SWalker::update_line_number(Ptree* ptree)
{
  // Ask the Parser for the linenumber of the ptree. This used to be
  // expensive until I hacked buffer.cc to cache the last line number found.
  // Now it's okay as long as you are looking for lines sequentially.
  char* fname;
  int fname_len;
  m_lineno = m_parser->LineNumber(ptree->LeftMost(), fname, fname_len);
  if (fname != m_filename_ptr)
    {
      m_filename_ptr = fname;
      m_filename.assign(fname, fname_len);
      m_builder->set_filename(m_filename);
    }
}

AST::Comment* make_Comment(const std::string& file, int line, Ptree* first, bool suspect=false)
{
  return new AST::Comment(file, line, first->ToString(), suspect);
}
Leaf* make_Leaf(char* pos, int len)
{
  return new Leaf(pos, len);
}
// Adds the given comments to the given declaration. If m_links is set,
// then syntax highlighting information is also stored.
void
SWalker::add_comments(AST::Declaration* decl, Ptree* node)
{
  if (node == NULL)
    return;

  // First, make sure that node is a list of comments
  if (node->What() == ntDeclaration)
    node = static_cast<PtreeDeclaration*>(node)->GetComments();

  // Loop over all comments in the list
  for (Ptree* next = node->Rest(); node && !node->IsLeaf(); next = node->Rest())
  {
    Ptree* first = node->First();
    if (!first || !first->IsLeaf())
    {
      node = next;
      continue;
    }

    update_line_number(node);
    if (decl)
    {
      size_t bs = m_basename.size(); 
      bool wrong_file;
      // Must take care of the basename, since decl->filename wont have it
      // NB: g++ < 3.0 doesn't support the 5 argument string.compare
      if (bs && strncmp(m_filename_ptr, m_basename.c_str(), bs) == 0)
        // This works since m_filename_ptr still points to comment's filename
        wrong_file = (strncmp(m_filename_ptr + bs, decl->filename().c_str(), decl->filename().size()) != 0);
      else
        wrong_file = (m_filename != decl->filename());
      if (wrong_file)
      {
        node = next;
        continue;
      }
    }

    // Check if comment is continued, eg: consecutive C++ comments
    while (next && next->First() && next->First()->IsLeaf())
    {
      if (strncmp(next->First()->GetPosition(), "//", 2))
        break;
      char* next_pos = next->First()->GetPosition();
      char* start_pos = node->First()->GetPosition();
      char* curr_pos = start_pos + node->First()->GetLength();
      // Must only be whitespace between current comment and next
      // and only one newline
      int newlines = 0;
      while (curr_pos < next_pos && strchr(" \t\r\n", *curr_pos))
        if (*curr_pos == '\n' && newlines > 0)
          break;
        else if (*curr_pos++ == '\n')
          ++newlines;
      if (curr_pos < next_pos)
        break;
      // Current comment stretches to end of next
      int len = int(next_pos - start_pos + next->First()->GetLength());
      //node->SetCar(first = new Leaf(start_pos, len));
      node->SetCar(first = make_Leaf(start_pos, len));
      // Skip the combined comment
      next = next->Rest();
    }

    // Ensure that there is no more than one newline between the comment and
    // the declaration. We assume that the declaration is the next
    // non-comment thing (which could be a bad assumption..)
    // If extract_tails is set, then comments separated by a space are still
    // included, but are marked as suspect for the Linker to deal with
    bool suspect = false;
    char* pos = first->GetPosition() + first->GetLength();
    while (*pos && strchr(" \t\r", *pos))
      ++pos;
    if (*pos == '\n')
    {
      ++pos;
      // Found only allowed \n
      while (*pos && strchr(" \t\r", *pos))
        ++pos;
      if (*pos == '\n' || !strncmp(pos, "/*", 2))
      {
        // 1. More than one newline.. skip entire comment and move onto next.
        // 2. This comment is followed by a /*, so ignore this one
        // If extract_tails is set, we keep it anyway but mark as suspect
        if (!m_extract_tails)
        {
          node = next;
          continue;
        }
        else
          suspect = true;
      }
    }

    if (decl)
    {
      //AST::Comment* comment = new AST::Comment("", 0, first->ToString(), suspect);
      AST::Comment* comment = make_Comment("", 0, first, suspect);
      decl->comments().push_back(comment);
    }
    if (m_links) m_links->long_span(first, "file-comment");
    // Set first to nil so we dont accidentally do them twice (eg:
    // when parsing expressions)
    node->SetCar(nil);
    node = next;
  }
}

// -- These methods implement add_comments for various node types that store
// comment pointers
void SWalker::add_comments(AST::Declaration* decl, CommentedLeaf* node)
{
  if (node) add_comments(decl, node->GetComments());
}
void SWalker::add_comments(AST::Declaration* decl, PtreeDeclaration* node)
{
  if (node) add_comments(decl, node->GetComments());
}
void SWalker::add_comments(AST::Declaration* decl, PtreeDeclarator* node)
{
  if (node) add_comments(decl, node->GetComments());
}
void SWalker::add_comments(AST::Declaration* decl, PtreeNamespaceSpec* node)
{
  if (node) add_comments(decl, node->GetComments());
}
void SWalker::find_comments(Ptree* node)
{
  Ptree* leaf, *parent;
  leaf = FindLeftLeaf(node, parent);
  if (leaf) add_comments(NULL, dynamic_cast<CommentedLeaf*>(leaf));
}

Ptree* SWalker::TranslateArgDeclList(bool, Ptree*, Ptree*) { STrace trace("SWalker::TranslateArgDeclList NYI"); return 0; }
Ptree* SWalker::TranslateInitializeArgs(PtreeDeclarator*, Ptree*) { STrace trace("SWalker::TranslateInitializeArgs NYI"); return 0; }
Ptree* SWalker::TranslateAssignInitializer(PtreeDeclarator*, Ptree*) { STrace trace("SWalker::TranslateAssignInitializer NYI"); return 0; }
//Class* SWalker::MakeClassMetaobject(Ptree*, Ptree*, Ptree*) { STrace trace("SWalker::MakeClassMetaobject NYI"); return 0; }
//Ptree* SWalker::TranslateClassSpec(Ptree*, Ptree*, Ptree*, Class*) { STrace trace("SWalker::TranslateClassSpec NYI"); return 0; }
//Ptree* SWalker::TranslateClassBody(Ptree*, Ptree*, Class*) { STrace trace("SWalker::TranslateClassBody NYI"); return 0; }
//Ptree* SWalker::TranslateTemplateInstantiation(Ptree*, Ptree*, Ptree*, Class*) { STrace trace("SWalker::TranslateTemplateInstantiation NYI"); return 0; }

// Format the given parameters. m_type_formatter is used to format the given
// list of parameters into a string, suitable for use as the name of a
// Function object.
std::string SWalker::format_parameters(AST::Parameter::vector& params)
{
  // TODO: Tell formatter to expand typedefs! Eg: this function uses a typedef
  // in implementation, but not for declaration in the class!!!!!!
  AST::Parameter::vector::iterator iter = params.begin(), end = params.end();
  if (iter == end)
    return "()";
  // Set scope for formatter
  AST::Scope* scope = m_builder->scope();
  if (scope)
    m_type_formatter->push_scope(scope->name());
  else
    {
      ScopedName empty;
      m_type_formatter->push_scope(empty);
    }
  // Format the parameters one at a time
  std::ostringstream buf;
  buf << "(" << m_type_formatter->format((*iter++)->type());
  while (iter != end)
    buf << "," << m_type_formatter->format((*iter++)->type());
  buf << ")";
  m_type_formatter->pop_scope();
  return buf.str();
}




//
// Translate Methods
//

// Overrides Walker::Translate to catch any exceptions
void
SWalker::Translate(Ptree* node)
{
  STrace trace("SWalker::Translate");
  try
    { Walker::Translate(node); }
  // Debug and non-debug modes handle these very differently
#ifdef DEBUG
  catch (const TranslateError& e)
    {
      if (e.node) node = e.node;
      char* fname;
      int fname_len;
      int lineno = m_parser->LineNumber(node->LeftMost(), fname, fname_len);
      std::ostringstream buf;
      buf << " (" << std::string(fname, fname_len) << ":" << lineno << ")";
      LOG("Warning: An exception occurred:" << buf.str());
      LOG("- " << e.str());
    }
  catch (const std::exception& e)
    {
      LOG("Warning: An exception occurred: " << e.what());
      nodeLOG(node);
    }
  catch (...)
    {
      LOG("Warning: An exception occurred (unknown) at:");
      nodeLOG(node);
    }
#else
  catch (const TranslateError& e)
    {
      // This error usually means that the syntax highlighting failed, and
      // can be safely ignored
    }
  catch (const std::exception& e)
    {
      std::cout << "Warning: An exception occurred: " << e.what() << std::endl;
      std::cout << "At: ";
      char* fname; int fname_len;
      int lineno = m_parser->LineNumber(node->LeftMost(), fname, fname_len);
      std::cout << " (" << std::string(fname, fname_len) << ":" << lineno << ")" << std::endl;
    }
  catch (...)
    {
      std::cout << "Warning: An unknown exception occurred: " << std::endl;
      std::cout << "At: ";
      char* fname; int fname_len;
      int lineno = m_parser->LineNumber(node->LeftMost(), fname, fname_len);
      std::cout << " (" << std::string(fname, fname_len) << ":" << lineno << ")" << std::endl;
    }
#endif
}

// Default translate, usually means a literal
Ptree* SWalker::TranslatePtree(Ptree* node)
{
  // Determine type of node
  char* str = node->ToString();
  if (*str >= '0' && *str <= '9' || *str == '.')
    {
      // Assume whole node is a number
      if (m_links) m_links->span(node, "file-number");
      // TODO: decide if Long, Float, Double, etc
      const char* num_type = (*str == '.' ? "double" : "int");
      while (*++str)
        {
          if (*str >= '0' && *str <= '9')
            ;
          else if (*str == 'e' || *str == 'E')
            {
              // Might be followed by + or -
              ++str;
              if (*str == '+' || *str == '-') ++str;
            }
          else if (*str == '.')
            num_type = "double";
          else if (*str == 'f' || *str == 'F')
            {
              num_type = "float";
              break;
            }
          else if (*str == 'l' || *str == 'L')
            {
              if (num_type == "int") num_type = "long";
              else if (num_type == "unsigned") num_type = "unsigned long";
              else if (num_type == "float") num_type = "long double";
              else std::cout << "Unknown num type: " << num_type << std::endl;
            }
          else if (*str == 'u' || *str == 'U')
            {
              if (num_type == "int") num_type = "unsigned";
              else if (num_type == "long") num_type = "unsigned long";
              else std::cout << "Unknown num type: " << num_type << std::endl;
            }
          else
            // End of numeric constant
            break;
      }
      m_type = m_lookup->lookupType(num_type);
    }
  else if (*str == '\'')
    {
      // Whole node is a char literal
      if (m_links) m_links->span(node, "file-string");
      m_type = m_lookup->lookupType("char");
    }
  else if (*str == '"')
    {
      // Assume whole node is a string
      if (m_links) m_links->span(node, "file-string");
      m_type = m_lookup->lookupType("char");
      Types::Type::Mods pre, post; pre.push_back("const"); post.push_back("*");
      m_type = new Types::Modifier(m_type, pre, post);
    }
  else if (*str == '/' && !node->IsLeaf())
    {
      // Assume comment. Must be a list of comments!
      AST::Declaration* decl;
      decl = m_builder->add_tail_comment(m_lineno);
      add_comments(decl, node);

   }
  else
    {
#ifdef DEBUG
      STrace trace("SWalker::TranslatePtree");
      LOG("Warning: Unknown Ptree "<<node->What());
      nodeLOG(node);
      //*((char*)0) = 1; // force breakpoint, or core dump :)
#endif
    }
  return 0;
}

//. NamespaceSpec
//. [ namespace <identifier> [{ body }] ]
Ptree*
SWalker::TranslateNamespaceSpec(Ptree* def)
{
  STrace trace("SWalker::TranslateNamespaceSpec");

  Ptree* pNamespace = def->First();
  Ptree* pIdentifier = def->Second();
  Ptree* pBody = def->Third();
  
  if (m_links) m_links->span(pNamespace, "file-keyword");
  else update_line_number(def);

  // Start the namespace
  AST::Namespace* ns;
  if (pIdentifier)
    ns = m_builder->start_namespace(parse_name(pIdentifier), NamespaceNamed);
  else
    ns = m_builder->start_namespace(m_filename, NamespaceAnon);

  // Add comments
  add_comments(ns, dynamic_cast<PtreeNamespaceSpec*>(def));
  if (m_links && Ptree::First(pIdentifier)) m_links->link(pIdentifier, ns);

  // Translate the body
  Translate(pBody);

  // End the namespace
  m_builder->end_namespace();
  return 0;
}

//. [ : (public|private|protected|nil) <name> {, ...} ]
std::vector<Inheritance*> SWalker::TranslateInheritanceSpec(Ptree *node)
{
  STrace trace("PyWalker::TranslateInheritanceSpec");
  std::vector<Inheritance*> ispec;
  Types::Type *type;
  while (node)
    {
      node = node->Cdr(); // skip : or ,
      // the attributes
      std::vector<std::string> attributes(node->Car()->Length() - 1);
      for (int i = 0; i != node->Car()->Length() - 1; ++i)
        {
          attributes[i] = parse_name(node->Car()->Nth(i));
          if (m_links) m_links->span(node->Car()->Nth(i), "file-keyword");
        }
      // look up the parent type
      Ptree* name = node->Car()->Last()->Car();
      if (name->IsLeaf())
        {
          try
            { type = m_lookup->lookupType(parse_name(name)); }
          catch (const TranslateError)
            {
              // Ignore error, and put an Unknown in, instead
              ScopedName uname; uname.push_back(parse_name(name));
              type = new Types::Unknown(uname);
            }
        }
      else
        {
          char* encname = name->GetEncodedName();
          m_decoder->init(encname);
          type = m_decoder->decodeType();
        }
      if (m_links) m_links->link(name, type);

      node = node->Cdr();
      // add it to the list
      ispec.push_back(new AST::Inheritance(type, attributes));
    }
  return ispec;
}


Ptree*
SWalker::TranslateClassSpec(Ptree* node) 
{
  // REVISIT: figure out why this method is so long
  STrace trace("SWalker::TranslateClassSpec");
  enum { SizeForwardDecl = 2, SizeAnonClass = 3, SizeClass = 4 };

  AST::Parameter::vector* is_template = m_template;
  m_template = NULL;

  int size = Ptree::Length(node);

  if (size == SizeForwardDecl)
    {
      // Forward declaration
      // [ class|struct <name> ]
      std::string name = parse_name(node->Second());
      if (is_template) LOG("Templated class forward declaration " << name);
      m_builder->add_forward(m_lineno, name, is_template);
      if (m_links)
        { // highlight the comments, at least
          PtreeClassSpec* cspec = static_cast<PtreeClassSpec*>(node);
          add_comments(NULL, cspec->GetComments());
        }
      return 0;
    }
  Ptree* pClass = node->First();
  Ptree* pName = NULL, *pInheritance = NULL;
  Ptree* pBody = NULL;
  if (size == SizeClass)
    {
      // [ class|struct <name> <inheritance> [{ body }] ]
      pName = node->Nth(1);
      pInheritance = node->Nth(2);
      pBody = node->Nth(3);
    }
  else if (size == SizeAnonClass)
    // An anonymous struct. OpenC++ encodes us a unique
    // (may be qualified if nested) name
    // [ struct [nil nil] [{ ... }] ]
    pBody = node->Nth(2);
  else
    throw nodeERROR(node, "Class node has bad length: " << size);

  if (m_links) m_links->span(pClass, "file-keyword");
  else update_line_number(node);
  
  // Create AST.Class object
  AST::Class *clas;
  std::string type = parse_name(pClass);
  char* encname = node->GetEncodedName();
  m_decoder->init(encname);
  if (encname[0] == 'T')
  {
    // Specialization
    // TODO: deal with this.
    // Eg: /usr/include/g++-3/std/straits.h
    // search: /^struct string_char_traits <char> {/
    // encname: "T\222string_char_traits\201c"
    LOG("Specialization?");
    nodeLOG(node);
    LOG("encname:"<<(code_iter)encname);
    Types::Parameterized* param = dynamic_cast<Types::Parameterized*>(m_decoder->decodeTemplate());
    // If a non-type param was found, it's name will be '*'
    for (size_t i = 0; i < param->parameters().size(); i++)
      if (Types::Dependent* dep = dynamic_cast<Types::Dependent*>(param->parameters()[i]))
      {
        if (dep->name().size() == 1 && dep->name()[0] == "*")
        {
          // Find the value of this parameter
          std::string name = parse_name(pName->Second()->Second()->Nth(i*2));
          dep->name()[0] = name;
        }
      }

    m_type_formatter->push_scope(m_builder->scope()->name());
    std::string name, spacey_name = m_type_formatter->format(param);
    m_type_formatter->pop_scope();
    std::remove_copy(
        spacey_name.begin(), spacey_name.end(),
        std::back_inserter(name), ' ');
    clas = m_builder->start_class(m_lineno, type, name, is_template);
    // TODO: figure out spec stuff, like what to do with vars, link to
    // original template, etc.
  }
  else if (encname[0] == 'Q')
  {
    ScopedName names;
    m_decoder->decodeQualName(names);
    clas = m_builder->start_class(m_lineno, type, names);
  }
  else 
  {
    std::string name = m_decoder->decodeName();
    clas = m_builder->start_class(m_lineno, type, name, is_template);
  }
  if (m_links && pName) m_links->link(pName, clas);
  LOG("Translating class '" << clas->name() << "'");

  // Translate the inheritance spec, if present
  if (pInheritance)
    {
      clas->parents() = TranslateInheritanceSpec(pInheritance);
      m_builder->update_class_base_search();
    }
  
  // Add comments
  PtreeClassSpec* cspec = static_cast<PtreeClassSpec*>(node);
  add_comments(clas, cspec->GetComments());

  // Push the impl stack for a cache of func impls
  m_func_impl_stack.push_back(FuncImplVec());

  // Translate the body of the class
  TranslateBlock(pBody);

  // Translate any func impls inlined in the class
  FuncImplVec& vec = m_func_impl_stack.back();
  FuncImplVec::iterator iter = vec.begin();
  while (iter != vec.end())
    TranslateFuncImplCache(*iter++);
  m_func_impl_stack.pop_back();
      
  m_builder->end_class();
  return 0;
}

Ptree*
SWalker::TranslateTemplateClass(Ptree* def, Ptree* node)
{
  STrace trace("SWalker::TranslateTemplateClass");
  AST::Parameter::vector* old_params = m_template;
  update_line_number(def);
  m_builder->start_template();
  try {
    TranslateTemplateParams(def->Third());
    TranslateClassSpec(node);
  }
  catch (...)
  {
    m_builder->end_template();
    m_template = old_params;
    throw;
  }
  m_builder->end_template();
  m_template = old_params;
  return 0;
}

void SWalker::TranslateTemplateParams(Ptree* params)
{
  STrace trace("SWalker::TranslateTemplateParams");
  m_template = new AST::Parameter::vector;
  AST::Parameter::vector& templ_params = *m_template;
  // Declare some default parameter values - these should not be modified!
  std::string name, value;
  AST::Parameter::Mods pre_mods, post_mods;
  while (params)
  {
    Ptree* param = params->First();
    nodeLOG(param);
    if (param->First()->Eq("class") || param->First()->Eq("typename"))
    {
      // Ensure that there is an identifier (it is optional!)
      if (param->Cdr() && param->Second())
      {
        // This parameter specifies a type, add as dependent 
        Types::Dependent* dep = m_builder->create_dependent(parse_name(param->Second()));
        m_builder->add(dep);
        AST::Parameter::Mods paramtype;
        paramtype.push_back(parse_name(param->First()));
        templ_params.push_back(new AST::Parameter(paramtype, dep, post_mods, name, value));
      }
      else
      {
        // Add a parameter, but with no name
        AST::Parameter::Mods paramtype;
        paramtype.push_back(parse_name(param->First()));
        templ_params.push_back(new AST::Parameter(paramtype, NULL, post_mods, name, value));
      }
    }
    else if (param->First()->Eq("template"))
    {
      // A non-type parameter that is templatized
      // eg: template< class A, template<class T> class B = foo > C;
      // FIXME.
      LOG("templated non-type template parameter!");
      nodeLOG(param);
    }
    else
    {
      // This parameter specifies a value or something
      // FIXME can do a lot more here..
      LOG("non-type template parameter! approximating..");
      nodeLOG(param);
      Ptree* p = param->Second();
      while (p && p->Car() && p->Car()->IsLeaf() && (p->Car()->Eq('*') || p->Car()->Eq('&'))) p = Ptree::Rest(p);
      std::string name = parse_name(p);
      Types::Dependent* dep = m_builder->create_dependent(name);
      m_builder->add(dep);
      // Figure out the type of the param
      m_decoder->init(param->Second()->GetEncodedType());
      Types::Type* param_type = m_decoder->decodeType();
      templ_params.push_back(new AST::Parameter(pre_mods, param_type, post_mods, name, value));
    }
    // Skip comma
    params = Ptree::Rest(params->Rest());
  }
  /*
  Types::Template* templ = new Types::Template(decl->name(), decl, templ_params);
  if (AST::Class* clas = dynamic_cast<AST::Class*>(decl))
    clas->set_template_type(templ);
  else if (AST::Function* func = dynamic_cast<AST::Function*>(decl))
    func->set_template_type(templ);
  std::ostrstream buf;
  buf << "template " << decl->type() << std::ends;
  decl->set_type(buf.str());
  */
}

Ptree*
SWalker::TranslateTemplateFunction(Ptree* def, Ptree* node)
{
  STrace trace("SWalker::TranslateTemplateFunction");
  nodeLOG(def);
  nodeLOG(node);
  if (node->What() != ntDeclaration) {
    LOG("Warning: Unknown node type in template");
    nodeLOG(def);
    return 0;
  }

  LOG("What is: " << node->What());
  LOG("Encoded name is: " << (code_iter)node->GetEncodedName());

  AST::Parameter::vector* old_params = m_template;
  update_line_number(def);
  m_builder->start_template();
  try
  {
    TranslateTemplateParams(def->Third());
    TranslateDeclaration(node);
  }
  catch (...)
  {
    m_builder->end_template();
    m_template = old_params;
    throw;
  }
  m_builder->end_template();
  m_template = old_params;
  return 0;
}

//. Linkage Spec
//. [ extern ["C++"] [{ body }] ]
Ptree*
SWalker::TranslateLinkageSpec(Ptree* node)
{
  STrace trace("SWalker::TranslateLinkageSpec");
  Translate(node->Third());
  return 0;
}

//. Block
//. [ { [ <statement>* ] } ]
Ptree*
SWalker::TranslateBlock(Ptree* block)
{
  STrace trace("SWalker::TranslateBlock");
  Ptree* rest = Ptree::Second(block);
  while (rest != nil)
    {
      Translate(rest->Car());
      rest = rest->Cdr();
    }
  if (m_extract_tails)
    {
      Ptree* close = Ptree::Third(block);
      AST::Declaration* decl;
      decl = m_builder->add_tail_comment(m_lineno);
      add_comments(decl, dynamic_cast<CommentedLeaf*>(close));
    }

  return 0;
}

//. Brace
//. [ { [ <statement>* ] } ]
Ptree*
SWalker::TranslateBrace(Ptree* brace) 
{
  STrace trace("SWalker::TranslateBrace");
  Ptree* rest = Ptree::Second(brace);
  while (rest != nil)
    {
      Translate(rest->Car());
      rest = rest->Cdr();
    }
  if (m_extract_tails)
    {
      Ptree* close = Ptree::Third(brace);
      AST::Declaration* decl;
      decl = m_builder->add_tail_comment(m_lineno);
      add_comments(decl, dynamic_cast<CommentedLeaf*>(close));
    }

  return 0;
}

//. TemplateDecl
//. [ template < [types] > [decl] ]
Ptree*
SWalker::TranslateTemplateDecl(Ptree* def) 
{
  STrace trace("SWalker::TranslateTemplateDecl");
  Ptree* body = Ptree::Nth(def, 4);
  Ptree* class_spec = GetClassTemplateSpec(body);
  if(class_spec->IsA(ntClassSpec))
    TranslateTemplateClass(def, class_spec);
  else
    TranslateTemplateFunction(def, body);
  return 0;
}

//. Translates a declaration, either variable, typedef or function
//. Variables:
//.  [ [modifiers] name [declarators] ; ]
//. Function prototype:
//.  [ [modifiers] name [declarators] ; ]
//. Function impl:
//.  [ [modifiers] name declarator [ { ... } ] ]
//. Typedef:
//.  ?
//. Class definition:
//.  [ [modifiers] [class foo ...] [declarators]? ; ]
Ptree* SWalker::TranslateDeclaration(Ptree* def) 
{
  STrace trace("SWalker::TranslateDeclaration");
  // Link any comments added because we are inside a function body
  if (m_links) find_comments(def);

  update_line_number(def);

  m_declaration = def;
  m_store_decl = true;
  Ptree* decls = Ptree::Third(def);
  
  // Typespecifier may be a class {} etc.
  TranslateTypespecifier(Ptree::Second(def));

  if (decls->IsA(ntDeclarator))        // if it is a function
    TranslateFunctionImplementation(def);
  else
    // if it is a function prototype or a variable declaration.
    if (!decls->IsLeaf())        // if it is not ";"
      TranslateDeclarators(decls);
  m_declaration = NULL;
  return 0;
}

//. [ [ declarator { = <expr> } ] , ... ]
Ptree*
SWalker::TranslateDeclarators(Ptree* decls) 
{
  STrace trace("SWalker::TranslateDeclarators");
  Ptree* rest = decls, *p;
  while (rest != nil)
    {
      p = rest->Car();
      if (p->IsA(ntDeclarator))
        {
          TranslateDeclarator(p);
          m_store_decl = false;
        } // if. There is no else..?
      rest = rest->Cdr();
      // Skip comma
      if (rest != nil) rest = rest->Cdr();
    }
  return 0;
}

//. TranslateDeclarator
//. Function proto:
//.   [ { * | & }* name ( [params] ) ]
//. param:
//.   [ [types] { [ { * | & }* name ] { = value } } ]
Ptree*
SWalker::TranslateDeclarator(Ptree* decl) 
{
  // REVISIT: Figure out why this method is so HUGE!
  STrace trace("SWalker::TranslateDeclarator");
  // Insert code from occ.cc here
  char* encname = decl->GetEncodedName();
  char* enctype = decl->GetEncodedType();
  if (!encname || !enctype)
    {
      std::cout << "encname or enctype null!" << std::endl;
      return 0;
    }
  
  try
    {
      // Decide if this is a function or variable
      m_decoder->init(enctype);
      code_iter& iter = m_decoder->iter();
      bool is_const = false;
      while (*iter == 'C') { ++iter; is_const = true; }
      if (*iter == 'F')
        return TranslateFunctionDeclarator(decl, is_const);
      else
        return TranslateVariableDeclarator(decl, is_const);
    }
  catch (const TranslateError& e)
    {
      e.set_node(decl);
      throw;
    }
  return 0;
}


Ptree*
SWalker::TranslateFunctionDeclarator(Ptree* decl, bool is_const) 
{
  STrace trace("SWalker::TranslateFunctionDeclarator");
  AST::Parameter::vector* is_template = m_template;
  m_template = NULL;

  code_iter& iter = m_decoder->iter();
  char* encname = decl->GetEncodedName();

  // This is a function. Skip the 'F'
  ++iter;

  // Create parameter objects
  Ptree *p_params = decl->Rest();
  while (p_params && !p_params->Car()->Eq('(')) p_params = Ptree::Rest(p_params);
  if (!p_params) { std::cout << "Warning: error finding params!" << std::endl; return 0; }
  std::vector<AST::Parameter*> params;
  TranslateParameters(p_params->Second(), params);
  m_param_cache = params;

  // Figure out the return type:
  while (*iter++ != '_'); // in case of decoding error this is needed
  Types::Type* returnType = m_decoder->decodeType();

  // Figure out premodifiers
  std::vector<std::string> premod;
  Ptree* p = Ptree::First(m_declaration);
  while (p)
    {
      premod.push_back(p->ToString());
      p = Ptree::Rest(p);
    }

  AST::Operation* oper = 0;
  // Find name:
  if (encname[0] == 'Q')
    {
      // The name is qualified, which introduces a bit of difficulty
      std::vector<std::string> names;
      m_decoder->init(encname);
      m_decoder->decodeQualName(names);
      names.back() += format_parameters(params);
      // A qual name must already be declared, so find it:
      try
        {
          Types::Named* named_type = m_lookup->lookupType(names, true);
          oper = Types::declared_cast<AST::Operation>(named_type);
        }
      catch (const Types::wrong_type_cast &)
        { throw ERROR("Qualified function name wasn't a function:" << names); }
      // expand param info, since we now have names for them
      std::vector<AST::Parameter*>::iterator piter = oper->parameters().begin();
      std::vector<AST::Parameter*>::iterator pend = oper->parameters().end();
      std::vector<AST::Parameter*>::iterator new_piter = params.begin();
      while (piter != pend)
        {
          AST::Parameter* param = *piter++, *new_param = *new_piter++;
          if (!param->name().size() && new_param->name().size())
            param->set_name(new_param->name());
        }
    }
  else
    {
      // Decode the function name
      std::string realname;
      TranslateFunctionName(encname, realname, returnType);
      // Name is same as realname, but with parameters added
      std::string name = realname + format_parameters(params);
      // Append const after params if this is a const function
      if (is_const) name += "const";
      // Create AST::Operation object
      oper = m_builder->add_operation(m_lineno, name, premod, returnType, realname, is_template);
      oper->parameters() = params;
    }
  add_comments(oper, m_declaration);
  add_comments(oper, dynamic_cast<PtreeDeclarator*>(decl));

  // if storing links, find name
  if (m_links)
    {
      // Store for use by TranslateFunctionImplementation
      m_operation = oper;

      // Do decl type first
      if (m_store_decl && m_declaration->Second())
        m_links->link(m_declaration->Second(), returnType);

      p = decl;
      while (p && p->Car()->IsLeaf() && (p->Car()->Eq('*') || p->Car()->Eq('&'))) p = Ptree::Rest(p);
      if (p)
        // p should now be at the name
        m_links->link(p->Car(), oper);
    }
  return 0;
}

Ptree*
SWalker::TranslateVariableDeclarator(Ptree* decl, bool is_const) 
{
  STrace trace("TranslateVariableDeclarator");
  // Variable declaration. Restart decoding
  char* encname = decl->GetEncodedName();
  char* enctype = decl->GetEncodedType();
  m_decoder->init(enctype);
  // Get type
  Types::Type* type = m_decoder->decodeType();
  std::string name;
  if (m_decoder->isName(encname)) name = m_decoder->decodeName(encname);
  else if (*encname == 'Q')
    {
      LOG("Scoped name in variable decl!");
      nodeLOG(decl);
      return 0;
    }
  else
    {
      LOG("Unknown name in variable decl!");
      nodeLOG(decl);
      return 0;
    }

  // TODO: implement sizes support
  std::vector<size_t> sizes;
  std::string var_type = m_builder->scope()->type();
  if (var_type == "function") var_type = "local";
  var_type += " variable";
  AST::Variable* var = m_builder->add_variable(m_lineno, name, type, false, var_type);
  //if (m_declaration->GetComments()) add_comments(var, m_declaration->GetComments());
  //if (decl->GetComments()) add_comments(var, decl->GetComments());
  add_comments(var, m_declaration);
  add_comments(var, dynamic_cast<PtreeDeclarator*>(decl));
  
  // if storing links, find name
  if (m_links)
    {
      // Do decl type first
      if (m_store_decl && m_declaration->Second()) 
        m_links->link(m_declaration->Second(), type);

      Ptree* p = decl;
      while (p && p->Car()->IsLeaf() && (p->Car()->Eq('*') || p->Car()->Eq('&'))) p = Ptree::Rest(p);
      if (p)
        {
          // p should now be at the name
          m_links->link(p->Car(), var);

          // Next might be '=' then expr
          p = p->Rest();
          if (p && p->Car() && p->Car()->Eq('='))
            {
              p = p->Rest();
              if (p && p->Car()) Translate(p->Car());
            }
        }
    }
  return 0;
}

// Fills the vector of Parameter types by parsing p_params.
void
SWalker::TranslateParameters(Ptree* p_params, std::vector<AST::Parameter*>& params)
{
  while (p_params)
    {
      // A parameter has a type, possibly a name and possibly a value
      std::string name, value;
      AST::Parameter::Mods premods, postmods;
      if (p_params->Car()->Eq(',')) p_params = p_params->Cdr();
      Ptree* param = p_params->First();
      // The type is stored in the encoded type string already
      Types::Type* type = m_decoder->decodeType();
      if (!type)
        {
          std::cout << "Premature end of decoding!" << std::endl;
          break; // NULL means end of encoding
        }
      // Discover contents. Ptree may look like:
      //[register iostate [* a] = [0 + 2]]
      //[register iostate [nil] = 0]
      //[register iostate [nil]]
      //[iostate [nil] = 0]
      //[iostate [nil]]   etc
      if (param->Length() > 1)
        {
          // There is a parameter
          int type_ix, value_ix = -1, len = param->Length();
          if (len >= 4 && param->Nth(len-2)->Eq('='))
            {
              // There is an =, which must be followed by the value
              value_ix = len-1;
              type_ix = len-4;
            }
          else
            {
              // No =, so last is name and second last is type
              type_ix = len-2;
            }
          // Link type
          if (m_links && !param->IsLeaf() && param->Nth(type_ix)) 
            m_links->link(param->Nth(type_ix), type);
          // Skip keywords (eg: register) which are Leaves
          for (int ix = 0; ix < type_ix && param->Nth(ix)->IsLeaf(); ix++)
            {
              Ptree* leaf = param->Nth(ix);
              premods.push_back(parse_name(leaf));
            }
          // Find name
          if (Ptree* pname = param->Nth(type_ix+1))
            {
              if (!pname->IsLeaf() && pname->Last() && pname->Last()->Car())
                {
                  // * and & modifiers are stored with the name so we must skip them
                  Ptree* last = pname->Last()->Car();
                  if (!last->Eq('*') && !last->Eq('&'))
                      // The last node is the name:
                    name = last->ToString();
                }
            }
          // Find value
          if (value_ix >= 0) value = param->Nth(value_ix)->ToString();
        }
      // Add the AST.Parameter type to the list
      params.push_back(new AST::Parameter(premods, type, postmods, name, value));
      p_params = Ptree::Rest(p_params);
    }
}

void SWalker::TranslateFunctionName(char* encname, std::string& realname, Types::Type*& returnType)
{
  STrace trace("SWalker::TranslateFunctionName");
  if (m_decoder->isName(encname))
    {
      if (encname[1] == '@')
        {
          // conversion operator
          m_decoder->init(encname);
          m_decoder->iter() += 2;
          returnType = m_decoder->decodeType();
          realname = "("+m_type_formatter->format(returnType)+")";
        }
      else
      {
        // simple name
        realname = m_decoder->decodeName(encname);
        // operator names are missing the 'operator', add it back
        char c = realname[0];
        if (c == '+' || c == '-' || c == '*' || c == '/' || c == '%'
            || c == '^' || c == '&' || c == '~' || c == '!' || c == '='
            || c == '<' || c == '>' || c == ',' || c == '(' || c == '[')
          realname = "operator"+realname;
      }
    }
  else if (*encname == 'Q')
    {
      // If a function declaration has a scoped name, then it is not
      // declaring a new function in that scope and can be ignored in
      // the context of synopsis.
      // TODO: maybe needed for syntax stuff?
      return;
    }
  else if (*encname == 'T')
    {
      // Template specialisation.
      // blah<int, int> is T4blah2ii ---> realname = foo<int,int>
      m_decoder->init(encname);
      code_iter& iter = ++m_decoder->iter();
      realname = m_decoder->decodeName()+"<";
      code_iter tend = iter + (*iter - 0x80u);
      iter++; // For some reason, putting this in prev line causes error with 3.2
      bool first = true;
      // Append type names to realname
      while (iter <= tend)
        {
          /*Types::Type* type = */m_decoder->decodeType();
          if (!first) realname+=","; else first=false;
          realname += "type"; //type->ToString();
        }
      realname += ">";
    }
  else
    std::cout << "Warning: Unknown function name: " << encname << std::endl;
}
        
//. Class or Enum
Ptree*
SWalker::TranslateTypespecifier(Ptree* tspec) 
{
  STrace trace("SWalker::TranslateTypespecifier");
  Ptree *class_spec = GetClassOrEnumSpec(tspec);
  if (class_spec) Translate(class_spec);
  return 0;
}

Ptree*
SWalker::TranslateTypedef(Ptree* node) 
{
  STrace trace("SWalker::TranslateTypedef");
  if (m_links) m_links->span(node->First(), "file-keyword");
  /* Ptree *tspec = */ TranslateTypespecifier(node->Second());
  m_declaration = node;
  m_store_decl = true;
  for (Ptree *declarator = node->Third(); declarator; declarator = declarator->ListTail(2))
    TranslateTypedefDeclarator(declarator->Car());
  return 0; 
}

void SWalker::TranslateTypedefDeclarator(Ptree* node)
{
  if (node->What() != ntDeclarator) return;
  char* encname = node->GetEncodedName();
  char* enctype = node->GetEncodedType();
  if (!encname || !enctype) return;

  update_line_number(node);

  // Get type of declarator
  m_decoder->init(enctype);
  Types::Type* type = m_decoder->decodeType();
  // Get name of typedef
  std::string name = m_decoder->decodeName(encname);
  // Create typedef object
  AST::Typedef* tdef = m_builder->add_typedef(m_lineno, name, type, false);
  add_comments(tdef, dynamic_cast<PtreeDeclarator*>(node));
  
  // if storing links, find name
  if (m_links)
    {
      if (m_store_decl && m_declaration->Second())
        m_links->link(m_declaration->Second(), type);
      Ptree* p = node;
      while (p && p->Car()->IsLeaf() && (p->Car()->Eq('*') || p->Car()->Eq('&')))
        p = Ptree::Rest(p);
      if (p)
        // p should now be at the name
        m_links->link(p->Car(), tdef);
    }
}

Ptree*
SWalker::TranslateFunctionImplementation(Ptree* node)
{
  STrace trace("SWalker::TranslateFunctionImplementation");
  m_operation = 0; m_params.clear();
  TranslateDeclarator(node->Third());
  if (!m_links) return 0; // Dont translate if not storing links
  if (m_filename != m_source) return 0; // Dont translate if not main file
  if (!m_operation) { std::cerr << "Warning: operation was null!" << std::endl; return 0; }

  FuncImplCache cache;
  cache.oper = m_operation;
  cache.params = m_param_cache;
  cache.body = node->Nth(3);

  if (dynamic_cast<AST::Class*>(m_builder->scope()))
    m_func_impl_stack.back().push_back(cache);
  else
    TranslateFuncImplCache(cache);
  return 0;
}

void
SWalker::TranslateFuncImplCache(const FuncImplCache& cache)
{
  STrace trace("SWalker::TranslateFuncImplCache");
  // We create a dummy namespace with the name of the function. Any
  // declarations in the function are added to this dummy namespace. Once we
  // are done, we remove it from the parent scope (its not much use in the
  // documents)
  std::vector<std::string> name = cache.oper->name();
  name.back() = "`"+name.back();
  m_builder->start_function_impl(name);
  try
    {
      // Add parameters
      std::vector<AST::Parameter*>::const_iterator iter, end;
      iter = cache.params.begin();
      end = cache.params.end();
      while (iter != end)
        {
          AST::Parameter* param = *iter++;
          m_builder->add_variable(m_lineno, param->name(), param->type(), false, "parameter");
        }
      // Add 'this' if method
      m_builder->add_this_variable();
      // Translate the function body
      TranslateBlock(cache.body);
    }
  catch (...)
    {
      LOG("Cleaning up func impl cache");
      m_builder->end_function_impl();
      throw;
    }
  m_builder->end_function_impl();
}

Ptree*
SWalker::TranslateAccessSpec(Ptree* spec)
{
  STrace trace("SWalker::TranslateAccessSpec");
  AST::Access axs = AST::Default;
  switch (spec->First()->What())
    {
    case PUBLIC:    axs = AST::Public;    break;
    case PROTECTED: axs = AST::Protected; break;
    case PRIVATE:   axs = AST::Private;   break;
    }
  m_builder->set_access(axs);
  if (m_links) m_links->span(spec->First(), "file-keyword");
  return 0;
}

/* Enum Spec
 *  [ enum [name] [{ [name [= value] ]* }] ]
 */
Ptree*
SWalker::TranslateEnumSpec(Ptree *spec)
{
  //update_line_number(spec);
  if (m_links) m_links->span(spec->First(), "file-keyword");
  if (!spec->Second()) { return 0; /* anonymous enum */ }
  std::string name = spec->Second()->ToString();

  update_line_number(spec);
  int enum_lineno = m_lineno;
  // Parse enumerators
  std::vector<AST::Enumerator*> enumerators;
  Ptree* penum = spec->Third()->Second();
  AST::Enumerator* enumor;
  while (penum)
    {
      update_line_number(penum);
      Ptree* penumor = penum->First();
      if (penumor->IsLeaf())
        {
          // Just a name
          enumor = m_builder->add_enumerator(m_lineno, penumor->ToString(), "");
          add_comments(enumor, static_cast<CommentedLeaf*>(penumor)->GetComments());
          if (m_links) m_links->link(penumor, enumor);
        }
      else
        {
          // Name = Value
          std::string name = penumor->First()->ToString(), value;
          if (penumor->Length() == 3)
            value = penumor->Third()->ToString();
          enumor = m_builder->add_enumerator(m_lineno, name, value);
          add_comments(enumor, dynamic_cast<CommentedLeaf*>(penumor->First()));
          if (m_links) m_links->link(penumor->First(), enumor);
        }
      enumerators.push_back(enumor);
      penum = Ptree::Rest(penum);
      // Skip comma
      if (penum && penum->Car() && penum->Car()->Eq(','))
        penum = Ptree::Rest(penum);
    }
  if (m_extract_tails)
    {
      Ptree* close = spec->Third()->Third();
      enumor = new AST::Enumerator(m_builder->filename(), m_lineno, "dummy", m_dummyname, "");
      add_comments(enumor, static_cast<CommentedLeaf*>(close));
      enumerators.push_back(enumor);
    }

  // Create AST.Enum object
  AST::Enum* theEnum = m_builder->add_enum(enum_lineno,name,enumerators);
  add_comments(theEnum, m_declaration);
  if (m_declaration)
    {
      // Enum declared inside declaration. Comments for the declaration
      // belong to the enum. This is policy. #TODO review policy
      //m_declaration->SetComments(nil); ?? typedef doesn't have comments?
    }
  if (m_links) m_links->link(spec->Second(), theEnum);
  return 0;
}


Ptree*
SWalker::TranslateUsing(Ptree* node)
{
  STrace trace("SWalker::TranslateUsing");
  // [ using Foo :: x ; ]
  // [ using namespace Foo ; ]
  // [ using namespace Foo = Bar ; ]
  if (m_links) m_links->span(node->First(), "file-keyword");
  bool is_namespace = false;
  Ptree *p = node->Rest();
  if (p->First()->Eq("namespace"))
    {
      if (m_links) m_links->span(p->First(), "file-keyword");
      // Find namespace to alias
      p = p->Rest();
      is_namespace = true;
    }
  // Find name that we are looking up, and make a new ptree list for linking it
  Ptree *p_name = Ptree::Snoc(nil, p->Car());
  ScopedName name;
  if (p->First()->Eq("::"))
    // Eg; "using ::memcpy;" Indicate global scope with empty first
    name.push_back("");
  else 
    {
      name.push_back(parse_name(p->First()));
      p = p->Rest(); 
    }
  while (p->First()->Eq("::"))
    {
      p_name = Ptree::Snoc(p_name, p->Car()); // Add '::' to p_name
      p = p->Rest();
      name.push_back(parse_name(p->First()));
      p_name = Ptree::Snoc(p_name, p->Car()); // Add identifier to p_name
      p = p->Rest();
    }
  // Resolve and link name
  try
    {
      Types::Named* type = m_lookup->lookupType(name);
      if (m_links) m_links->link(p_name, type);
      if (is_namespace)
        {
          // Check for '=' alias
          if (p->First()->Eq("="))
            {
              p = p->Rest();
              std::string alias = parse_name(p->First());
              m_builder->add_aliased_using_namespace(type, alias);
            }
          else
            m_builder->add_using_namespace(type);
        }
      else
        // Let builder do all the work
        m_builder->add_using_declaration(type);
    }
  catch (const TranslateError& e)
    { LOG("Oops!"); e.set_node(node); throw; }
  return 0;
}


