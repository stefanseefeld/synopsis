//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#include <boost/python.hpp>
#include <Synopsis/PTree.hh>

namespace bpl = boost::python;
using namespace Synopsis;
namespace PT = Synopsis::PTree;

namespace
{

class VisitorWrapper : public PT::Visitor
{
public:
  VisitorWrapper(PyObject *s) : self(s) {}

  virtual void visit_node(PT::Node *) {}
  virtual void visit_atom(PT::Atom *a) { apply("visit_node", a);}
  virtual void visit_list(PT::List *l) { apply("visit_node", l);}

  virtual void visit_literal(PT::Literal *a) { apply("visit_atom", a);}
  virtual void visit_commented_atom(PT::CommentedAtom *a) { apply("visit_atom", a);}
  virtual void visit_dup_atom(PT::DupAtom *a) { apply("visit_atom", a);}
  virtual void visit_identifier(PT::Identifier *a) { apply("visit_atom", a);}
  virtual void visit_keyword(PT::Keyword *a) { apply("visit_atom", a);}

  virtual void visit_brace(PT::Brace *l) { apply("visit_list", l);}
  virtual void visit_block(PT::Block *l) { apply("visit_brace", l);}
  virtual void visit_class_body(PT::ClassBody *l) { apply("visit_brace", l);}
  virtual void visit_template_decl(PT::TemplateDeclaration *l) { apply("visit_list", l);}
  virtual void visit_template_instantiation(PT::TemplateInstantiation *l) { apply("visit_list", l);}
  virtual void visit_extern_template(PT::ExternTemplate *l) { apply("visit_list", l);}
  virtual void visit_linkage_spec(PT::LinkageSpec *l) { apply("visit_list", l);}
  virtual void visit_namespace_spec(PT::NamespaceSpec *l) { apply("visit_list", l);}
  virtual void visit_declaration(PT::SimpleDeclaration *l) { apply("visit_list", l);}
  virtual void visit_namespace_alias(PT::NamespaceAlias *l) { apply("visit_declaration", l);}
  virtual void visit_using_directive(PT::UsingDirective *l) { apply("visit_declaration", l);}
  virtual void visit_parameter_declaration(PT::ParameterDeclaration *l) { apply("visit_declaration", l);}
  virtual void visit_function_definition(PT::FunctionDefinition *l) { apply("visit_declaration", l);}
  virtual void visit_using_declaration(PT::UsingDeclaration *l) { apply("visit_list", l);}
  virtual void visit_decl_spec(PT::DeclSpec *l) { apply("visit_decl_spec", l);}
  virtual void visit_declarator(PT::Declarator *l) { apply("visit_list", l);}
  virtual void visit_name(PT::Name *l) { apply("visit_list", l);}
  virtual void visit_fstyle_cast_expr(PT::FstyleCastExpr *l) { apply("visit_list", l);}
  virtual void visit_class_spec(PT::ClassSpec *l) { apply("visit_list", l);}
  virtual void visit_enum_spec(PT::EnumSpec *l) { apply("visit_list", l);}
  virtual void visit_elaborated_type_spec(PT::ElaboratedTypeSpec *l) { apply("visit_list", l);}
  virtual void visit_access_spec(PT::AccessSpec *l) { apply("visit_list", l);}
  virtual void visit_access_decl(PT::AccessDecl *l) { apply("visit_list", l);}
  virtual void visit_if_statement(PT::IfStatement *l) { apply("visit_list", l);}
  virtual void visit_switch_statement(PT::SwitchStatement *l) { apply("visit_list", l);}
  virtual void visit_while_statement(PT::WhileStatement *l) { apply("visit_list", l);}
  virtual void visit_do_statement(PT::DoStatement *l) { apply("visit_list", l);}
  virtual void visit_for_statement(PT::ForStatement *l) { apply("visit_list", l);}
  virtual void visit_try_statement(PT::TryStatement *l) { apply("visit_list", l);}
  virtual void visit_break_statement(PT::BreakStatement *l) { apply("visit_list", l);}
  virtual void visit_continue_statement(PT::ContinueStatement *l) { apply("visit_list", l);}
  virtual void visit_return_statement(PT::ReturnStatement *l) { apply("visit_list", l);}
  virtual void visit_goto_statement(PT::GotoStatement *l) { apply("visit_list", l);}
  virtual void visit_case_statement(PT::CaseStatement *l) { apply("visit_list", l);}
  virtual void visit_default_statement(PT::DefaultStatement *l) { apply("visit_list", l);}
  virtual void visit_label_statement(PT::LabelStatement *l) { apply("visit_list", l);}
  virtual void visit_expr_statement(PT::ExprStatement *l) { apply("visit_list", l);}
  virtual void visit_expression(PT::Expression *l) { apply("visit_list", l);}
  virtual void visit_assign_expr(PT::AssignExpr *l) { apply("visit_list", l);}
  virtual void visit_cond_expr(PT::CondExpr *l) { apply("visit_list", l);}
  virtual void visit_infix_expr(PT::InfixExpr *l) { apply("visit_list", l);}
  virtual void visit_pm_expr(PT::PmExpr *l) { apply("visit_list", l);}
  virtual void visit_cast_expr(PT::CastExpr *l) { apply("visit_list", l);}
  virtual void visit_unary_expr(PT::UnaryExpr *l) { apply("visit_list", l);}
  virtual void visit_throw_expr(PT::ThrowExpr *l) { apply("visit_list", l);}
  virtual void visit_sizeof_expr(PT::SizeofExpr *l) { apply("visit_list", l);}
  virtual void visit_typeid_expr(PT::TypeidExpr *l) { apply("visit_list", l);}
  virtual void visit_typeof_expr(PT::TypeofExpr *l) { apply("visit_list", l);}
  virtual void visit_new_expr(PT::NewExpr *l) { apply("visit_list", l);}
  virtual void visit_delete_expr(PT::DeleteExpr *l) { apply("visit_list", l);}
  virtual void visit_array_expr(PT::ArrayExpr *l) { apply("visit_list", l);}
  virtual void visit_funcall_expr(PT::FuncallExpr *l) { apply("visit_list", l);}
  virtual void visit_postfix_expr(PT::PostfixExpr *l) { apply("visit_list", l);}
  virtual void visit_dot_member_expr(PT::DotMemberExpr *l) { apply("visit_list", l);}
  virtual void visit_arrow_member_expr(PT::ArrowMemberExpr *l) { apply("visit_list", l);}
  virtual void visit_paren_expr(PT::ParenExpr *l) { apply("visit_list", l);}

  virtual void visit(PT::Node *n) { apply("visit_node", n);}
  virtual void visit(PT::Atom *n) { apply("visit_atom", n);}
  virtual void visit(PT::List* n) { apply("visit_list", n);}
  virtual void visit(PT::Literal *n) { apply("visit_literal", n);}
  virtual void visit(PT::CommentedAtom *n) { apply("visit_commented_atom", n);}
  virtual void visit(PT::DupAtom *n) { apply("visit_dup_atom", n);}
  virtual void visit(PT::Identifier *n) { apply("visit_identifier", n);}
  virtual void visit(PT::Keyword *n) { apply("visit_keyword", n);}

  virtual void visit(PT::Brace *n) { apply("visit_brace", n);}
  virtual void visit(PT::Block *n) { apply("visit_block", n);}
  virtual void visit(PT::ClassBody *n) { apply("visit_class_body", n);}
  virtual void visit(PT::TemplateDeclaration *n) { apply("visit_template_decl", n);}
  virtual void visit(PT::TemplateInstantiation *n) { apply("visit_template_instantiation", n);}
  virtual void visit(PT::ExternTemplate *n) { apply("visit_extern_template", n);}
  virtual void visit(PT::LinkageSpec *n) { apply("visit_linkage_spec", n);}
  virtual void visit(PT::NamespaceSpec *n) { apply("visit_namespace_spec", n);}
  virtual void visit(PT::NamespaceAlias *n) { apply("visit_namespace_alias", n);}
  virtual void visit(PT::UsingDirective *n) { apply("visit_using_directive", n);}
  virtual void visit(PT::SimpleDeclaration *n) { apply("visit_simple_declaration", n);}
  virtual void visit(PT::FunctionDefinition *n) { apply("visit_function_definition", n);}
  virtual void visit(PT::ParameterDeclaration *n) { apply("visit_parameter_declaration", n);}
  virtual void visit(PT::UsingDeclaration *n) { apply("visit_using_declaration", n);}
  virtual void visit(PT::DeclSpec *n) { apply("visit_decl_spec", n);}
  virtual void visit(PT::Declarator *n) { apply("visit_declarator", n);}
  virtual void visit(PT::Name *n) { apply("visit_name", n);}
  virtual void visit(PT::FstyleCastExpr *n) { apply("visit_fstyle_cast_expr", n);}
  virtual void visit(PT::ClassSpec *n) { apply("visit_class_spec", n);}
  virtual void visit(PT::EnumSpec *n) { apply("visit_enum_spec", n);}
  virtual void visit(PT::ElaboratedTypeSpec *n) { apply("visit_elaborated_type_spec", n);}
  virtual void visit(PT::AccessSpec *n) { apply("visit_access_spec", n);}
  virtual void visit(PT::AccessDecl *n) { apply("visit_access_decl", n);}
  virtual void visit(PT::IfStatement *n) { apply("visit_if_statement", n);}
  virtual void visit(PT::SwitchStatement *n) { apply("visit_switch_statement", n);}
  virtual void visit(PT::WhileStatement *n) { apply("visit_while_statement", n);}
  virtual void visit(PT::DoStatement *n) { apply("visit_do_statement", n);}
  virtual void visit(PT::ForStatement *n) { apply("visit_for_statement", n);}
  virtual void visit(PT::TryStatement *n) { apply("visit_try_statement", n);}
  virtual void visit(PT::BreakStatement *n) { apply("visit_break_statement", n);}
  virtual void visit(PT::ContinueStatement *n) { apply("visit_continue_statement", n);}
  virtual void visit(PT::ReturnStatement *n) { apply("visit_return_statement", n);}
  virtual void visit(PT::GotoStatement *n) { apply("visit_goto_statement", n);}
  virtual void visit(PT::CaseStatement *n) { apply("visit_case_statement", n);}
  virtual void visit(PT::DefaultStatement *n) { apply("visit_default_statement", n);}
  virtual void visit(PT::LabelStatement *n) { apply("visit_label_statement", n);}
  virtual void visit(PT::ExprStatement *n) { apply("visit_expr_statement", n);}
  virtual void visit(PT::Expression *n) { apply("visit_expression", n);}
  virtual void visit(PT::AssignExpr *n) { apply("visit_assign_expr", n);}
  virtual void visit(PT::CondExpr *n) { apply("visit_cond_expr", n);}
  virtual void visit(PT::InfixExpr *n) { apply("visit_infix_expr", n);}
  virtual void visit(PT::PmExpr *n) { apply("visit_pm_expr", n);}
  virtual void visit(PT::CastExpr *n) { apply("visit_cast_expr", n);}
  virtual void visit(PT::UnaryExpr *n) { apply("visit_unary_expr", n);}
  virtual void visit(PT::ThrowExpr *n) { apply("visit_throw_expr", n);}
  virtual void visit(PT::SizeofExpr *n) { apply("visit_sizeof_expr", n);}
  virtual void visit(PT::TypeidExpr *n) { apply("visit_typeid_expr", n);}
  virtual void visit(PT::TypeofExpr *n) { apply("visit_typeof_expr", n);}
  virtual void visit(PT::NewExpr *n) { apply("visit_new_expr", n);}
  virtual void visit(PT::DeleteExpr *n) { apply("visit_delete_expr", n);}
  virtual void visit(PT::ArrayExpr *n) { apply("visit_array_expr", n);}
  virtual void visit(PT::FuncallExpr *n) { apply("visit_funcall_expr", n);}
  virtual void visit(PT::PostfixExpr *n) { apply("visit_postfix_expr", n);}
  virtual void visit(PT::DotMemberExpr *n) { apply("visit_dot_member_expr", n);}
  virtual void visit(PT::ArrowMemberExpr *n) { apply("visit_arrow_member_expr", n);}
  virtual void visit(PT::ParenExpr *n) { apply("visit_paren_expr", n);}

private:
  void apply(char const *f, PT::Node* n) { bpl::call_method<void>(self, f, bpl::ptr(n));}

  PyObject *self;
};

std::string as_string(PT::Encoding const &e) 
{
  std::ostringstream oss;
  oss << e;
  return oss.str();
}

PT::Node *nth_(PT::List *l, size_t k) { return PT::nth(l, k);}
PT::Node *tail_(PT::List *l, size_t k) { return PT::tail(l, k);}

}

BOOST_PYTHON_MODULE(PTree)
{
  bpl::class_<PT::Encoding> encoding("Encoding");
  encoding.def("__str__", PT::string);
  encoding.def("unmangled", &PT::Encoding::unmangled);
  encoding.add_property("is_simple_name", &PT::Encoding::is_simple_name);
  encoding.add_property("is_global_scope", &PT::Encoding::is_global_scope);
  encoding.add_property("is_qualified", &PT::Encoding::is_qualified);
  encoding.add_property("is_function", &PT::Encoding::is_function);
  encoding.add_property("is_template_id", &PT::Encoding::is_template_id);

  bpl::class_<PT::Visitor, boost::noncopyable, VisitorWrapper> visitor("Visitor");
  visitor.def("visit_node", &VisitorWrapper::visit_node);
  visitor.def("visit_atom", &VisitorWrapper::visit_atom);
  visitor.def("visit_list", &VisitorWrapper::visit_list);
  visitor.def("visit_literal", &VisitorWrapper::visit_literal);
  visitor.def("visit_commented_atom", &VisitorWrapper::visit_commented_atom);
  visitor.def("visit_dup_atom", &VisitorWrapper::visit_dup_atom);
  visitor.def("visit_identifier", &VisitorWrapper::visit_identifier);
  visitor.def("visit_keyword", &VisitorWrapper::visit_keyword);

  visitor.def("visit_brace", &VisitorWrapper::visit_brace);
  visitor.def("visit_block", &VisitorWrapper::visit_block);
  visitor.def("visit_class_body", &VisitorWrapper::visit_class_body);
  visitor.def("visit_template_decl", &VisitorWrapper::visit_template_decl);
  visitor.def("visit_template_instantiation", &VisitorWrapper::visit_template_instantiation);
  visitor.def("visit_extern_template", &VisitorWrapper::visit_extern_template);
  visitor.def("visit_linkage_spec", &VisitorWrapper::visit_linkage_spec);
  visitor.def("visit_namespace_spec", &VisitorWrapper::visit_namespace_spec);
  visitor.def("visit_namespace_alias", &VisitorWrapper::visit_namespace_alias);
  visitor.def("visit_using_directive", &VisitorWrapper::visit_using_directive);
  visitor.def("visit_declaration", &VisitorWrapper::visit_declaration);
  visitor.def("visit_parameter_declaration", &VisitorWrapper::visit_parameter_declaration);
  visitor.def("visit_using_declaration", &VisitorWrapper::visit_using_declaration);
  visitor.def("visit_declarator", &VisitorWrapper::visit_declarator);
  visitor.def("visit_name", &VisitorWrapper::visit_name);
  visitor.def("visit_fstyle_cast_expr", &VisitorWrapper::visit_fstyle_cast_expr);
  visitor.def("visit_class_spec", &VisitorWrapper::visit_class_spec);
  visitor.def("visit_enum_spec", &VisitorWrapper::visit_enum_spec);
  visitor.def("visit_access_spec", &VisitorWrapper::visit_access_spec);
  visitor.def("visit_access_decl", &VisitorWrapper::visit_access_decl);
  visitor.def("visit_if_statement", &VisitorWrapper::visit_if_statement);
  visitor.def("visit_switch_statement", &VisitorWrapper::visit_switch_statement);
  visitor.def("visit_while_statement", &VisitorWrapper::visit_while_statement);
  visitor.def("visit_do_statement", &VisitorWrapper::visit_do_statement);
  visitor.def("visit_for_statement", &VisitorWrapper::visit_for_statement);
  visitor.def("visit_try_statement", &VisitorWrapper::visit_try_statement);
  visitor.def("visit_break_statement", &VisitorWrapper::visit_break_statement);
  visitor.def("visit_continue_statement", &VisitorWrapper::visit_continue_statement);
  visitor.def("visit_return_statement", &VisitorWrapper::visit_return_statement);
  visitor.def("visit_goto_statement", &VisitorWrapper::visit_goto_statement);
  visitor.def("visit_case_statement", &VisitorWrapper::visit_case_statement);
  visitor.def("visit_default_statement", &VisitorWrapper::visit_default_statement);
  visitor.def("visit_label_statement", &VisitorWrapper::visit_label_statement);
  visitor.def("visit_expr_statement", &VisitorWrapper::visit_expr_statement);
  visitor.def("visit_expression", &VisitorWrapper::visit_expression);
  visitor.def("visit_assign_expr", &VisitorWrapper::visit_assign_expr);
  visitor.def("visit_cond_expr", &VisitorWrapper::visit_cond_expr);
  visitor.def("visit_infix_expr", &VisitorWrapper::visit_infix_expr);
  visitor.def("visit_pm_expr", &VisitorWrapper::visit_pm_expr);
  visitor.def("visit_cast_expr", &VisitorWrapper::visit_cast_expr);
  visitor.def("visit_unary_expr", &VisitorWrapper::visit_unary_expr);
  visitor.def("visit_throw_expr", &VisitorWrapper::visit_throw_expr);
  visitor.def("visit_sizeof_expr", &VisitorWrapper::visit_sizeof_expr);
  visitor.def("visit_typeid_expr", &VisitorWrapper::visit_typeid_expr);
  visitor.def("visit_typeof_expr", &VisitorWrapper::visit_typeof_expr);
  visitor.def("visit_new_expr", &VisitorWrapper::visit_new_expr);
  visitor.def("visit_delete_expr", &VisitorWrapper::visit_delete_expr);
  visitor.def("visit_array_expr", &VisitorWrapper::visit_array_expr);
  visitor.def("visit_funcall_expr", &VisitorWrapper::visit_funcall_expr);
  visitor.def("visit_postfix_expr", &VisitorWrapper::visit_postfix_expr);
  visitor.def("visit_dot_member_expr", &VisitorWrapper::visit_dot_member_expr);
  visitor.def("visit_arrow_member_expr", &VisitorWrapper::visit_arrow_member_expr);
  visitor.def("visit_paren_expr", &VisitorWrapper::visit_paren_expr);

  bpl::class_<PT::Node, PT::Node *, boost::noncopyable> node("Node", bpl::no_init);
  // no idea why we can't pass &Node::accept here, but if we do,
  // the registry will complain that there is no lvalue conversion...
  node.def("accept", &PT::Node::accept);
  bpl::class_<PT::Atom, bpl::bases<PT::Node>, PT::Atom *, boost::noncopyable> atom("Atom", bpl::no_init);
  atom.def("__str__", PT::string);
  atom.add_property("position", &PT::Atom::position);
  atom.add_property("length", &PT::Atom::length);
  bpl::class_<PT::List, bpl::bases<PT::Node>, PT::List *, boost::noncopyable> list("List", bpl::no_init);
  // The PTree module uses garbage collection so just ignore memory management,
  // at least for now.
  list.def("car", &PT::List::car, bpl::return_value_policy<bpl::reference_existing_object>());
  list.def("cdr", &PT::List::cdr, bpl::return_value_policy<bpl::reference_existing_object>());
  list.def("first", &PT::nth<0>, bpl::return_value_policy<bpl::reference_existing_object>());
  list.def("second", &PT::nth<1>, bpl::return_value_policy<bpl::reference_existing_object>());
  list.def("third", &PT::nth<2>, bpl::return_value_policy<bpl::reference_existing_object>());
  list.def("nth", nth_, bpl::return_value_policy<bpl::reference_existing_object>());
  list.def("tail", tail_, bpl::return_value_policy<bpl::reference_existing_object>());
  list.add_property("name", &PT::List::encoded_name);
  list.add_property("type", &PT::List::encoded_type);

  // various atoms

  bpl::class_<PT::Literal, bpl::bases<PT::Atom>, PT::Literal *, boost::noncopyable> literal("Literal", bpl::no_init);
  bpl::class_<PT::CommentedAtom, bpl::bases<PT::Atom>, PT::CommentedAtom *, boost::noncopyable> comment_atom("CommentedAtom", bpl::no_init);
  bpl::class_<PT::DupAtom, bpl::bases<PT::CommentedAtom>, PT::DupAtom *, boost::noncopyable> dup_atom("DupAtom", bpl::no_init);
  bpl::class_<PT::Identifier, bpl::bases<PT::CommentedAtom>, PT::Identifier *, boost::noncopyable> identifier("Identifier", bpl::no_init);
  bpl::class_<PT::Keyword, bpl::bases<PT::CommentedAtom>, PT::Keyword *, boost::noncopyable> reserved("Keyword", bpl::no_init);

  // various lists

  bpl::class_<PT::Brace, bpl::bases<PT::List>, PT::Brace *, boost::noncopyable> brace("Brace", bpl::no_init);
  bpl::class_<PT::Block, bpl::bases<PT::Brace>, PT::Block *, boost::noncopyable> block("Block", bpl::no_init);
  bpl::class_<PT::ClassBody, bpl::bases<PT::Brace>, PT::ClassBody *, boost::noncopyable> class_body("ClassBody", bpl::no_init);
  bpl::class_<PT::TemplateDeclaration, bpl::bases<PT::List>, PT::TemplateDeclaration *, boost::noncopyable> template_decl("TemplateDeclaration", bpl::no_init);
  bpl::class_<PT::TemplateInstantiation, bpl::bases<PT::List>, PT::TemplateInstantiation *, boost::noncopyable> template_instantiation("TemplateInstantiation", bpl::no_init);
  bpl::class_<PT::ExternTemplate, bpl::bases<PT::List>, PT::ExternTemplate *, boost::noncopyable> extern_template("ExternTemplate", bpl::no_init);
  bpl::class_<PT::LinkageSpec, bpl::bases<PT::List>, PT::LinkageSpec *, boost::noncopyable> linkage_spec("LinkageSpec", bpl::no_init);
  bpl::class_<PT::NamespaceSpec, bpl::bases<PT::List>, PT::NamespaceSpec *, boost::noncopyable> namespace_spec("NamespaceSpec", bpl::no_init);
  bpl::class_<PT::SimpleDeclaration, bpl::bases<PT::List>, PT::SimpleDeclaration *, boost::noncopyable> declaration("SimpleDeclaration", bpl::no_init);
  declaration.add_property("decl_specifier_seq",
			   bpl::make_function(&PT::SimpleDeclaration::decl_specifier_seq, 
					      bpl::return_internal_reference<>()));
  declaration.add_property("declarators",
			   bpl::make_function(&PT::SimpleDeclaration::declarators, 
					      bpl::return_internal_reference<>()));
  bpl::class_<PT::NamespaceAlias, bpl::bases<PT::List>, PT::NamespaceAlias *, boost::noncopyable> namespace_alias("NamespaceAlias", bpl::no_init);
  bpl::class_<PT::UsingDirective, bpl::bases<PT::List>, PT::UsingDirective *, boost::noncopyable> using_directive("UsingDirective", bpl::no_init);
  bpl::class_<PT::UsingDeclaration, bpl::bases<PT::List>, PT::UsingDeclaration *, boost::noncopyable> using_declaration("UsingDeclaration", bpl::no_init);
  bpl::class_<PT::FunctionDefinition, bpl::bases<PT::List>, PT::FunctionDefinition *, boost::noncopyable> function_definition("FunctionDefinition", bpl::no_init);
  bpl::class_<PT::ParameterDeclaration, bpl::bases<PT::List>, PT::ParameterDeclaration *, boost::noncopyable> parameter_declaration("ParameterDeclaration", bpl::no_init);
  bpl::class_<PT::DeclSpec, bpl::bases<PT::List>, PT::DeclSpec *, boost::noncopyable> decl_spec("DeclSpec", bpl::no_init);
  decl_spec.add_property("friend", &PT::DeclSpec::is_friend);
  decl_spec.add_property("typedef", &PT::DeclSpec::is_typedef);
  bpl::class_<PT::Declarator, bpl::bases<PT::List>, PT::Declarator *, boost::noncopyable> declarator("Declarator", bpl::no_init);
  declarator.add_property("initializer", 
			  bpl::make_function(&PT::Declarator::initializer, 
					     bpl::return_internal_reference<>()));
  bpl::class_<PT::Name, bpl::bases<PT::List>, PT::Name *, boost::noncopyable> name("Name", bpl::no_init);
  bpl::class_<PT::FstyleCastExpr, bpl::bases<PT::List>, PT::FstyleCastExpr *, boost::noncopyable> fstyle_cast_expr("FstyleCastExpr", bpl::no_init);
  bpl::class_<PT::ClassSpec, bpl::bases<PT::List>, PT::ClassSpec *, boost::noncopyable> class_spec("ClassSpec", bpl::no_init);
  bpl::class_<PT::EnumSpec, bpl::bases<PT::List>, PT::EnumSpec *, boost::noncopyable> enum_spec("EnumSpec", bpl::no_init);
  bpl::class_<PT::ElaboratedTypeSpec, bpl::bases<PT::List>, PT::ElaboratedTypeSpec *, boost::noncopyable> elaborated_type_spec("ElaboratedTypeSpec", bpl::no_init);
  bpl::class_<PT::AccessSpec, bpl::bases<PT::List>, PT::AccessSpec *, boost::noncopyable> access_spec("AccessSpec", bpl::no_init);
  bpl::class_<PT::AccessDecl, bpl::bases<PT::List>, PT::AccessDecl *, boost::noncopyable> access_decl("AccessDecl", bpl::no_init);

  // statements

  bpl::class_<PT::IfStatement, bpl::bases<PT::List>, PT::IfStatement *, boost::noncopyable> if_statement("IfStatement", bpl::no_init);
  bpl::class_<PT::SwitchStatement, bpl::bases<PT::List>, PT::SwitchStatement *, boost::noncopyable> switch_statement("SwitchStatement", bpl::no_init);
  bpl::class_<PT::WhileStatement, bpl::bases<PT::List>, PT::WhileStatement *, boost::noncopyable> while_statement("WhileStatement", bpl::no_init);
  bpl::class_<PT::DoStatement, bpl::bases<PT::List>, PT::DoStatement *, boost::noncopyable> do_statement("DoStatement", bpl::no_init);
  bpl::class_<PT::ForStatement, bpl::bases<PT::List>, PT::ForStatement *, boost::noncopyable> for_statement("ForStatement", bpl::no_init);
  bpl::class_<PT::TryStatement, bpl::bases<PT::List>, PT::TryStatement *, boost::noncopyable> try_statement("TryStatement", bpl::no_init);
  bpl::class_<PT::BreakStatement, bpl::bases<PT::List>, PT::BreakStatement *, boost::noncopyable> break_statement("BreakStatement", bpl::no_init);
  bpl::class_<PT::ContinueStatement, bpl::bases<PT::List>, PT::ContinueStatement *, boost::noncopyable> continue_statement("ContinueStatement", bpl::no_init);
  bpl::class_<PT::ReturnStatement, bpl::bases<PT::List>, PT::ReturnStatement *, boost::noncopyable> return_statement("ReturnStatement", bpl::no_init);
  bpl::class_<PT::GotoStatement, bpl::bases<PT::List>, PT::GotoStatement *, boost::noncopyable> goto_statement("GotoStatement", bpl::no_init);
  bpl::class_<PT::CaseStatement, bpl::bases<PT::List>, PT::CaseStatement *, boost::noncopyable> case_statement("CaseStatement", bpl::no_init);
  bpl::class_<PT::DefaultStatement, bpl::bases<PT::List>, PT::DefaultStatement *, boost::noncopyable> default_statement("DefaultStatement", bpl::no_init);
  bpl::class_<PT::LabelStatement, bpl::bases<PT::List>, PT::LabelStatement *, boost::noncopyable> label_statement("LabelStatement", bpl::no_init);
  bpl::class_<PT::ExprStatement, bpl::bases<PT::List>, PT::ExprStatement *, boost::noncopyable> expr_statement("ExprStatement", bpl::no_init);

  // expressions

  bpl::class_<PT::Expression, bpl::bases<PT::List>, PT::Expression *, boost::noncopyable> expression("Expression", bpl::no_init);
  bpl::class_<PT::AssignExpr, bpl::bases<PT::List>, PT::AssignExpr *, boost::noncopyable> assign_expr("AssignExpr", bpl::no_init);
  bpl::class_<PT::CondExpr, bpl::bases<PT::List>, PT::CondExpr *, boost::noncopyable> cond_expr("CondExpr", bpl::no_init);
  bpl::class_<PT::InfixExpr, bpl::bases<PT::List>, PT::InfixExpr *, boost::noncopyable> infix_expr("InfixExpr", bpl::no_init);
  bpl::class_<PT::PmExpr, bpl::bases<PT::List>, PT::PmExpr *, boost::noncopyable> pm_expr("PmExpr", bpl::no_init);
  bpl::class_<PT::CastExpr, bpl::bases<PT::List>, PT::CastExpr *, boost::noncopyable> cast_expr("CastExpr", bpl::no_init);
  bpl::class_<PT::UnaryExpr, bpl::bases<PT::List>, PT::UnaryExpr *, boost::noncopyable> unary_expr("UnaryExpr", bpl::no_init);
  bpl::class_<PT::ThrowExpr, bpl::bases<PT::List>, PT::ThrowExpr *, boost::noncopyable> throw_expr("ThrowExpr", bpl::no_init);
  bpl::class_<PT::SizeofExpr, bpl::bases<PT::List>, PT::SizeofExpr *, boost::noncopyable> sizeof_expr("SizeofExpr", bpl::no_init);
  bpl::class_<PT::TypeidExpr, bpl::bases<PT::List>, PT::TypeidExpr *, boost::noncopyable> typeid_expr("TypeidExpr", bpl::no_init);
  bpl::class_<PT::TypeofExpr, bpl::bases<PT::List>, PT::TypeofExpr *, boost::noncopyable> typeof_expr("TypeofExpr", bpl::no_init);
  bpl::class_<PT::NewExpr, bpl::bases<PT::List>, PT::NewExpr *, boost::noncopyable> new_expr("NewExpr", bpl::no_init);
  bpl::class_<PT::DeleteExpr, bpl::bases<PT::List>, PT::DeleteExpr *, boost::noncopyable> delete_expr("DeleteExpr", bpl::no_init);
  bpl::class_<PT::ArrayExpr, bpl::bases<PT::List>, PT::ArrayExpr *, boost::noncopyable> array_expr("ArrayExpr", bpl::no_init);
  bpl::class_<PT::FuncallExpr, bpl::bases<PT::List>, PT::FuncallExpr *, boost::noncopyable> funcall_expr("FuncallExpr", bpl::no_init);
  bpl::class_<PT::PostfixExpr, bpl::bases<PT::List>, PT::PostfixExpr *, boost::noncopyable> postfix_expr("PostfixExpr", bpl::no_init);
  bpl::class_<PT::DotMemberExpr, bpl::bases<PT::List>, PT::DotMemberExpr *, boost::noncopyable> dot_member_expr("DotMemberExpr", bpl::no_init);
  bpl::class_<PT::ArrowMemberExpr, bpl::bases<PT::List>, PT::ArrowMemberExpr *, boost::noncopyable> arrow_member_expr("ArrowMemberExpr", bpl::no_init);
  bpl::class_<PT::ParenExpr, bpl::bases<PT::List>, PT::ParenExpr *, boost::noncopyable> paren_expr("ParenExpr", bpl::no_init);
}
