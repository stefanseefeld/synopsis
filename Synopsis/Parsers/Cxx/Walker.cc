//
// Copyright (C) 2000 Stefan Seefeld
// Copyright (C) 2000 Stephen Davies
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include <Python.h>
#include <iostream>
#include <string>
#include <typeinfo>
#include <sstream>
#include <algorithm>

#include <Synopsis/Buffer.hh>
#include <Synopsis/PTree.hh>
#include <Synopsis/PTree/Writer.hh>
#include <Synopsis/PTree/Display.hh>

#include "Walker.hh"
#include "STrace.hh"
#include "Types.hh"
#include "ASG.hh"
#include "Builder.hh"
#include "Decoder.hh"
#include "TypeIdFormatter.hh"
#include "SXRGenerator.hh"
#include "Lookup.hh"
#include "Filter.hh"
#include "Dictionary.hh"

using namespace ASG;
using Synopsis::Token;
using Synopsis::Buffer;
#ifdef DO_TRACE
int STrace::slevel = 0, STrace::dlevel = 0;
std::ostringstream* STrace::stream = 0;
STrace::string_list STrace::m_list;
std::ostream& STrace::operator <<(PTree::Node *p)
{
  std::ostream& out = operator <<("-");
  PTree::display(p, out, true);
  return out;
}
#endif

namespace
{
//. Helper function to recursively find the first left-most leaf node
PTree::Node *find_left_leaf(PTree::Node *node, PTree::Node *& parent)
{
  if (!node || node->is_atom()) return node;
  // Non-leaf node. So find first leafy child
  PTree::Node *leaf;
  while (node)
  {
    if (node->car())
    {
      // There is a child here..
      if (node->car()->is_atom())
      {
        // And this child is a leaf! return it and set parent
        parent = node;
        return node->car();
      }
      if ((leaf = find_left_leaf(node->car(), parent)))
        // Not a leaf so try recursing on it
        return leaf;
    }
    // No leaves from car of this node, so try next cdr
    node = node->cdr();
  }
  return 0;
}

PTree::Node *strip_cv_from_integral_type(PTree::Node *integral)
{
  if(integral == 0) return 0;

  if(!integral->is_atom())
    if(PTree::is_a(integral->car(), Token::CONST, Token::VOLATILE))
      return PTree::second(integral);
    else if(PTree::is_a(PTree::second(integral), Token::CONST, Token::VOLATILE))
      return integral->car();

  return integral;
}

PTree::Node *get_class_or_enum_spec(PTree::Node *typespec)
{
  PTree::Node *spec = strip_cv_from_integral_type(typespec);
  if(PTree::is_a(spec, Token::ntClassSpec, Token::ntEnumSpec))
    return spec;
  return 0;
}

PTree::ClassSpec *get_class_template_spec(PTree::Node *body)
{
  if(*PTree::third(body) == ';')
  {
    PTree::Node *spec = strip_cv_from_integral_type(PTree::second(body));
    return static_cast<PTree::ClassSpec *>(spec);
  }
  return 0;
}

}



Walker *Walker::g_walker = 0;

Walker::Walker(FileFilter* filter, Builder* builder, Buffer* buffer)
  : //Walker(buffer),
    my_builder(builder),
    my_filter(filter),
    my_buffer(buffer),
    my_decoder(new Decoder(my_builder)),
    my_declaration(0),
    my_in_typedef(false),
    my_defines_class_or_enum(false),
    my_template(0),
    my_lineno(0),
    my_file(0),
    sxr_(0),
    my_store_decl(false),
    my_type_formatter(new TypeIdFormatter()),
    my_function(0),
    my_type(0),
    my_scope(0),
    my_postfix_flag(Postfix_Var),
    my_in_template_decl(false)
{
  g_walker = this; // FIXME: is this needed?
  my_builder->set_walker(this);
  my_lookup = my_builder->lookup();
}

// Destructor
Walker::~Walker()
{
  delete my_decoder;
  delete my_type_formatter;
}

// The name returned is just the node's text if the node is a leaf. Otherwise,
// the ToString method of Ptree is used, which is rather expensive since it
// creates a temporary write buffer and reifies the node tree into it.
std::string
Walker::parse_name(PTree::Node *node) const
{
  return PTree::reify(node);
}

void
Walker::set_store_links(SXRGenerator* sxr)
{
  sxr_ = sxr;
}

int Walker::line_of_ptree(PTree::Node *node)
{
  update_line_number(node);
  return my_lineno;
}

// Updates the line number stored in this Walker instance, and the filename
// stored in the Builder instance at my_builder. The filename is only set if
// the actual char* changed (which only happens when the preprocessor places
// another #line directive)
void Walker::update_line_number(PTree::Node *ptree)
{
  // Ask the Parser for the linenumber of the ptree. This used to be
  // expensive until I hacked buffer.cc to cache the last line number found.
  // Now it's okay as long as you are looking for lines sequentially.
  std::string filename;
  my_lineno = my_buffer->origin(ptree->begin(), filename);
  if (filename != my_filename)
  {
    my_filename = filename;
    my_file = my_filter->get_sourcefile(my_filename.c_str());
    my_builder->set_file(my_file);
  }
}

ASG::Comment *
make_Comment(SourceFile* file, int line, PTree::Node *first, bool suspect=false)
{
  return new ASG::Comment(file, line, PTree::reify(first), suspect);
}

PTree::Atom *make_Leaf(const char *pos, size_t len)
{
    return new PTree::Atom(pos, len);
}

// Adds the given comments to the given declaration.
void
Walker::add_comments(ASG::Declaration* decl, PTree::Node *node)
{
  if (!node) return;
  std::vector<std::string> comments;

  // First, make sure that node is a list of comments
  if (PTree::type_of(node) == Token::ntDeclaration)
    node = static_cast<PTree::Declaration*>(node)->get_comments();
  // Loop over all comments in the list
  bool suspect = false;
  for (PTree::Node *next = PTree::rest(node); node && !node->is_atom(); next = PTree::rest(node))
  {
    PTree::Node *first = PTree::first(node);
    if (!first || !first->is_atom())
    {
      node = next;
      continue;
    }
    // Check if comment is continued, eg: consecutive C++ comments
    while (next && PTree::first(next) && PTree::first(next)->is_atom())
    {
      if (!strncmp(first->position() + first->length() - 2, "*/", 2))
        break;
      if (strncmp(PTree::first(next)->position(), "//", 2))
        break;
      const char *next_pos = PTree::first(next)->position();
      const char *start_pos = PTree::first(node)->position();
      const char *curr_pos = start_pos + PTree::first(node)->length();
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
      int len = int(next_pos - start_pos + PTree::first(next)->length());
      //node->SetCar(first = new Leaf(start_pos, len));
      node->set_car(first = make_Leaf(start_pos, len));
      // Skip the combined comment
      next = PTree::rest(next);
    }

    // all comments that are not immediately (i.e. separated
    // by a single new line) followed by a declaration are
    // marked as 'suspect'
    suspect = false;
    const char *pos = first->position() + first->length();
    while (*pos && strchr(" \t\r", *pos)) ++pos;
    if (*pos == '\n')
    {
      ++pos;
      // Found only allowed \n
      while (*pos && strchr(" \t\r", *pos)) ++pos;
      if (*pos == '\n' || !strncmp(pos, "/*", 2)) suspect = true;
    }

    if (decl)
      comments.push_back(PTree::reify(first));
    if (sxr_) sxr_->long_span(first, "comment");
    // Set first to 0 so we dont accidentally do them twice (eg:
    // when parsing expressions)
    node->set_car(0);
    node = next;
  }
  if (suspect) comments.push_back("");
  // Now add the comments, if applicable
  if (decl)
    decl->comments() = comments;
}

// -- These methods implement add_comments for various node types that store
// comment pointers
void Walker::add_comments(ASG::Declaration* decl, PTree::CommentedAtom *node)
{
  if (node) add_comments(decl, node->get_comments());
}
void Walker::add_comments(ASG::Declaration* decl, PTree::Declaration* node)
{
  if (node) add_comments(decl, node->get_comments());
}
void Walker::add_comments(ASG::Declaration* decl, PTree::Declarator* node)
{
  if (node) add_comments(decl, node->get_comments());
}
void Walker::add_comments(ASG::Declaration* decl, PTree::NamespaceSpec* node)
{
  if (node) add_comments(decl, node->get_comments());
}
void Walker::find_comments(PTree::Node *node)
{
  PTree::Node *parent;
  PTree::Node *leaf = find_left_leaf(node, parent);
  if (leaf) add_comments(0, dynamic_cast<PTree::CommentedAtom *>(leaf));
}

PTree::Node *Walker::translate_arg_decl_list(bool, PTree::Node *, PTree::Node *)
{
  STrace trace("Walker::translate_arg_decl_list NYI");
  return 0;
}

PTree::Node *Walker::translate_initialize_args(PTree::Declarator*, PTree::Node *)
{
  STrace trace("Walker::translate_initialize_args NYI");
  return 0;
}

PTree::Node *Walker::translate_assign_initializer(PTree::Declarator*, PTree::Node *)
{
  STrace trace("Walker::translate_assign_initializer NYI");
  return 0;
}

// Format the given parameters. my_type_formatter is used to format the given
// list of parameters into a string, suitable for use as the name of a
// Function object.
std::string Walker::format_parameters(ASG::Parameter::vector& params)
{
  // TODO: Tell formatter to expand typedefs! Eg: this function uses a typedef
  // in implementation, but not for declaration in the class!!!!!!
  ASG::Parameter::vector::iterator iter = params.begin(), end = params.end();
  if (iter == end) return "()";
  // Set scope for formatter
  ASG::Scope* scope = my_builder->scope();
  if (scope) my_type_formatter->push_scope(scope->name());
  else my_type_formatter->push_scope(QName());
  // Format the parameters one at a time
  std::ostringstream buf;
  buf << "(" << my_type_formatter->format((*iter++)->type());
  while (iter != end)
    buf << "," << my_type_formatter->format((*iter++)->type());
  buf << ")";
  my_type_formatter->pop_scope();
  return buf.str();
}

void
Walker::translate(PTree::Node *node)
{
  STrace trace("Walker::translate");
  try
  {
    if (node) node->accept(this);
  }
  catch (Dictionary::KeyError const &e)
  {
    std::cerr << "Unknown type '" << e.name << "'\n";
    std::string filename;
    unsigned long line = my_buffer->origin(node->begin(), filename);
    std::cerr << " (" << filename << ":" << line << ")" << std::endl;
    throw;    
  }
  catch (Dictionary::MultipleError const &e)
  {
    std::cerr << "Multiple definitions found for type '" << e.name << "'\n";
    for (Dictionary::Type_vector::const_iterator i = e.types.begin(); i != e.types.end(); ++i)
      std::cerr << (*i)->name() << std::endl;
    throw;    
  }
  // Debug and non-debug modes handle these very differently
#if DEBUG
  catch (const TranslateError& e)
  {
    if (e.node) node = e.node;
    std::string filename;
    unsigned long line = my_buffer->origin(node->begin(),
					   filename);
    LOG("Warning: An exception occurred:" << " (" << filename << ":" << line << ")");
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
    std::cerr << "Warning: An exception occurred: " << e.what() << std::endl;
    std::cerr << "At: ";
    std::string filename;
    unsigned long line = my_buffer->origin(node->begin(), filename);
    std::cerr << " (" << filename << ":" << line << ")" << std::endl;
    throw;
  }
  catch (...)
  {
    std::cerr << "Warning: An unknown exception occurred: " << std::endl;
    std::cerr << "At: ";
    std::string filename;
    unsigned long line = my_buffer->origin(node->begin(), filename);
    std::cerr << " (" << filename << ":" << line << ")" << std::endl;
    throw;
  }
#endif
}

// Default translate, usually means a literal
void Walker::visit(PTree::Atom *node)
{
  STrace trace("Walker::visit(PTree::Atom *)");
  // Determine type of node
  std::string s = PTree::reify(node);
  const char *str = s.c_str();
  if ((*str >= '0' && *str <= '9') || *str == '.')
  {
    // Assume whole node is a number
    if (sxr_) sxr_->span(node, "literal");
    // TODO: decide if Long, Float, Double, etc
    const char* num_type = (*str == '.' ? "double" : "int");
    while (*++str)
    {
      if (*str >= '0' && *str <= '9') {}
      else if (*str == 'e' || *str == 'E')
      {
        // Might be followed by + or -
        ++str;
        if (*str == '+' || *str == '-') ++str;
      }
      else if (*str == '.') num_type = "double";
      else if (*str == 'f' || *str == 'F')
      {
        num_type = "float";
        break;
      }
      else if (*str == 'l' || *str == 'L')
      {
        if (strcmp(num_type, "int") == 0) num_type = "long";
        else if (strcmp(num_type, "long") == 0) num_type = "long long";
	else if (strcmp(num_type, "unsigned") == 0) num_type = "unsigned long";
	else if (strcmp(num_type, "float") == 0) num_type = "long double";
	else if (strcmp(num_type, "double") == 0) num_type = "long double";
        else
          std::cerr << "Unknown num type: " << num_type << std::endl;
      }
      else if (*str == 'u' || *str == 'U')
      {
        if (strcmp(num_type, "int") == 0) num_type = "unsigned";
        else if (strcmp(num_type, "long") == 0) num_type = "unsigned long";
        else
          std::cerr << "Unknown num type: " << num_type << std::endl;
      }
      else break;// End of numeric constant
    }
    my_type = my_lookup->lookupType(num_type);
  }
  else if (*str == '\'')
  {
    // Whole node is a char literal
    if (sxr_) sxr_->span(node, "string");
    my_type = my_lookup->lookupType("char");
  }
  else if (*str == '"')
  {
    // Assume whole node is a string
    if (sxr_) sxr_->span(node, "string");
    my_type = my_lookup->lookupType("char");
    Types::Type::Mods pre, post;
    pre.push_back("const");
    post.push_back("*");
    my_type = new Types::Modifier(my_type, pre, post);
  }
  else if (*str == '/' && !node->is_atom())
  {
    // Assume comment. Must be a list of comments!
    ASG::Declaration* decl;
    update_line_number(node);
    decl = my_builder->add_tail_comment(my_lineno);
    add_comments(decl, static_cast<PTree::CommentedAtom *>(node));
  }
  else
  {
#ifdef DEBUG
    STrace trace("Walker::TranslatePtree");
    LOG("Warning: Unknown Ptree "<<PTree::type_of(node));
    nodeLOG(node);
    //*((char*)0) = 1; // force breakpoint, or core dump :)
#endif
  }
}

// As various nodes may through due to incomplete symbol
// lookup support, we need an 'exception firewall' here to
// be able to traverse the following nodes
void Walker::visit(PTree::List *node)
{
  for (PTree::Node *i = node; i; i = i->cdr())
    if (i->car())
      try { i->car()->accept(this);}
      catch (const TranslateError &) {}
}

void Walker::visit(PTree::CommentedAtom *node)
{
  // The only purpose of this method is to filter
  // out those atoms that are used as end markers.
  // They can be recognized by having length() == 0.
  if (node->length() == 0)
  {
    // The begin of the node coincides with the start of the comment.
    update_line_number(node);
    ASG::Builtin *builtin = my_builder->add_tail_comment(my_lineno);
    add_comments(builtin, node);
  }
  else
    visit(static_cast<PTree::Atom *>(node));
}

//. NamespaceSpec
void Walker::visit(PTree::NamespaceSpec *node)
{
  STrace trace("Walker::visit(PTree::NamespaceSpec *)");
  update_line_number(node);

  PTree::Node *key = PTree::first(node);
  PTree::Node *id = PTree::second(node);
  PTree::Node *body = PTree::third(node);
  
  if (sxr_) sxr_->span(key, "keyword");

  // Start the namespace
  ASG::Namespace* ns;
  if (id)
  {
    ns = my_builder->start_namespace(parse_name(id), NamespaceNamed);
    ns->set_file(my_file);
  }
  else ns = my_builder->start_namespace(my_file->name(), NamespaceAnon);

  add_comments(ns, static_cast<PTree::NamespaceSpec*>(node));
  if (sxr_ && PTree::first(id)) sxr_->xref(id, ns);

  // Translate the body
  translate(body);

  // End the namespace
  my_builder->end_namespace();
}

//. [ : (public|private|protected|0) <name> {, ...} ]
std::vector<Inheritance*> Walker::translate_inheritance_spec(PTree::Node *node)
{
  STrace trace("Walker::translate_inheritance_spec");
  std::vector<Inheritance*> ispec;
  Types::Type *type;
  while (node)
  {
    node = node->cdr(); // skip : or ,
    // the attributes
    std::vector<std::string> attributes(PTree::length(node->car()) - 1);
    for (int i = 0; i != PTree::length(node->car()) - 1; ++i)
    {
      attributes[i] = parse_name(PTree::nth(node->car(), i));
      if (sxr_) sxr_->span(PTree::nth(node->car(), i), "keyword");
    }
    // look up the parent type
    PTree::Node *name = PTree::last(node->car())->car();
    if (name->is_atom())
    {
      try
      {
        type = my_lookup->lookupType(parse_name(name));
      }
      catch (const TranslateError)
      {
        // Ignore error, and put an Unknown in, instead
        QName uname;
        uname.push_back(parse_name(name));
        type = new Types::Unknown(uname);
      }
    }
    else
    {
      my_decoder->init(name->encoded_name());
      type = my_decoder->decodeType();
    }
    if (sxr_) sxr_->xref(name, type);

    node = node->cdr();
    // add it to the list
    ispec.push_back(new ASG::Inheritance(type, attributes));
  }
  return ispec;
}

void Walker::visit(PTree::ClassSpec *node)
{
  STrace trace("Walker::visit(PTree::ClassSpec*)");
#if DEBUG
  trace << node;
#endif
  ASG::Parameter::vector* is_template = my_template;
  my_template = 0;
  int size = PTree::length(node);
  PTree::Node *pClass = PTree::first(node);
  PTree::Node *pName = 0, *pInheritance = 0;
  PTree::ClassBody *pBody = 0;

  if (size == 2)
  {
    // Forward declaration
    // [ class|struct <name> ]
    pName = PTree::nth(node, 1);
    std::string name = parse_name(pName);
    if (is_template)
      LOG("Templated class forward declaration " << name);
    ASG::Forward *class_ = my_builder->add_forward(my_lineno, name, parse_name(pClass),
                                                   is_template);
    add_comments(class_, node->get_comments());
    return;
  }
  else if (size == 4)
  {
    // [ class|struct <name> <inheritance> [{ body }] ]
    pName = PTree::nth(node, 1);
    pInheritance = PTree::nth(node, 2);
    pBody = static_cast<PTree::ClassBody *>(PTree::nth(node, 3));
  }
  else if (size == 3)
    // An anonymous struct. OpenC++ encodes us a unique
    // (may be qualified if nested) name
    // [ struct [nil nil] [{ ... }] ]
    pBody = static_cast<PTree::ClassBody *>(PTree::nth(node, 2));
  else
    throw nodeERROR(node, "Class node has bad length: " << size);

  if (sxr_) sxr_->span(pClass, "keyword");
  else update_line_number(node);
  // Create ASG.Class object
  ASG::Class *clas;
  std::string type = parse_name(pClass);
  PTree::Encoding enc = node->encoded_name();
  my_decoder->init(enc);
  if (enc.at(0) == 'T')
  {
    Types::Parameterized* param = my_decoder->decodeTemplate();
    // If a non-type param was found, its name will be '*'
    for (size_t i = 0; i < param->parameters().size(); i++)
      if (Types::Dependent* dep = dynamic_cast<Types::Dependent*>(param->parameters()[i]))
      {
        if (dep->name().size() == 1 && dep->name()[0] == "*")
        {
          // Find the value of this parameter
          std::string name = parse_name(PTree::nth(PTree::second(PTree::second(pName)), i*2));
          dep->name()[0] = name;
        }
      }

    my_type_formatter->push_scope(my_builder->scope()->name());
    std::string name = my_type_formatter->format(param);
    my_type_formatter->pop_scope();
    std::string primary_name;
    if (pName)
      primary_name = parse_name(pName->is_atom() ? pName : PTree::first(pName));
    clas = my_builder->start_class(my_lineno, type, name, is_template, primary_name);
    // TODO: figure out spec stuff, like what to do with vars, link to
    // original template, etc.
  }
  else if (enc.at(0) == 'Q')
  {
    QName names;
    my_decoder->decodeQualName(names);
    clas = my_builder->start_class(my_lineno, type, names);
  }
  else
  {
    std::string name = my_decoder->decodeName();
    std::string primary_name;
    if (pName)
      primary_name = parse_name(pName->is_atom() ? pName : PTree::first(pName));
    if (pName && !pName->is_atom()) primary_name = parse_name(PTree::first(pName));
    clas = my_builder->start_class(my_lineno, type, name, is_template, primary_name);
  }
  if (sxr_ && pName) sxr_->xref(pName, clas);
  LOG("Translating class '" << clas->name() << "'");

  // Translate the inheritance spec, if present
  if (pInheritance)
  {
    clas->parents() = translate_inheritance_spec(pInheritance);
    my_builder->update_class_base_search();
  }

  add_comments(clas, node->get_comments());

  // Push the impl stack for a cache of func impls
  my_func_impl_stack.push_back(FuncImplVec());
  my_defines_class_or_enum = false;
  // Translate the body of the class
  bool in_template_decl = my_in_template_decl;
  my_in_template_decl = false;
  translate(pBody);
  // Translate any func impls inlined in the class
  FuncImplVec& vec = my_func_impl_stack.back();
  FuncImplVec::iterator iter = vec.begin();
  while (iter != vec.end())
    translate_func_impl_cache(*iter++);
  my_func_impl_stack.pop_back();
  my_builder->end_class();
  my_in_template_decl = in_template_decl;
  my_defines_class_or_enum = true;
}

PTree::TemplateDecl *
Walker::translate_class_template(PTree::TemplateDecl *def, PTree::ClassSpec *node)
{
  STrace trace("Walker::translate_class_template");
  ASG::Parameter::vector* old_params = my_template;
  update_line_number(def);
  my_builder->start_template();
  try
  {
    translate_template_params(PTree::third(def));
    visit(node);
  }
  catch (...)
  {
    my_builder->end_template();
    my_template = old_params;
    throw;
  }
  my_builder->end_template();
  my_template = old_params;
  return def;
}

void Walker::translate_template_params(PTree::Node *params)
{
  STrace trace("Walker::translate_template_params");
  my_template = new ASG::Parameter::vector;
  ASG::Parameter::vector& templ_params = *my_template;
  // Declare some default parameter values - these should not be modified!
  std::string name, value;
  ASG::Parameter::Mods pre_mods, post_mods;
  while (params)
  {
    PTree::Node *param = PTree::first(params);
    if (PTree::is_a(param, Token::ntParameterDecl))
      param = PTree::rest(param);
    else if (PTree::is_a(PTree::first(param), Token::ntTemplateDecl))
      param = PTree::first(param);
    nodeLOG(param);
    if (*PTree::first(param) == "class" || *PTree::first(param) == "typename")
    {
      Types::Dependent* dep = 0;
      // Ensure that there is an identifier (it is optional!)
      if (param->cdr() && PTree::second(param))
      {
        dep = my_builder->create_dependent(parse_name(PTree::second(param)));
        my_builder->add(dep);
      }
      ASG::Parameter::Mods paramtype;
      paramtype.push_back(parse_name(PTree::first(param)));
      templ_params.push_back(new ASG::Parameter(paramtype, dep, post_mods, name, value));
    }
    else if (*PTree::first(param) == "template")
    {
      // A template template parameter.
      Types::Dependent* dep = 0;
      if(PTree::nth(param, 5))
      {
        dep = my_builder->create_dependent(parse_name(PTree::nth(param, 5)));
        my_builder->add(dep);
      }
      ASG::Parameter::Mods paramtype;
      std::string type = "template <" + parse_name(PTree::nth(param, 2)) + "> " + parse_name(PTree::nth(param, 4));
      paramtype.push_back(type);
      templ_params.push_back(new ASG::Parameter(paramtype, dep, post_mods, name, value));
      nodeLOG(param);
    }
    else
    {
      // This parameter specifies a value or something
      // FIXME can do a lot more here..
      LOG("non-type template parameter! approximating..");
      nodeLOG(param);
      PTree::Node *p = PTree::second(param);
      while (p && p->car() && p->car()->is_atom() && (*p->car() == '*' || *p->car() == '&'))
        p = PTree::rest(p);
      std::string name = parse_name(p);
      // FIXME: At this point name will contain the initializer, if it
      //        was present. Search for '=' and assign everything after
      //        that to the value.
      std::string::size_type v = name.find('=');
      if (v != std::string::npos)
      {
	value = name.substr(v + 1);
	while (value[0] == ' ') value.erase(value.begin());
	name = name.substr(0, v - 1);
      }
      Types::Dependent* dep = my_builder->create_dependent(name);
      my_builder->add(dep);
      // Figure out the type of the param
      my_decoder->init(PTree::second(param)->encoded_type());
      Types::Type* param_type = my_decoder->decodeType();
      templ_params.push_back(new ASG::Parameter(pre_mods, param_type, post_mods, name, value));
    }
    // Skip comma
    params = PTree::rest(PTree::rest(params));
  }
  /*
    Types::Template* templ = new Types::Template(decl->name(), decl, templ_params);
    if (ASG::Class* clas = dynamic_cast<ASG::Class*>(decl))
    clas->set_template_type(templ);
    else if (ASG::Function* func = dynamic_cast<ASG::Function*>(decl))
    func->set_template_type(templ);
    std::ostrstream buf;
    buf << "template " << decl->type() << std::ends;
    decl->set_type(buf.str());
  */
}

PTree::TemplateDecl *
Walker::translate_function_template(PTree::TemplateDecl *def, PTree::Node *node)
{
  STrace trace("Walker::translate_function_template");
  nodeLOG(def);
  nodeLOG(node);
  PTree::Declaration *decl = dynamic_cast<PTree::Declaration *>(node);
  if (!decl)
  {
    LOG("Warning: Unknown node type in template");
    nodeLOG(def);
    return 0;
  }

  LOG("Encoded name is: " << node->encoded_name());

  ASG::Parameter::vector* old_params = my_template;
  update_line_number(def);
  my_builder->start_template();
  try
  {
    translate_template_params(PTree::third(def));
    visit(decl);
  }
  catch (...)
  {
    my_builder->end_template();
    my_template = old_params;
    throw;
  }
  my_builder->end_template();
  my_template = old_params;
  return 0;
}

//. Linkage Spec
void Walker::visit(PTree::LinkageSpec *node)
{
  STrace trace("Walker::visit(LinkageSpec*)");
  translate(PTree::third(node));
}

//. Block
void Walker::visit(PTree::Block *node)
{
  STrace trace("Walker::visit(PTree::Block *");
  PTree::Node *rest = PTree::second(node);
  while (rest)
  {
    translate(rest->car());
    rest = rest->cdr();
  }
  PTree::Node *close = PTree::third(node);
  ASG::Declaration *decl;
  decl = my_builder->add_tail_comment(my_lineno);
  add_comments(decl, dynamic_cast<PTree::CommentedAtom *>(close));
}

//. Brace
void Walker::visit(PTree::Brace *node)
{
  STrace trace("Walker::visit(PTree::Brace *)");
  PTree::Node *rest = PTree::second(node);
  while (rest)
  {
    translate(rest->car());
    rest = rest->cdr();
  }
  PTree::Node *close = PTree::third(node);
  ASG::Declaration *decl;
  decl = my_builder->add_tail_comment(my_lineno);
  add_comments(decl, dynamic_cast<PTree::CommentedAtom *>(close));
}

//. TemplateDecl
void Walker::visit(PTree::TemplateDecl *node)
{
  STrace trace("Walker::visit(PTree::TemplateDecl*)");
  my_in_template_decl = true;
  PTree::Node *body = PTree::nth(node, 4);
  PTree::ClassSpec *class_spec = get_class_template_spec(body);

  // FIXME: We skip the template handling if the template argument list is empty.
  //        For classes we can discover it being a specialization since the class name
  //        reveals it (a template-id).
  //        Not so for functions. Thus, we need to find a way to remember that a given
  //        function is a specialization.
  if (PTree::third(node))
  {
    if (class_spec) translate_class_template(node, class_spec);
    else translate_function_template(node, body);
  }
  else // explicit specialization
  {
    if (class_spec) visit(class_spec);
    else visit(static_cast<PTree::Declaration *>(body));
  }
  my_in_template_decl = false;
}

//. A typeof(expr) expression evaluates to the type of 'expr'. This is a GNU
//. GCC extension!
//. Since the OCC parser can't resolve the type properly, we try to do it here
//. and modify the type of the declarations to set it
PTree::Node *Walker::translate_typeof(PTree::Node *spec, PTree::Node *declarations)
{
  STrace trace("Walker::translate_typeof");
  return 0;
  nodeLOG(spec);
  PTree::Encoding enc = PTree::third(spec)->encoded_name();
  LOG("The name is: " << enc);
  LOG("The type is: " << PTree::third(spec)->encoded_type());
  // Find the type referred to by the expression
  if (!my_decoder->isName(enc))
  {
    LOG("typeof is not a simple name: ");
    nodeLOG(spec);
    return 0;
  }
  std::string name = my_decoder->decodeName(enc);
  LOG("name is " << name);
  Types::Type* type = my_lookup->lookupType(name, true);
  // Find the declaration it refers to
  Types::Declared* declared = dynamic_cast<Types::Declared*>(type);
  if (!declared) return 0;
  LOG("Looked up " << declared->name());
  ASG::Declaration* decl = declared->declaration();
  if (!decl) return 0;
  LOG("Declaration is " << decl->name());
  // TODO: make this a visitor and support different things
  if (/*ASG::Function* func =*/ dynamic_cast<ASG::Function*>(decl))
  {
    LOG("decl is a function.");
    while (declarations)
    {
      PTree::Node *declarator = PTree::first(declarations);
      declarations = PTree::rest(declarations);
      
      if (PTree::type_of(declarator) == Token::ntDeclarator)
        ((PTree::Declarator*)declarator)->set_encoded_type("PFv_v");
      else
        LOG("declarator is " << PTree::type_of(declarator));
    }
  }
  else
  {
    LOG("unknown decl type");
  }
  nodeLOG(declarations);
  return 0;
}

void Walker::visit(PTree::Declaration *node)
{
  STrace trace("Walker::visit(PTree::Declaration *)");
#if DEBUG
  trace << node;
#endif
  update_line_number(node);
  // Link any comments added because we are inside a function body
  if (sxr_) find_comments(node);
  my_declaration = node;
  bool in_typedef = my_in_typedef;
  my_in_typedef = false;
  my_store_decl = true;
  PTree::Node *decls = PTree::third(node);

  // Typespecifier may be a class {} etc.
  translate_type_specifier(PTree::second(node));
  // Or it might be a typeof()
  if (PTree::second(node) && PTree::type_of(PTree::second(node)) == Token::ntTypeofExpr)
    translate_typeof(PTree::second(node), decls);

  if (PTree::is_a(decls, Token::ntDeclarator))
  {
    // A single declarator is probably a function impl, but could also be
    // the declarator in an if or switch condition
    PTree::Encoding enc = decls->encoded_type();
    if (!enc.empty())
    {
      // A function may be const, skip the C
      PTree::Encoding::iterator i = enc.begin();
      while (*i == 'C') ++i;
      if (*i != 'F')
      {
        // Not a function
        translate_declarator(decls);
        my_declaration = 0;
        return;
      }
    }
    translate_function_implementation(node);
  }
  else
    // if it is a function prototype or a variable declaration.
    if (!decls->is_atom())        // if it is not ";"
      translate_declarators(decls);
  my_in_typedef = in_typedef;
  my_declaration = 0;
}

PTree::Node *
Walker::translate_declarators(PTree::Node *decls)
{
  STrace trace("Walker::translate_declarators");
  PTree::Node *rest = decls, *p;
  while (rest != 0)
  {
    p = rest->car();
    if (PTree::is_a(p, Token::ntDeclarator))
    {
      translate_declarator(p);
      my_store_decl = false;
    } // if. There is no else..?
    rest = rest->cdr();
    // Skip comma
    if (rest != 0) rest = rest->cdr();
  }
  return 0;
}

//. translate_declarator
//. Function proto:
//.   [ { * | & }* name ( [params] ) ]
//. param:
//.   [ [types] { [ { * | & }* name ] { = value } } ]
PTree::Node *
Walker::translate_declarator(PTree::Node *decl)
{
  // REVISIT: Figure out why this method is so HUGE!
  STrace trace("Walker::translate_declarator");
  // Insert code from occ.cc here
  PTree::Encoding encname = decl->encoded_name();
  PTree::Encoding enctype = decl->encoded_type();
  if (encname.empty() || enctype.empty())
  {
    std::cerr << "encname or enctype empty !" << std::endl;
    return 0;
  }

  try
  {
    // Decide if this is a function or variable
    my_decoder->init(enctype);
    code_iter& iter = my_decoder->iter();
    bool is_const = false;
    while (*iter == 'C')
    {
      ++iter;
      is_const = true;
    }
    if (*iter == 'F')
      return translate_function_declarator(decl, is_const);
    else
      return translate_variable_declarator(decl, is_const);
  }
  catch (const TranslateError& e)
  {
    e.set_node(decl);
    throw;
  }
  return 0;
}


PTree::Node *
Walker::translate_function_declarator(PTree::Node *decl, bool is_const)
{
  STrace trace("Walker::translate_function_declarator");
  ASG::Parameter::vector* is_template = my_template;
  my_template = 0;

  code_iter& iter = my_decoder->iter();
  PTree::Encoding encname = decl->encoded_name();
  
  // This is a function. Skip the 'F'
  ++iter;
  
  // Create parameter objects
  PTree::Node *p_params = PTree::rest(decl);
  while (p_params && p_params->car() && *p_params->car() != '(')
    p_params = PTree::rest(p_params);
  if (!p_params)
  {
    std::string filename;
    unsigned long lineno = my_buffer->origin(decl->begin(), filename);
    std::cerr << "Warning: error finding params for '" 
	      << PTree::reify(decl) 
	      << "\' (at " << filename << ':' << lineno << ')' << std::endl;
    return 0;
  }
  std::vector<ASG::Parameter*> params;
  translate_parameters(PTree::second(p_params), params);
  my_param_cache = params;
  
  // Figure out the return type:
  while (*iter++ != '_') {} // in case of decoding error this is needed
  Types::Type* returnType = my_decoder->decodeType();
  
  // Figure out premodifiers
  std::vector<std::string> premod;
  PTree::Node *p = PTree::first(my_declaration);
  while (p)
  {
    premod.push_back(PTree::reify(p->car()));
    p = PTree::rest(p);
  }

  ASG::Function* func = 0;
  // Find name:
  if (encname.at(0) == 'Q')
  {
    // The name is qualified, which introduces a bit of difficulty
    std::vector<std::string> names;
    my_decoder->init(encname);
    my_decoder->decodeQualName(names);
    names.back() += format_parameters(params);
    // A qual name must already be declared, so find it:
    try
    {
      Types::Named* named_type = my_lookup->lookupType(names, true);
      func = Types::declared_cast<ASG::Function>(named_type);
    }
    catch (const Types::wrong_type_cast &)
    {
      throw ERROR("Qualified function name wasn't a function:" << names);
    }
    // expand param info, since we now have names for them
    std::vector<ASG::Parameter*>::iterator piter = func->parameters().begin();
    std::vector<ASG::Parameter*>::iterator pend = func->parameters().end();
    std::vector<ASG::Parameter*>::iterator new_piter = params.begin();
    while (piter != pend)
    {
      ASG::Parameter* param = *piter++, *new_param = *new_piter++;
      if (!param->name().size() && new_param->name().size())
        param->set_name(new_param->name());
    }
  }
  else
  {
    // Decode the function name
    std::string realname;
    translate_function_name(encname, realname, returnType);
    // Name is same as realname, but with parameters added
    std::string name = realname + format_parameters(params);
    // Append const after params if this is a const function
  
    // Figure out postmodifiers
    std::vector<std::string> postmod;
    if (is_const) 
    {
      name += "const";
      postmod.push_back("const");
    }

    // Create ASG::Function object
    func = my_builder->add_function(my_lineno, name, premod, returnType, postmod, realname, is_template);
    func->parameters() = params;
  }
  add_comments(func, my_declaration);
  add_comments(func, dynamic_cast<PTree::Declarator*>(decl));

  // if storing links, find name
  if (sxr_)
  {
    // Store for use by TranslateFunctionImplementation
    my_function = func;
    
    // Do decl type first
    if (my_store_decl && PTree::second(my_declaration))
      sxr_->xref(PTree::second(my_declaration), returnType);

    p = decl;
    while (p && p->car()->is_atom() && (*p->car() == '*' || *p->car() == '&'))
      p = PTree::rest(p);
    if (p)
      // p should now be at the name
      sxr_->xref(p->car(), func);
  }
  return 0;
}

PTree::Node*
Walker::translate_variable_declarator(PTree::Node *decl, bool is_const)
{
  STrace trace("translate_variable_declarator");
  // Variable declaration. Restart decoding
  PTree::Encoding encname = decl->encoded_name();
  PTree::Encoding enctype = decl->encoded_type();
  my_decoder->init(enctype);
  // Get type
  Types::Type* type = my_decoder->decodeType();
  std::string name;
  if (my_decoder->isName(encname))
    name = my_decoder->decodeName(encname);
  else if (encname.at(0) == 'Q')
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
  std::string var_type = my_builder->scope()->type();
  if (var_type == "class" || var_type == "struct" || var_type == "union")
    var_type = "data member";
  else
  {
    if (var_type == "function")
      var_type = "local";
    var_type += is_const ? " constant" : " variable";
  }
  ASG::Declaration* var;

  if (is_const)
  {
    std::string value;
    if (PTree::length(decl) == 3)
      value = PTree::reify(PTree::nth(decl, 2));
    var = my_builder->add_constant(my_lineno, name, type, var_type, value);
  }
  else
    var = my_builder->add_variable(my_lineno, name, type, false, var_type);
  add_comments(var, my_declaration);
  add_comments(var, dynamic_cast<PTree::Declarator*>(decl));
  // if storing links, find name
  if (sxr_)
  {
    // Do decl type first
    if (my_store_decl && PTree::second(my_declaration))
      sxr_->xref(PTree::second(my_declaration), type);
    
    PTree::Node *p = decl;
    while (p && p->car()->is_atom() && 
           (*p->car() == '*' || *p->car() == '&' || *p->car() == "const"))
    {
      // Link the const keyword
      if (*p->car() == "const")
        sxr_->span(p->car(), "keyword");
      p = PTree::rest(p);
    }
    if (p)
    {
      // p should now be at the name
      sxr_->xref(p->car(), var);
      
      // Next might be '=' then expr
      p = PTree::rest(p);
      if (p && p->car() && *p->car() == '=')
      {
        p = PTree::rest(p);
        if (p && p->car())
          translate(p->car());
      }
    }
  }
  return 0;
}

// Fills the vector of Parameter types by parsing p_params.
void
Walker::translate_parameters(PTree::Node *p_params, std::vector<ASG::Parameter*>& params)
{
  STrace trace("Walker::translate_parameters");
  if (PTree::length(p_params) == 1 && *p_params->car() == "void") return;
  while (p_params)
  {
    // A parameter has a type, possibly a name and possibly a value
    std::string name, value;
    ASG::Parameter::Mods premods, postmods;
    if (*p_params->car() == ',')
      p_params = p_params->cdr();
    PTree::Node *param = PTree::first(p_params);
    // The type is stored in the encoded type string already
    Types::Type* type = my_decoder->decodeType();
    if (!type)
    {
      std::cerr << "Premature end of decoding!" << std::endl;
      break; // 0 means end of encoding
    }
    if (PTree::length(param) == 3)
    {
      PTree::Declarator *decl = static_cast<PTree::Declarator*>(PTree::nth(param, 2));
      name = parse_name(decl->name());
      value = parse_name(decl->initializer());
      // Link type
      if (sxr_ && PTree::nth(param, 1)) sxr_->xref(PTree::nth(param, 1), type);

      // Skip keywords (eg: register) which are Leaves
      PTree::Node *atom = PTree::nth(param, 0);
      if (atom) premods.push_back(parse_name(atom));
    }
    // Add the ASG.Parameter type to the list
    params.push_back(new ASG::Parameter(premods, type, postmods, name, value));
    p_params = PTree::rest(p_params);
  }
}

void Walker::translate_function_name(const PTree::Encoding &encname, std::string& realname, Types::Type*& returnType)
{
  STrace trace("Walker::translate_function_name");
  if (my_decoder->isName(encname))
  {
    if (encname.at(1) == '@')
    {
      // conversion operator
      my_decoder->init(encname);
      my_decoder->iter() += 2;
      returnType = my_decoder->decodeType();
      realname = "("+my_type_formatter->format(returnType)+")";
    }
    else
    {
      // simple name
      realname = my_decoder->decodeName(encname);
      // operator names are missing the 'operator', add it back
      char c = realname[0];
      if (c == '+' || c == '-' || c == '*' || c == '/' || c == '%'
          || c == '^' || c == '&' || c == '!' || c == '=' || c == '<'
          || c == '>' || c == ',' || c == '(' || c == '['
          || (c == '~' && realname[1] == 0))
        realname = "operator"+realname;
    }
  }
  else if (encname.at(0) == 'Q')
  {
    // If a function declaration has a scoped name, then it is not
    // declaring a new function in that scope and can be ignored in
    // the context of synopsis.
    // TODO: maybe needed for syntax stuff?
    return;
  }
  else if (encname.at(0) == 'T')
  {
    // Template specialisation.
    // blah<int, int> is T4blah2ii ---> realname = foo<int,int>
    my_decoder->init(encname);
    code_iter& iter = ++my_decoder->iter();
    realname = my_decoder->decodeName()+"<";
    code_iter tend = iter + (*iter - 0x80u);
    iter++; // For some reason, putting this in prev line causes error with 3.2
    bool first = true;
    // Append type names to realname
    while (iter <= tend)
    {
      /*Types::Type* type = */my_decoder->decodeType();
      if (!first) realname+=",";
      else first=false;
      realname += "type"; //type->ToString();
    }
    realname += ">";
  }
  else
    std::cerr << "Warning: Unknown function name: " << encname << std::endl;
}

//. Class or Enum
PTree::Node*
Walker::translate_type_specifier(PTree::Node *tspec)
{
  STrace trace("Walker::translate_type_specifier");
  PTree::Node *class_spec = get_class_or_enum_spec(tspec);
  if (class_spec) translate(class_spec);
  return 0;
}

void Walker::visit(PTree::Typedef *node)
{
  STrace trace("Walker::visit(Typedef*)");
  my_defines_class_or_enum = false;
  bool in_typedef_back = my_in_typedef;
  my_in_typedef = true;
  if (sxr_) sxr_->span(PTree::first(node), "keyword");
  /* PTree::Node *tspec = */
  translate_type_specifier(PTree::second(node));
  my_declaration = node;
  my_store_decl = true;
  for (PTree::Node *declarator = PTree::third(node); declarator; declarator = PTree::tail(declarator, 2))
    translate_typedef_declarator(declarator->car());
  my_in_typedef = in_typedef_back;
  my_defines_class_or_enum = false;
}

void Walker::translate_typedef_declarator(PTree::Node *node)
{
  STrace trace("Walker::translate_typedef_declarator");
  if (PTree::type_of(node) != Token::ntDeclarator) return;
  PTree::Encoding encname = node->encoded_name();
  PTree::Encoding enctype = node->encoded_type();
  if (encname.empty() || enctype.empty()) return;

  update_line_number(node);

  // Get type of declarator
  my_decoder->init(enctype);
  Types::Type* type = my_decoder->decodeType();
  // Get name of typedef
  std::string name = my_decoder->decodeName(encname);
  // Create typedef object
  ASG::Typedef* tdef = my_builder->add_typedef(my_lineno, name, type, my_defines_class_or_enum);
  add_comments(tdef, dynamic_cast<PTree::Declarator*>(node));
  // if storing links, find name
  if (sxr_)
  {
    if (my_store_decl && PTree::second(my_declaration))
      sxr_->xref(PTree::second(my_declaration), type);

    PTree::Node *p = node;
    // function pointer: [( [* f] )]
    if (p && !p->car()->is_atom() && *p->car()->car() == '(')
      p = PTree::rest(p->car())->car();

    while (p && p->car()->is_atom() && (*p->car() == '*' || *p->car() == '&'))
      p = PTree::rest(p);

    if (p)
      // p should now be at the name
      sxr_->xref(p->car(), tdef);
  }
}

PTree::Node*
Walker::translate_function_implementation(PTree::Node *node)
{
  STrace trace("Walker::translate_function_implementation");
  my_function = 0;
  my_params.clear();
  translate_declarator(PTree::third(node));
  if (!my_filter->should_visit_function_impl(my_file)) return 0;
  if (!my_function)
  {
    std::cerr << "Warning: function was null!" << std::endl;
    return 0;
  }

  FuncImplCache cache;
  cache.func = my_function;
  cache.params = my_param_cache;
  cache.body = PTree::nth(node, 3);

  if (dynamic_cast<ASG::Class*>(my_builder->scope()))
    my_func_impl_stack.back().push_back(cache);
  else
  {
    bool in_template_decl = my_in_template_decl;
    my_in_template_decl = false;
    translate_func_impl_cache(cache);
    my_in_template_decl = in_template_decl;
  }
  return 0;
}

void
Walker::translate_func_impl_cache(const FuncImplCache &cache)
{
  STrace trace("Walker::translate_func_impl_cache");
#if DEBUG
  trace << cache.body;
#endif
  // We create a dummy namespace with the name of the function. Any
  // declarations in the function are added to this dummy namespace. Once we
  // are done, we remove it from the parent scope (its not much use in the
  // documents)
  std::vector<std::string> name = cache.func->name();
  name.back() = "`"+name.back();
  my_builder->start_function_impl(name);
  try
  {
    // Add parameters
    std::vector<ASG::Parameter*>::const_iterator iter, end;
    iter = cache.params.begin();
    end = cache.params.end();
    while (iter != end)
    {
      ASG::Parameter* param = *iter++;
      // Make sure the parameter is named
      if (param->name().size())
        my_builder->add_variable(my_lineno, param->name(), param->type(), false, "parameter");
    }
    // Add 'this' if method
    my_builder->add_this_variable();
    // Translate the function body
    const_cast<PTree::Node *>(cache.body)->accept(this);
  }
  catch (...)
  {
    LOG("Cleaning up func impl cache");
    my_builder->end_function_impl();
    throw;
  }
  my_builder->end_function_impl();
}

void Walker::visit(PTree::AccessSpec *node)
{
  STrace trace("Walker::visit(PTree::AccessSpec*)");
  ASG::Access axs = ASG::Default;
  switch (PTree::type_of(PTree::first(node)))
  {
    case Token::PUBLIC:
      axs = ASG::Public;
      break;
    case Token::PROTECTED:
      axs = ASG::Protected;
      break;
    case Token::PRIVATE:
      axs = ASG::Private;
      break;
  }
  update_line_number(node);
  if (PTree::Node *comments = node->get_comments())
  {
    ASG::Builtin *builtin = my_builder->add_tail_comment(my_lineno);
    add_comments(builtin, comments);
  }
  my_builder->set_access(axs);
  if (sxr_) sxr_->span(PTree::first(node), "keyword");
}

/* Enum Spec
 *  
 */
void Walker::visit(PTree::EnumSpec *node)
{
  STrace trace("Walker::visit(PTree::EnumSpec*)");
  //update_line_number(spec);
  my_defines_class_or_enum = true;
  if (sxr_) sxr_->span(PTree::first(node), "keyword");
  std::string name;
  if (PTree::second(node))
    name = PTree::reify(PTree::second(node));
  else
  {
    my_decoder->init(node->encoded_name());
    name = my_decoder->decodeName();
  }
  update_line_number(node);
  int enum_lineno = my_lineno;
  // Parse enumerators
  std::vector<ASG::Enumerator*> enumerators;
  PTree::Node *penum = PTree::second(PTree::third(node));
  ASG::Enumerator *enumor;
  while (penum)
  {
    update_line_number(penum);
    PTree::Node *penumor = PTree::first(penum);
    if (penumor->is_atom())
    {
      // Just a name
      enumor = my_builder->add_enumerator(my_lineno, PTree::reify(penumor), "");
      add_comments(enumor, static_cast<PTree::CommentedAtom *>(penumor)->get_comments());
      if (sxr_) sxr_->xref(penumor, enumor);
    }
    else
    {
      // Name = Value
      std::string name = PTree::reify(PTree::first(penumor));
      std::string value;
      if (PTree::length(penumor) == 3)
        value = PTree::reify(PTree::third(penumor));
      enumor = my_builder->add_enumerator(my_lineno, name, value);
      add_comments(enumor, dynamic_cast<PTree::CommentedAtom *>(PTree::first(penumor)));
      if (sxr_) sxr_->xref(PTree::first(penumor), enumor);
    }
    enumerators.push_back(enumor);
    penum = PTree::rest(penum);
    // Skip comma
    if (penum && penum->car() && *penum->car() == ',') penum = PTree::rest(penum);
  }
  PTree::Node *close = PTree::third(PTree::third(node));
  enumor = new ASG::Enumerator(my_file, my_lineno, "dummy", my_dummyname, "");
  add_comments(enumor, static_cast<PTree::CommentedAtom *>(close));
  enumerators.push_back(enumor);
  
  // Create ASG.Enum object
  ASG::Enum* theEnum = my_builder->add_enum(enum_lineno,name,enumerators);
  add_comments(theEnum, my_declaration);
  if (my_declaration)
  {
    // Enum declared inside declaration. Comments for the declaration
    // belong to the enum. This is policy. #TODO review policy
    //my_declaration->SetComments(0); ?? typedef doesn't have comments?
  }
  if (sxr_) sxr_->xref(PTree::second(node), theEnum);
  my_defines_class_or_enum = true;
}

void Walker::visit(PTree::UsingDirective *node)
{
  STrace trace("Walker::visit(PTree::UsingDirective*)");
  update_line_number(node);

  if (sxr_) sxr_->span(PTree::first(node), "keyword");
  PTree::Node *p = PTree::rest(node);
  if (sxr_) sxr_->span(PTree::first(p), "keyword");
  // Find namespace to alias
  p = PTree::rest(p);

  // Find name that we are looking up, and make a new ptree list for linking it
  p = p->car(); // p now points to the 'PTree::Name' child of 'PTree::Using'
  PTree::Node *p_name = PTree::snoc(0, p->car());
  QName name;
  if (*PTree::first(p) == "::")
    // Eg; "using ::memcpy;" Indicate global scope with empty first
    name.push_back("");
  else
  {
    name.push_back(parse_name(PTree::first(p)));
    p = PTree::rest(p);
  }
  while (p && *PTree::first(p) == "::")
  {
    p_name = PTree::snoc(p_name, p->car()); // Add '::' to p_name
    p = PTree::rest(p);
    name.push_back(parse_name(PTree::first(p)));
    p_name = PTree::snoc(p_name, p->car()); // Add identifier to p_name
    p = PTree::rest(p);
  }

  // Resolve and link name
  try
  {
    Types::Named* type = my_lookup->lookupType(name);
    if (sxr_) sxr_->xref(p_name, type);
    // Check for '=' alias
    // Huh ? '=' isn't valid within a 'using' directive or declaration
    if (p && *PTree::first(p) == "=")
    {
      p = PTree::rest(p);
      std::string alias = parse_name(PTree::first(p));
      my_builder->add_aliased_using_namespace(type, alias);
    }
    else my_builder->add_using_directive(my_lineno, type);
  }
  catch (const TranslateError& e)
  {
    LOG("Oops!");
    e.set_node(node);
    throw;
  }
}

void Walker::visit(PTree::UsingDeclaration *node)
{
  STrace trace("Walker::visit(PTree::UsingDeclaration*)");
  if (sxr_) sxr_->span(PTree::first(node), "keyword");
  PTree::Node *p = PTree::rest(node);
  // Find name that we are looking up, and make a new ptree list for linking it
  PTree::Node *p_name = PTree::snoc(0, p->car());
  QName name;
  if (*PTree::first(p) == "::")
    // Eg; "using ::memcpy;" Indicate global scope with empty first
    name.push_back("");
  else
  {
    name.push_back(parse_name(PTree::first(p)));
    p = PTree::rest(p);
  }
  while (p && *PTree::first(p) == "::")
  {
    p_name = PTree::snoc(p_name, p->car()); // Add '::' to p_name
    p = PTree::rest(p);
    name.push_back(parse_name(PTree::first(p)));
    p_name = PTree::snoc(p_name, p->car()); // Add identifier to p_name
    p = PTree::rest(p);
  }

  // Resolve and link name
  try
  {
    Types::Named* type = my_lookup->lookupType(name);

    if (sxr_) sxr_->xref(p_name, type);
    // Let builder do all the work
    my_builder->add_using_declaration(my_lineno, type);
  }
  catch (const TranslateError& e)
  {
    LOG("Oops!");
    e.set_node(node);
    throw;
  }
}

void Walker::visit(PTree::ReturnStatement *node)
{
  STrace trace("Walker::visit(PTree::ReturnStatement*)");
  if (!sxr_) return;

  // Link 'return' keyword
  sxr_->span(PTree::first(node), "keyword");

  // Translate the body of the return, if present
  if (PTree::length(node) == 3) translate(PTree::second(node));
}

void Walker::visit(PTree::InfixExpr *node)
{
  STrace trace("Walker::visit(PTree::Infix*)");
  translate(PTree::first(node));
  Types::Type* left_type = my_type;
  translate(PTree::third(node));
  Types::Type* right_type = my_type;
  std::string oper = parse_name(PTree::second(node));
  TypeIdFormatter tf;
  LOG("BINARY-OPER: " << tf.format(left_type) << " " << oper << " " << tf.format(right_type));
  nodeLOG(node);
  if (!left_type || !right_type)
  {
    my_type = 0;
    return;
  }
  // Lookup an appropriate operator
  ASG::Function* func = my_lookup->lookupOperator(oper, left_type, right_type);
  if (func)
  {
    my_type = func->return_type();
    if (sxr_) sxr_->xref(PTree::second(node), func->declared());
  }
  return;
}

void Walker::translate_variable(PTree::Node *spec)
{
  // REVISIT: Figure out why this is so long!!!
  STrace trace("Walker::TranslateVariable");
  if (sxr_) find_comments(spec);
  try
  {
    PTree::Node *name_spec = spec;
    Types::Named* type;
    QName scoped_name;
    if (!spec->is_atom())
    {
      // Must be a scoped name.. iterate through the scopes
      // stop when spec is at the last name
      //nodeLOG(spec);
      // If first node is '::' then reset my_scope to the global scope
      if (*PTree::first(spec) == "::")
      {
        scoped_name.push_back("");
        spec = PTree::rest(spec);
      }
      while (PTree::length(spec) > 2)
      {
        scoped_name.push_back(parse_name(PTree::first(spec)));
        /*
          if (!type) { throw nodeERROR(spec, "scope '" << parse_name(spec->First()) << "' not found!"); }
          try { my_scope = Types::declared_cast<ASG::Scope>(type); }
          catch (const Types::wrong_type_cast&) { throw nodeERROR(spec, "scope '"<<parse_name(spec->First())<<"' found but not a scope!"); }
          // Link the scope name
          if (sxr_) sxr_->xref(spec->First(), my_scope->declared());
        */
        spec = PTree::rest(PTree::rest(spec));
      }
      spec = PTree::first(spec);
      // Check for 'operator >>' type syntax:
      if (!spec->is_atom() && PTree::length(spec) == 2 && *PTree::first(spec) == "operator")
      {
        // Name lookup is done based on only the operator type, so
        // skip the 'operator' node
        spec = PTree::second(spec);
      }
      scoped_name.push_back(parse_name(spec));
    }
    std::string name = parse_name(spec);
    if (my_postfix_flag == Postfix_Var)
    {
      // Variable lookup. my_type will be the vtype
      /*cout << "my_scope is " << (my_scope ? my_type_formatter->format(my_scope->declared()) : "global") << endl;*/
      if (!scoped_name.empty())
        type = my_lookup->lookupType(scoped_name, true, my_scope);
      else if (my_scope)
        type = my_lookup->lookupType(name, my_scope);
      else
        type = my_lookup->lookupType(name);
      if (!type)
      {
        throw nodeERROR(spec, "variable '" << name << "' not found!");
      }
      // Now find vtype (throw wrong_type_cast if not a variable)
      try
      {
        Types::Declared& declared = dynamic_cast<Types::Declared&>(*type);
        // The variable could be a Variable or Enumerator
        ASG::Variable* var;
        ASG::Enumerator* enumor;
        if ((var = dynamic_cast<ASG::Variable*>(declared.declaration())) != 0)
        {
          // It is a variable
          my_type = var->vtype();
          // Store a link to the variable itself (not its type)
          if (sxr_) sxr_->xref(name_spec, type);
          /*cout << "var type name is " << my_type_formatter->format(my_type) << endl;*/
        }
        else if ((enumor = dynamic_cast<ASG::Enumerator*>(declared.declaration())) != 0)
        {
          // It is an enumerator
          my_type = 0; // we have no use for enums in type code
          // But still a link is needed
          if (sxr_) sxr_->xref(name_spec, type);
          /*cout << "enum type name is " << my_type_formatter->format(type) << endl;*/
        }
        else
        {
          throw nodeERROR(name_spec, "var was not a Variable nor Enumerator!");
        }
      }
      catch (const std::bad_cast &)
      {
        if (dynamic_cast<Types::Unknown*>(type))
          throw nodeERROR(spec, "variable '" << name << "' was an Unknown type!");
        if (dynamic_cast<Types::Base*>(type))
          throw nodeERROR(spec, "variable '" << name << "' was a Base type!");
        throw nodeERROR(spec, "variable '" << name << "' wasn't a declared type!");
      }
    }
    else
    {
      // Function lookup. my_type will be returnType. params are in my_params
      ASG::Scope* scope = my_scope ;
      if (!scope) scope = my_builder->scope();
      // if (!scoped_name.empty()) func = my_lookup->lookupFunc(scoped_name, scope, my_params);
      ASG::Function* func = my_lookup->lookupFunc(name, scope, my_params);
      if (!func)
      {
        throw nodeERROR(name_spec, "Warning: function '" << name << "' not found!");
      }
      // Store a link to the function name
      if (sxr_)
        sxr_->xref(name_spec, func->declared(), SXRGenerator::FunctionCall);
      // Now find returnType
      my_type = func->return_type();
    }
  }
  catch(const TranslateError& e)
  {
    my_scope = 0;
    my_type = 0;
    e.set_node(spec);
    throw;
  }
  catch(const Types::wrong_type_cast &)
  {
    throw nodeERROR(spec, "wrong type error in TranslateVariable!");
  }
  catch(...)
  {
    throw nodeERROR(spec, "unknown error in TranslateVariable!");
  }
  my_scope = 0;
}

void Walker::translate_function_args(PTree::Node *args)
{
  // args: [ arg (, arg)* ]
  while (PTree::length(args))
  {
    PTree::Node *arg = PTree::first(args);
    // Translate this arg, TODO: my_params would be better as a vector<Type*>
    my_type = 0;
    translate(arg);
    my_params.push_back(my_type);
    // Skip over arg and comma
    args = PTree::rest(PTree::rest(args));
  }
}

void Walker::visit(PTree::FuncallExpr *node) 	// and fstyle cast
{
  STrace trace("Walker::visit(PTree::FuncallExpr*)");
  // TODO: figure out how to deal with fstyle casts.. does it only apply to
  // base types? eg: int(4.0) ?
  // This is similar to TranslateVariable, except we have to check params..
  // doh! That means more my_type nastiness
  //
  LOG(node);
  // In translating the postfix the last var should be looked up as a
  // function. This means we have to find the args first, and store them in
  // my_params as a hint
  Types::Type::vector save_params = my_params;
  my_params.clear();
  try
  {
    translate_function_args(PTree::third(node));
  }
  catch (...)
  {
    // Restore params before rethrowing exception
    my_params = save_params;
    throw;
  }

  Postfix_Flag save_flag = my_postfix_flag;
  try
  {
    my_postfix_flag = Postfix_Func;
    translate(PTree::first(node));
  }
  catch (...)
  {
    // Restore params and flag before rethrowing exception
    my_params = save_params;
    my_postfix_flag = save_flag;
    throw;
  }

  // Restore my_params since we're done with it now
  my_params = save_params;
  my_postfix_flag = save_flag;
}

void Walker::visit(PTree::ExprStatement *node)
{
  STrace trace("Walker::visit(ExprStatement*)");
  translate(PTree::first(node));
}

void Walker::visit(PTree::UnaryExpr *node)
{
  STrace trace("Walker::visit(UnaryExpr*)");
  if (sxr_) find_comments(node);
  // TODO: lookup unary operator
  translate(PTree::second(node));
}

void Walker::visit(PTree::AssignExpr *node)
{
  STrace trace("Walker::visit(AssignExpr*)");
  // TODO: lookup = operator
  my_type = 0;
  translate(PTree::first(node));
  Types::Type* ret_type = my_type;
  translate(PTree::third(node));
  my_type = ret_type;
}

//. Resolves the final type of any given Type. For example, it traverses
//. typedefs and parameterized types, and resolves unknowns by looking them up.
class TypeResolver : public Types::Visitor
{
  // TODO: Move to separate file???
public:
  //. Constructor - needs a Builder to resolve unknowns with
  TypeResolver(Builder* b) { my_builder = b;}
  
  //. Resolves the given type object
  Types::Type* resolve(Types::Type* t)
  {
    my_type = t;
    t->accept(this);
    return my_type;
  }

  //. Tries to resolve the given type object to a Scope
  ASG::Scope* scope(Types::Type* t) throw (Types::wrong_type_cast, TranslateError)
  {
    return Types::declared_cast<ASG::Scope>(resolve(t));
  }

  //. Looks up the unknown type for a fresh definition
  void visit_unknown(Types::Unknown* t)
  {
    my_type = my_builder->lookup()->resolveType(t);
    if (!dynamic_cast<Types::Unknown*>(my_type)) my_type->accept(this);
  }
  
  //. Recursively processes the aliased type
  void visit_modifier(Types::Modifier* t) { t->alias()->accept(this);}

  //. Checks for typedefs and recursively processes them
  void visit_declared(Types::Declared* t)
  {
    ASG::Typedef* tdef = dynamic_cast<ASG::Typedef*>(t->declaration());
    if (tdef) tdef->alias()->accept(this);
    else my_type = t;
  }

  //. Processes the template type
  void visit_parameterized(Types::Parameterized* t)
  {
    if (t->template_id()) t->template_id()->accept(this);
  }

protected:
  Builder* my_builder; //.< A reference to the builder object
  Types::Type* my_type; //.< The type to return
};

void Walker::visit(PTree::ArrowMemberExpr *node)
{
  STrace trace("Walker::visit(ArrowMember*)");
  my_type = 0;
  my_scope = 0;
  Postfix_Flag save_flag = my_postfix_flag;
  my_postfix_flag = Postfix_Var;
  translate(PTree::first(node));
  my_postfix_flag = save_flag;
  // my_type should be a modifier to a declared to a class. Throw bad_cast if not
  if (!my_type)
  {
    throw nodeERROR(node, "Unable to resolve type of LHS of ->");
  }
  try
  {
    my_scope = TypeResolver(my_builder).scope(my_type);
  }
  catch (const Types::wrong_type_cast&)
  {
    throw nodeERROR(node, "LHS of -> was not a scope!");
  }
  // Find member, my_type becomes the var type or func returnType
  translate(PTree::third(node));
  my_scope = 0;
}

void Walker::visit(PTree::DotMemberExpr *node)
{
  STrace trace("Walker::visit(DotMember*)");
  my_type = 0;
  my_scope = 0;
  Postfix_Flag save_flag = my_postfix_flag;
  my_postfix_flag = Postfix_Var;
  translate(PTree::first(node));
  my_postfix_flag = save_flag;
  LOG(parse_name(PTree::first(node)) << " resolved to " << my_type_formatter->format(my_type));
  // my_type should be a declared to a class
  if (!my_type)
  {
    throw nodeERROR(node, "Unable to resolve type of LHS of .");
  }
  LOG("resolving type to scope");
  // Check for reference type
  try
  {
    my_scope = TypeResolver(my_builder).scope(my_type);
  }
  catch (const Types::wrong_type_cast &)
  {
    throw nodeERROR(node, "Warning: LHS of . was not a scope: " << my_type_formatter->format(my_type));
  }
  // Find member, my_type becomes the var type or func returnType
  LOG("translating third");
  translate(PTree::third(node));
  my_scope = 0;
}

void Walker::visit(PTree::IfStatement *node)
{
  STrace trace("Walker::visit(IfStatement*)");
  if (sxr_) find_comments(node);
  if (sxr_) sxr_->span(PTree::first(node), "keyword");
  // Start a temporary namespace, in case expr is a declaration
  my_builder->start_namespace("if", NamespaceUnique);
  // Parse expression
  translate(PTree::third(node));
  // Store a copy of any declarations for use in the else block
  std::vector<ASG::Declaration*> decls = my_builder->scope()->declarations();
  // Translate then-statement. If a block then we avoid starting a new ns
  PTree::Node *stmt = PTree::nth(node, 4);
  if (stmt && PTree::first(stmt) && *PTree::first(stmt) == '{') visit((PTree::Brace *)stmt);
  else translate(stmt);
  // End the block and check for else
  my_builder->end_namespace();
  if (PTree::length(node) == 7)
  {
    if (sxr_) sxr_->span(PTree::nth(node, 5), "keyword");
    ASG::Namespace* ns = my_builder->start_namespace("else", NamespaceUnique);
    ns->declarations().insert(ns->declarations().begin(), decls.begin(), decls.end());
    // Translate else statement, same deal as above
    stmt = PTree::nth(node, 6);
    if (stmt && PTree::first(stmt) && *PTree::first(stmt) == '{') visit((PTree::Brace*)stmt);
    else translate(stmt);
    my_builder->end_namespace();
  }
}

void Walker::visit(PTree::SwitchStatement *node)
{
  STrace trace("Walker::visit(SwitchStatement*)");
  if (sxr_) find_comments(node);
  if (sxr_) sxr_->span(PTree::first(node), "keyword");
  my_builder->start_namespace("switch", NamespaceUnique);
  // Parse expression
  translate(PTree::third(node));
  // Translate statement. If a block then we avoid starting a new ns
  PTree::Node *stmt = PTree::nth(node, 4);
  if (stmt && PTree::first(stmt) && *PTree::first(stmt) == '{') visit((PTree::Brace *)stmt);
  else translate(stmt);
  // End the block and check for else
  my_builder->end_namespace();
}

void Walker::visit(PTree::CaseStatement *node)
{
  STrace trace("Walker::visit(Case*)");
  if (sxr_) find_comments(node);
  if (sxr_) sxr_->span(PTree::first(node), "keyword");
  translate(PTree::second(node));
  translate(PTree::nth(node, 3));
}

void Walker::visit(PTree::DefaultStatement *node)
{
  STrace trace("Walker::visit(DefaultStatement*)");
  if (sxr_) find_comments(node);
  if (sxr_) sxr_->span(PTree::first(node), "keyword");
  translate(PTree::third(node));
}

void Walker::visit(PTree::BreakStatement *node)
{
  STrace trace("Walker::visit(Break*)");
  if (sxr_) find_comments(node);
  if (sxr_) sxr_->span(PTree::first(node), "keyword");
}

void Walker::visit(PTree::ForStatement *node)
{
  STrace trace("Walker::visit(For*)");
  if (sxr_) find_comments(node);
  if (sxr_) sxr_->span(PTree::first(node), "keyword");
  my_builder->start_namespace("for", NamespaceUnique);
  // Parse expressions
  translate(PTree::third(node));
  translate(PTree::nth(node, 3));
  translate(PTree::nth(node, 5));
  // Translate statement. If a block then we avoid starting a new ns
  PTree::Node *stmt = PTree::nth(node, 7);
  if (stmt && PTree::first(stmt) && *PTree::first(stmt) == '{')
    visit((PTree::Brace *)stmt);
  else translate(stmt);
  // End the block
  my_builder->end_namespace();
}

void Walker::visit(PTree::WhileStatement *node)
{
  STrace trace("Walker::visit(While*)");
  if (sxr_) find_comments(node);
  if (sxr_) sxr_->span(PTree::first(node), "keyword");
  my_builder->start_namespace("while", NamespaceUnique);
  // Parse expression
  translate(PTree::third(node));
  // Translate statement. If a block then we avoid starting a new ns
  PTree::Node *stmt = PTree::nth(node, 4);
  if (stmt && PTree::first(stmt) && *PTree::first(stmt) == '{')
    visit((PTree::Brace *)stmt);
  else translate(stmt);
  // End the block and check for else
  my_builder->end_namespace();
}

void Walker::visit(PTree::PostfixExpr *node)
{
  STrace trace("Walker::visit(Postfix*)");
  translate(PTree::first(node));
}

void Walker::visit(PTree::ParenExpr *node)
{
  STrace trace("Walker::visit(Paren*)");
  if (sxr_) find_comments(node);
  translate(PTree::second(node));
}

void Walker::visit(PTree::CastExpr *node)
{
  STrace trace("Walker::visit(Cast*)");
  if (sxr_) find_comments(node);
  PTree::Node *type_expr = PTree::second(node);
  //Translate(type_expr->First());
  PTree::Encoding enc = PTree::second(type_expr)->encoded_type();
  if (!enc.empty())
  {
    my_decoder->init(enc);
    my_type = my_decoder->decodeType();
    my_type = TypeResolver(my_builder).resolve(my_type);
    if (my_type && sxr_)
      sxr_->xref(PTree::first(type_expr), my_type);
  }
  else my_type = 0;
  translate(PTree::nth(node, 3));
}

void Walker::visit(PTree::TryStatement *node)
{
  STrace trace("Walker::visit(Try*)");
  if (sxr_) find_comments(node);
  if (sxr_) sxr_->span(PTree::first(node), "keyword");
  my_builder->start_namespace("try", NamespaceUnique);
  translate(PTree::second(node));
  my_builder->end_namespace();
  for (int n = 2; n < PTree::length(node); n++)
  {
    // [ catch ( arg ) [{}] ]
    PTree::Node *catch_node = PTree::nth(node, n);
    if (sxr_) sxr_->span(PTree::first(catch_node), "keyword");
    my_builder->start_namespace("catch", NamespaceUnique);
    PTree::Node *arg = PTree::third(catch_node);
    if (PTree::length(arg) == 2)
    {
      // Get the arg type
      my_decoder->init(PTree::second(arg)->encoded_type());
      Types::Type* arg_type = my_decoder->decodeType();
      // Link the type
      Types::Type* arg_link = TypeResolver(my_builder).resolve(arg_type);
      if (sxr_) sxr_->xref(PTree::first(arg), arg_link);
      // Create a declaration for the argument
      if (PTree::second(arg))
      {
	PTree::Encoding enc = PTree::second(arg)->encoded_name();
	if (!enc.empty())
	{
	  std::string name = my_decoder->decodeName(enc);
	  my_builder->add_variable(my_lineno, name, arg_type, false, "exception");
	}
      }
    }
    // Translate contents of 'catch' block
    translate(PTree::nth(catch_node, 4));
    my_builder->end_namespace();
  }
}

void Walker::visit(PTree::ArrayExpr *node)
{
  STrace trace("Walker::visit(ArrayExpr*)");
  translate(PTree::first(node));
  Types::Type* object = my_type;
  
  translate(PTree::third(node));
  Types::Type* arg = my_type;
  
  if (!object || !arg)
  {
    my_type = 0;
    return;
  }
  // Resolve final type
  try
  {
    TypeIdFormatter tf;
    LOG("ARRAY-OPER: " << tf.format(object) << " [] " << tf.format(arg));
    ASG::Function* func;
    my_type = my_lookup->arrayOperator(object, arg, func);
    if (func && sxr_)
    {
      // Link the [ and ] to the function operator used
      sxr_->xref(PTree::nth(node, 1), func->declared());
      sxr_->xref(PTree::nth(node, 3), func->declared());
    }
  }
  catch (const TranslateError& e)
  {
    e.set_node(node);
    throw;
  }
}

void Walker::visit(PTree::CondExpr *node)
{
  STrace trace("Walker::visit(Cond*)");
  translate(PTree::nth(node, 0));
  translate(PTree::nth(node, 2));
  translate(PTree::nth(node, 4));
}

void Walker::visit(PTree::Kwd::This *node)
{
  STrace trace("Walker::visit(This*)");
  if (sxr_) find_comments(node);
  if (sxr_) sxr_->span(node, "keyword");
  // Set my_type to type of 'this', stored in the name lookup for this func
  my_type = my_lookup->lookupType("this");
}

void Walker::visit(PTree::TemplateInstantiation *)
{
  STrace trace("Walker::visit(TemplateInstantiation*) NYI");
}

void Walker::visit(PTree::ExternTemplate *)
{
  STrace trace("Walker::visit(ExternTemplate*) NYI");
}

void Walker::visit(PTree::MetaclassDecl *)
{
  STrace trace("Walker::visit(MetaclassDecl*) NYI");
}

PTree::Node *Walker::translate_storage_specifiers(PTree::Node *)
{
  STrace trace("Walker::translate_storage_specifiers NYI");
  return 0;
}

PTree::Node *Walker::translate_function_body(PTree::Node *)
{
  STrace trace("Walker::translate_function_body NYI");
  return 0;
}

void Walker::visit(PTree::AccessDecl *node)
{
  STrace trace("Walker::visit(AccessDecl*) NYI");
#if DEBUG
  trace << node;
#endif
  if (sxr_) find_comments(node);
}

void Walker::visit(PTree::UserAccessSpec *node)
{
  STrace trace("Walker::visist(UserAccessSpec*) NYI");
#if DEBUG
  trace << node;
#endif
  if (sxr_) find_comments(node);
}

void Walker::visit(PTree::DoStatement *node)
{
  STrace trace("Walker::visit(Do*) NYI");
  if (sxr_)
  {
    find_comments(node);
    sxr_->span(PTree::first(node), "keyword");
    sxr_->span(PTree::third(node), "keyword");
  }
  // Translate block
  my_builder->start_namespace("do", NamespaceUnique);
  // Translate statement. If a block then we avoid starting a new ns
  PTree::Node *stmt = PTree::second(node);
  if (stmt && PTree::first(stmt) && *PTree::first(stmt) == '{')
    visit((PTree::Brace*)stmt);
  else translate(stmt);
  // End the block and check for else
  my_builder->end_namespace();
  // Translate the while condition
  translate(PTree::nth(node, 4));
}

void Walker::visit(PTree::ContinueStatement *node)
{
  STrace trace("Walker::visit(Continue*) NYI");
  if (sxr_) find_comments(node);
  if (sxr_) sxr_->span(PTree::first(node), "keyword");
}

void Walker::visit(PTree::GotoStatement *node)
{
  STrace trace("Walker::visist(Goto*) NYI");
#if DEBUG
  trace << node;
#endif
  if (sxr_) find_comments(node);
}

void Walker::visit(PTree::LabelStatement *node)
{
  STrace trace("Walker::visit(Label*) NYI");
#if DEBUG
  trace << node;
#endif
  if (sxr_) find_comments(node);
}

void Walker::visit(PTree::Expression *node)
{
  STrace trace("Walker::visit(Expression*)");
#if DEBUG
  trace << node;
#endif
  PTree::Node *node2 = node;
  while (node2)
  {
    translate(PTree::first(node2));
    node2 = PTree::rest(node2);
    if (node2) node2 = PTree::rest(node2);
  }
}

void Walker::visit(PTree::PmExpr *node)
{
  STrace trace("Walker::visit(Pm*) NYI");
}

void Walker::visit(PTree::ThrowExpr *node)
{
  STrace trace("Walker::visit(Throw*)");
#if DEBUG
  trace << node;
#endif
  if (sxr_) find_comments(node);
  if (sxr_) sxr_->span(PTree::first(node), "keyword");
  translate(PTree::second(node));
}

void Walker::visit(PTree::SizeofExpr *node)
{
  STrace trace("Walker::visit(Sizeof*)");
#if DEBUG
  trace << node;
#endif
  if (sxr_) find_comments(node);
  if (sxr_) sxr_->span(PTree::first(node), "keyword");
  // TODO: find the type for highlighting, and figure out what the ??? is
  my_type = my_lookup->lookupType("int");
}

void Walker::visit(PTree::NewExpr *node)
{
  STrace trace("Walker::visit(New*) NYI");
#if DEBUG
  trace << node;
#endif
  if (sxr_) find_comments(node);
}

PTree::Node *Walker::translate_new3(PTree::Node *node)
{
  STrace trace("Walker::translate_new3 NYI");
#if DEBUG
  trace << node;
#endif
  if (sxr_) find_comments(node);
  return 0;
}

void Walker::visit(PTree::DeleteExpr *node)
{
  STrace trace("Walker::visit(DeleteExpr*)");
#if DEBUG
  trace << node;
#endif
  if (sxr_) find_comments(node);
  if (sxr_) sxr_->span(PTree::first(node), "keyword");
  translate(PTree::second(node));
}

void Walker::visit(PTree::FstyleCastExpr *node)
{
  STrace trace("Walker::visit(FstyleCast*) NYI");
#if DEBUG
  trace << node;
#endif
  if (sxr_) find_comments(node);
  my_type = 0;
  //Translate(node->Third()); <-- unknown ptree???? FIXME
  my_decoder->init(node->encoded_type());
  my_type = my_decoder->decodeType();
  // TODO: Figure out if should have called a function for this
}

void Walker::visit(PTree::UserStatementExpr *node)
{
  STrace trace("Walker::visit(UserStatement*) NYI");
}

void Walker::visit(PTree::StaticUserStatementExpr *node)
{
  STrace trace("Walker::visit(StaticUserStatement*) NYI");
}
