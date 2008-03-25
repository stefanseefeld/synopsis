//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include "ASGTranslator.hh"
#include <Synopsis/Trace.hh>
#include <Synopsis/PTree/Writer.hh> // for PTree::reify
#include <Support/path.hh>

using Synopsis::Token;
using Synopsis::Trace;
namespace PT = Synopsis::PTree;

namespace
{
inline bpl::dict annotations(bpl::object o) { return bpl::extract<bpl::dict>(o.attr("annotations"));}
inline bpl::list comments(bpl::object o) { return bpl::extract<bpl::list>(annotations(o).get("comments"));}
}


ASGTranslator::ASGTranslator(std::string const &filename,
			     std::string const &base_path, bool primary_file_only,
			     bpl::object ir, bool v, bool d)
  : asg_module_(bpl::import("Synopsis.ASG")),
    sf_module_(bpl::import("Synopsis.SourceFile")),
    files_(bpl::extract<bpl::dict>(ir.attr("files"))()),
    raw_filename_(filename),
    base_path_(base_path),
    primary_file_only_(primary_file_only),
    lineno_(0),
    verbose_(v),
    debug_(d),
    defines_class_or_enum_(false)
{
  Trace trace("ASGTranslator::ASGTranslator", Trace::TRANSLATION);

  bpl::object qname_module = bpl::import("Synopsis.QualifiedName");
  qname_ = qname_module.attr("QualifiedCxxName");
  bpl::object asg = ir.attr("asg");
  types_ = bpl::extract<bpl::dict>(asg.attr("types"))();
  declarations_ = bpl::extract<bpl::list>(asg.attr("declarations"))();
  // determine canonical filenames
  std::string long_filename = make_full_path(raw_filename_);
  std::string short_filename = make_short_path(raw_filename_, base_path_);

  bpl::object file = files_.get(short_filename);
  if (file)
    file_ = file;
  else
  {
    file_ = sf_module_.attr("SourceFile")(short_filename, long_filename, "C");
    files_[short_filename] = file_;
  }
  annotations(file_)["primary"] = true;

#define DECLARE_BUILTIN_TYPE(T) types_[qname(T)] = asg_module_.attr("BuiltinTypeId")("C", qname(T))

  DECLARE_BUILTIN_TYPE("char");
  DECLARE_BUILTIN_TYPE("short");
  DECLARE_BUILTIN_TYPE("int");
  DECLARE_BUILTIN_TYPE("long");
  DECLARE_BUILTIN_TYPE("unsigned");
  DECLARE_BUILTIN_TYPE("unsigned long");
  DECLARE_BUILTIN_TYPE("float");
  DECLARE_BUILTIN_TYPE("double");
  DECLARE_BUILTIN_TYPE("void");
  DECLARE_BUILTIN_TYPE("...");
  DECLARE_BUILTIN_TYPE("long long");
  DECLARE_BUILTIN_TYPE("long double");
  // some GCC extensions...
  DECLARE_BUILTIN_TYPE("__builtin_va_list");

#undef DECLARE_BUILTIN_TYPE
}

void ASGTranslator::translate(PT::Node *ptree, Buffer &buffer)
{
  Trace trace("ASGTranslator::translate", Trace::TRANSLATION);
  buffer_ = &buffer;
  ptree->accept(this);
}

void ASGTranslator::visit(PT::List *node)
{
  if (node->car()) node->car()->accept(this);
  if (node->cdr()) node->cdr()->accept(this);
}

void ASGTranslator::visit(PT::Declarator *declarator)
{
  Trace trace("ASGTranslator::visit(PT::Declarator *)", Trace::TRANSLATION);
  trace << declarator;
  if (!PT::nth<0>(declarator)) return; // empty

  bool visible = update_position(declarator);
  PT::Encoding name = declarator->encoded_name();
  PT::Encoding type = declarator->encoded_type();
  if (type.is_function())
  {
    trace << "declare function " << name << " (" << type << ')' 
	  << raw_filename_ << ':' << lineno_;

    bpl::list parameter_types;
    bpl::object return_type = lookup_function_types(type, parameter_types);
    bpl::list parameters;
    PT::List *p = declarator->cdr();
    while (p && p->car() && *p->car() != '(') p = p->cdr();

    for (PT::List *node = static_cast<PT::List *>(PT::nth<1>(p));
	 node->cdr();
	 node = PT::tail(node, 2))
    {
      node->car()->accept(this); // PT::ParameterDeclaration
      // FIXME: can parameter_ be 'void' ?
      parameters.append(parameter_);
    }
    size_t length = (name.front() - 0x80);
    bpl::object fname = qname(std::string(name.begin() + 1, name.begin() + 1 + length));
    bpl::object real_name = fname[-1];
    bpl::object pre;
    bpl::object post;
    bpl::object function = asg_module_.attr("Function")(file_, lineno_,
                                                        "function",
                                                        pre,
                                                        return_type,
                                                        post,
                                                        fname,
                                                        real_name);

    function.attr("parameters").attr("extend")(parameters);
    if (declaration_) add_comments(function, declaration_->get_comments());
    add_comments(function, declarator->get_comments());
    if (visible) declare(function);
  }
  else
  {
    bpl::object t = lookup(type);
    size_t length = (name.front() - 0x80);
    bpl::object vname = qname(std::string(name.begin() + 1, name.begin() + 1 + length));

    // FIXME
    std::string vtype;// = builder_->scope()->type();
    if (vtype == "class" || vtype == "struct" || vtype == "union")
      vtype = "data member";
    else
    {
      if (vtype == "function")
	vtype = "local ";
      vtype += "variable";
    }
    bpl::object variable = asg_module_.attr("Variable")(file_, lineno_,
                                                        vtype, vname, t, false);
    if (declaration_) add_comments(variable, declaration_->get_comments());
    add_comments(variable, declarator->get_comments());
    if (visible) declare(variable);
    declare_type(vname, variable, visible);
  }
}

void ASGTranslator::visit(PT::SimpleDeclaration *declaration)
{
  Trace trace("ASGTranslator::visit(SimpleDeclaration)", Trace::TRANSLATION);
  bool visible = update_position(declaration);
  // Check whether this is a typedef:
  PT::DeclSpec *spec = declaration->decl_specifier_seq();
  // the decl-specifier-seq may contain a class-specifier, i.e.
  // which we need to define first.
  defines_class_or_enum_ = false;
  if (spec) spec->accept(this);
  if (spec && spec->is_typedef())
  {
    for (PT::List *d = declaration->declarators(); d; d = PT::tail(d, 2))
    {
      PT::Declarator *declarator = static_cast<PT::Declarator *>(d->car());
      PT::Encoding name = declarator->encoded_name();
      PT::Encoding type = declarator->encoded_type();
      trace << "declare type " << name << " (" << type << ')' 
	    << raw_filename_ << ':' << lineno_;
      assert(name.is_simple_name());
      size_t length = (name.front() - 0x80);
      bpl::object tname = qname(std::string(name.begin() + 1, name.begin() + 1 + length));
      bpl::object alias = lookup(type);
      bpl::object declaration = asg_module_.attr("Typedef")(file_, lineno_,
                                                            "typedef",
                                                            tname,
                                                            alias, defines_class_or_enum_);
      add_comments(declaration, declarator->get_comments());
      if (visible) declare(declaration);
      declare_type(tname, declaration, visible);
    }
  }
  else
  {
    // Cache the declaration while traversing the individual declarators;
    // the comments are passed through.
    declaration_ = declaration;
    visit(static_cast<PT::List *>(declaration));
    declaration_ = 0;
  }
}

void ASGTranslator::visit(PT::ClassSpec *spec)
{
  Trace trace("ASGTranslator::visit(PT::ClassSpec *)", Trace::TRANSLATION);
  
  bool visible = update_position(spec);

  std::string key = PT::string(spec->key());
  bpl::object name;
  if (spec->name()) name = qname(PT::string(static_cast<PT::Atom *>(spec->name())));
  else // anonymous
  {
    PT::Encoding ename = spec->encoded_name();
    size_t length = (ename.front() - 0x80);
    name = qname(std::string(ename.begin() + 1, ename.begin() + 1 + length));
  }
  bpl::object class_ = asg_module_.attr("Class")(file_, lineno_, key, name);
  add_comments(class_, spec->get_comments());
  if (visible) declare(class_);
  declare_type(name, class_, visible);
  defines_class_or_enum_ = false;
  scope_.push(class_);
  spec->body()->accept(this);
  scope_.pop();
  defines_class_or_enum_ = true;
}

void ASGTranslator::visit(PT::EnumSpec *spec)
{
  Trace trace("ASGTranslator::visit(PT::EnumSpec *)", Trace::TRANSLATION);

  bool visible = update_position(spec);
  std::string name;
  
  if (!spec->name()) //anonymous
  {
    PT::Encoding ename = spec->encoded_name();
    size_t length = (ename.front() - 0x80);
    name = std::string(ename.begin() + 1, ename.begin() + 1 + length);
  }
  else name = PT::reify(spec->name());

  bpl::list enumerators;
  PT::List *enode = spec->enumerators();
  bpl::object enumerator;
  while (enode)
  {
    // quite a costly way to update the line number...
    update_position(enode);
    PT::Node *penumor = PT::nth<0>(enode);
    if (penumor->is_atom())
    {
      // identifier
      bpl::object name = qname(PT::string(static_cast<PT::Atom *>(penumor)));
      enumerator = asg_module_.attr("Enumerator")(file_, lineno_, name, "");
      add_comments(enumerator, static_cast<PT::Identifier *>(penumor)->get_comments());
    }
    else
    {
      // identifier = constant-expression
      bpl::object name = qname(PT::string(static_cast<PT::Atom *>(PT::nth<0>(static_cast<PT::List *>(penumor)))));
      // TBD: For now stringify the initializer expression.
      std::string value = PT::reify(PT::nth<2>(static_cast<PT::List *>(penumor)));
      enumerator = asg_module_.attr("Enumerator")(file_, lineno_, name, value);
      add_comments(enumerator, static_cast<PT::Identifier *>(penumor)->get_comments());
    }
    enumerators.append(enumerator);
    enode = PT::tail(enode, 2);
    // Skip comma
    if (enode && enode->car() && *enode->car() == ',') enode = PT::tail(enode, 0);
  }
  // Create ASG.Enum object
  bpl::object enum_ = asg_module_.attr("Enum")(file_, lineno_, qname(name), enumerators);
  add_comments(enum_, spec);

  if (visible) declare(enum_);
  declare_type(qname(name), enum_, visible);
  defines_class_or_enum_ = true;
}

void ASGTranslator::visit(PT::ParameterDeclaration *param)
{
  Trace trace("ASGTranslator::visit(ParameterDeclaration)", Trace::TRANSLATION);

  PT::DeclSpec *spec = param->decl_specifier_seq();
  // FIXME: extract modifiers and type from decl-specifier-seq
  bpl::object pre;
  bpl::object type = lookup(spec->type());
  bpl::object post;
  std::string name = param->declarator()->encoded_name().unmangled();
  std::string value;
  PT::Node *initializer = param->initializer();
  if (initializer) value = PT::reify(initializer);
  parameter_ = asg_module_.attr("Parameter")(pre, type, post, name, value);
}

void ASGTranslator::add_comments(bpl::object declarator, PT::Node *c)
{
  Trace trace("ASGTranslator::add_comments", Trace::TRANSLATION);
  if (!declarator || !c) return;
}

bool ASGTranslator::update_position(PT::Node *node)
{
  Trace trace("ASGTranslator::update_position", Trace::TRANSLATION);

  std::string filename;
  unsigned long column;
  buffer_->origin(node->begin(), filename, lineno_, column);

  if (filename != raw_filename_)
  {
    if (primary_file_only_)
      // raw_filename_ remains the primary file's name
      // and all declarations from elsewhere are ignored
      return false;
    raw_filename_ = filename;

    // determine canonical filenames
    std::string long_filename = make_full_path(filename);
    std::string short_filename = make_short_path(filename, base_path_);

    bpl::object file = files_.get(short_filename);
    if (file)
      file_ = file;
    else
    {
      file_ = sf_module_.attr("SourceFile")(short_filename, long_filename, "C");
      files_[short_filename] = file_;
    }
  }
  return true;
}

bpl::object ASGTranslator::lookup(PT::Encoding const &name)
{
  Trace trace("ASGTranslator::lookup", Trace::TRANSLATION);
  trace << name;
  name_ = name;
  bpl::object type;
  decode_type(name.begin(), type);
  return type;
}

bpl::object ASGTranslator::lookup_function_types(PT::Encoding const &name,
                                                 bpl::list parameters)
{
  Trace trace("ASGTranslator::lookup_function_types", Trace::TRANSLATION);
  trace << name;
  name_ = name;

  PT::Encoding::iterator i = name.begin();
  assert(*i == 'F');
  ++i;
  while (true)
  {
    bpl::object parameter;
    i = decode_type(i, parameter);
    if (parameter)
      parameters.append(parameter);
    else
      break;
  }
  ++i; // skip over '_'
  bpl::object return_type;
  i = decode_type(i, return_type);
  return return_type;
}

bpl::object ASGTranslator::declare_type(bpl::object name,
                                        bpl::object declaration,
                                        bool declared)
{
  Trace trace("ASGTranslator::declare_type", Trace::TRANSLATION);
  trace << bpl::extract<char const *>(bpl::str(name));
  bpl::object type;
  if (declared) type = asg_module_.attr("DeclaredTypeId")("C", name, declaration);
  else type = asg_module_.attr("UnknownTypeId")("C", name);
  types_[name] = type;
  return type;
}

// This is almost a verbatim copy of the Decoder::decode
// methods from Synopsis/Parsers/Cxx/syn/decoder.cc
// with some minor modifications to disable the C++ specific things.
// FIXME: this ought to be part of SymbolLookup::Type.
PT::Encoding::iterator ASGTranslator::decode_name(PT::Encoding::iterator i,
                                                  std::string &name)
{
  Trace trace("ASGTranslator::decode_name", Trace::TRANSLATION);
  size_t length = *i++ - 0x80;
  name = std::string(length, '\0');
  std::copy(i, i + length, name.begin());
  i += length;
  return i;
}

void ASGTranslator::declare(bpl::object declaration)
{
  if (scope_.size())
    bpl::extract<bpl::list>(scope_.top().attr("declarations"))().append(declaration);
  else
    declarations_.append(declaration);    
}

PT::Encoding::iterator ASGTranslator::decode_type(PT::Encoding::iterator i,
                                                  bpl::object &type)
{
  Trace trace("TypeTranslator::decode_type", Trace::TRANSLATION);
  bpl::list premod, postmod;
  std::string name;
  bpl::object base;

  // Loop forever until broken
  while (i != name_.end() && !name.length() && !base)
  {
    int c = *i++;
    switch (c)
    {
      case 'P':
	postmod.insert(0, "*");
	break;
//       case 'R':
// 	postmod.insert(0, "&");
// 	break;
      case 'S':
	premod.append("signed");
	break;
      case 'U':
	premod.append("unsigned");
	break;
      case 'C':
	premod.append("const");
	break;
      case 'V':
	premod.append("volatile");
	break;
      case 'A':
      {
	std::string array("[");
	while (*i != '_') array.push_back(*i++);
	array.push_back(']');
	++i;
	postmod.append(array);
	  break;
      }
//       case '*':
//       {
// 	ScopedName n;
// 	n.push_back("*");
// 	base = new Types::Dependent(n);
// 	break;
//       }
      case 'i':
	name = "int";
	break;
      case 'v':
	name = "void";
	break;
//       case 'b':
// 	name = "bool";
// 	break;
      case 's':
	name = "short";
	break;
      case 'c':
	name = "char";
	break;
//       case 'w':
// 	name = "wchar_t";
// 	break;
      case 'l':
	name = "long";
	break;
      case 'j':
	name = "long long";
	break;
      case 'f':
	name = "float";
	break;
      case 'd':
	name = "double";
	break;
      case 'r':
	name = "long double";
	break;
      case 'e':
	name = "...";
	break;
      case '?':
	name = "int"; // in C, no return type spec defaults to int
	break;
//       case 'Q':
// 	base = decodeQualType();
// 	break;
      case '_':
	--i;
	type = bpl::object();
	return i; // end of func params
      case 'F':
	i = decode_func_ptr(i, base, postmod);
	break;
//       case 'T':
// 	base = decodeTemplate();
// 	break;
//       case 'M':
// 	// Pointer to member. Format is same as for named types
// 	name = decodeName() + "::*";
// 	break;
      default:
	if (c > 0x80)
	{
	  --i;
	  i = decode_name(i, name);
	  break;
	}
	// FIXME
	// 	    std::cerr << "\nUnknown char decoding '"<<m_string<<"': "
	// 		 << char(c) << " " << c << " at "
	// 		 << (m_iter - m_string.begin()) << std::endl;
    } // switch
  } // while
  if (!base && !name.length())
  {
    // FIXME
    // 	std::cerr << "no type or name found decoding " << m_string << std::endl;
    type = bpl::object();
    return i;
  }
  if (!base)
    base = types_[qname(name)];
  if (!bpl::len(premod) && !bpl::len(postmod))
    type = base;
  else
    type = asg_module_.attr("ModifierTypeId")("C", base, premod, postmod);

  return i;
}

PT::Encoding::iterator ASGTranslator::decode_func_ptr(PT::Encoding::iterator i,
                                                      bpl::object &type,
                                                      bpl::list postmod)
{
  Trace trace("TypeTranslator::decode_func_ptr", Trace::TRANSLATION);
  // Function ptr. Encoded same as function
  bpl::list premod;
  // Move * from postmod to funcptr's premod. This makes the output be
  // "void (*convert)()" instead of "void (convert)()*"
  if (bpl::len(postmod) > 0 && postmod[0] == bpl::object("*"))
  {
    premod.append(postmod.pop(0));
  }
  bpl::list parameters;
  while (true)
  {
    bpl::object parameter;
    i = decode_type(i, parameter);
    if (parameter)
      parameters.append(parameter);
    else
      break;
  }
  ++i; // skip over '_'
  i = decode_type(i, type);
  type = asg_module_.attr("FunctionTypeId")(type, premod, parameters);
  return i;
}
