//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//
#include <Synopsis/PTree.hh>
#include <boost/python.hpp>

namespace bpl = boost::python;
using namespace Synopsis;
using namespace Synopsis::PTree;

namespace
{

class VisitorWrapper : public Visitor
{
public:
  VisitorWrapper(PyObject *s) : self(s) {}

  virtual void visit_node(Node *) {}
  virtual void visit_atom(Atom *a) { apply("visit_node", a);}
  virtual void visit_list(List *l) { apply("visit_node", l);}

  virtual void visit_literal(Literal *a) { apply("visit_atom", a);}
  virtual void visit_commented_atom(CommentedAtom *a) { apply("visit_atom", a);}
  virtual void visit_dup_atom(DupAtom *a) { apply("visit_atom", a);}
  virtual void visit_identifier(Identifier *a) { apply("visit_atom", a);}
  virtual void visit_reserved(Reserved *a) { apply("visit_atom", a);}
  virtual void visit_this(This *a) { apply("visit_atom", a);}
  virtual void visit_auto(AtomAUTO *a) { apply("visit_atom", a);}
  virtual void visit_boolean(AtomBOOLEAN *a) { apply("visit_atom", a);}
  virtual void visit_char(AtomCHAR *a) { apply("visit_atom", a);}
  virtual void visit_wchar(AtomWCHAR *a) { apply("visit_atom", a);}
  virtual void visit_const(AtomCONST *a) { apply("visit_atom", a);}
  virtual void visit_double(AtomDOUBLE *a) { apply("visit_atom", a);}
  virtual void visit_extern(AtomEXTERN *a) { apply("visit_atom", a);}
  virtual void visit_float(AtomFLOAT *a) { apply("visit_atom", a);}
  virtual void visit_friend(AtomFRIEND *a) { apply("visit_atom", a);}
  virtual void visit_inline(AtomINLINE *a) { apply("visit_atom", a);}
  virtual void visit_int(AtomINT *a) { apply("visit_atom", a);}
  virtual void visit_long(AtomLONG *a) { apply("visit_atom", a);}
  virtual void visit_mutable(AtomMUTABLE *a) { apply("visit_atom", a);}
  virtual void visit_namespace(AtomNAMESPACE *a) { apply("visit_atom", a);}
  virtual void visit_private(AtomPRIVATE *a) { apply("visit_atom", a);}
  virtual void visit_protected(AtomPROTECTED *a) { apply("visit_atom", a);}
  virtual void visit_public(AtomPUBLIC *a) { apply("visit_atom", a);}
  virtual void visit_register(AtomREGISTER *a) { apply("visit_atom", a);}
  virtual void visit_short(AtomSHORT *a) { apply("visit_atom", a);}
  virtual void visit_signed(AtomSIGNED *a) { apply("visit_atom", a);}
  virtual void visit_static(AtomSTATIC *a) { apply("visit_atom", a);}
  virtual void visit_unsigned(AtomUNSIGNED *a) { apply("visit_atom", a);}
  virtual void visit_using_(AtomUSING *a) { apply("visit_atom", a);}
  virtual void visit_virtual(AtomVIRTUAL *a) { apply("visit_atom", a);}
  virtual void visit_void(AtomVOID *a) { apply("visit_atom", a);}
  virtual void visit_volatile(AtomVOLATILE *a) { apply("visit_atom", a);}

  virtual void visit_brace(Brace *l) { apply("visit_list", l);}
  virtual void visit_block(Block *l) { apply("visit_brace", l);}
  virtual void visit_class_body(ClassBody *l) { apply("visit_brace", l);}
  virtual void visit_typedef(Typedef *l) { apply("visit_list", l);}
  virtual void visit_template_decl(TemplateDecl *l) { apply("visit_list", l);}
  virtual void visit_template_instantiation(TemplateInstantiation *l) { apply("visit_list", l);}
  virtual void visit_extern_template(ExternTemplate *l) { apply("visit_list", l);}
  virtual void visit_metaclass_decl(MetaclassDecl *l) { apply("visit_list", l);}
  virtual void visit_linkage_spec(LinkageSpec *l) { apply("visit_list", l);}
  virtual void visit_namespace_spec(NamespaceSpec *l) { apply("visit_list", l);}
  virtual void visit_namespace_alias(NamespaceAlias *l) { apply("visit_list", l);}
  virtual void visit_using(Using *l) { apply("visit_list", l);}
  virtual void visit_declaration(Declaration *l) { apply("visit_list", l);}
  virtual void visit_declarator(Declarator *l) { apply("visit_list", l);}
  virtual void visit_name(Name *l) { apply("visit_list", l);}
  virtual void visit_fstyle_cast_expr(FstyleCastExpr *l) { apply("visit_list", l);}
  virtual void visit_class_spec(ClassSpec *l) { apply("visit_list", l);}
  virtual void visit_enum_spec(EnumSpec *l) { apply("visit_list", l);}
  virtual void visit_access_spec(AccessSpec *l) { apply("visit_list", l);}
  virtual void visit_access_decl(AccessDecl *l) { apply("visit_list", l);}
  virtual void visit_user_access_spec(UserAccessSpec *l) { apply("visit_list", l);}
  virtual void visit_if_statement(IfStatement *l) { apply("visit_list", l);}
  virtual void visit_switch_statement(SwitchStatement *l) { apply("visit_list", l);}
  virtual void visit_while_statement(WhileStatement *l) { apply("visit_list", l);}
  virtual void visit_do_statement(DoStatement *l) { apply("visit_list", l);}
  virtual void visit_for_statement(ForStatement *l) { apply("visit_list", l);}
  virtual void visit_try_statement(TryStatement *l) { apply("visit_list", l);}
  virtual void visit_break_statement(BreakStatement *l) { apply("visit_list", l);}
  virtual void visit_continue_statement(ContinueStatement *l) { apply("visit_list", l);}
  virtual void visit_return_statement(ReturnStatement *l) { apply("visit_list", l);}
  virtual void visit_goto_statement(GotoStatement *l) { apply("visit_list", l);}
  virtual void visit_case_statement(CaseStatement *l) { apply("visit_list", l);}
  virtual void visit_default_statement(DefaultStatement *l) { apply("visit_list", l);}
  virtual void visit_label_statement(LabelStatement *l) { apply("visit_list", l);}
  virtual void visit_expr_statement(ExprStatement *l) { apply("visit_list", l);}
  virtual void visit_comma_expr(CommaExpr *l) { apply("visit_list", l);}
  virtual void visit_assign_expr(AssignExpr *l) { apply("visit_list", l);}
  virtual void visit_cond_expr(CondExpr *l) { apply("visit_list", l);}
  virtual void visit_infix_expr(InfixExpr *l) { apply("visit_list", l);}
  virtual void visit_pm_expr(PmExpr *l) { apply("visit_list", l);}
  virtual void visit_cast_expr(CastExpr *l) { apply("visit_list", l);}
  virtual void visit_unary_expr(UnaryExpr *l) { apply("visit_list", l);}
  virtual void visit_throw_expr(ThrowExpr *l) { apply("visit_list", l);}
  virtual void visit_sizeof_expr(SizeofExpr *l) { apply("visit_list", l);}
  virtual void visit_typeid_expr(TypeidExpr *l) { apply("visit_list", l);}
  virtual void visit_typeof_expr(TypeofExpr *l) { apply("visit_list", l);}
  virtual void visit_new_expr(NewExpr *l) { apply("visit_list", l);}
  virtual void visit_delete_expr(DeleteExpr *l) { apply("visit_list", l);}
  virtual void visit_array_expr(ArrayExpr *l) { apply("visit_list", l);}
  virtual void visit_funcall_expr(FuncallExpr *l) { apply("visit_list", l);}
  virtual void visit_postfix_expr(PostfixExpr *l) { apply("visit_list", l);}
  virtual void visit_dot_member_expr(DotMemberExpr *l) { apply("visit_list", l);}
  virtual void visit_arrow_member_expr(ArrowMemberExpr *l) { apply("visit_list", l);}
  virtual void visit_paren_expr(ParenExpr *l) { apply("visit_list", l);}

  virtual void visit(Node *n) { apply("visit_node", n);}
  virtual void visit(Atom *n) { apply("visit_atom", n);}
  virtual void visit(List* n) { apply("visit_list", n);}
  virtual void visit(Literal *n) { apply("visit_literal", n);}
  virtual void visit(CommentedAtom *n) { apply("visit_commented_atom", n);}
  virtual void visit(DupAtom *n) { apply("visit_dup_atom", n);}
  virtual void visit(Identifier *n) { apply("visit_identifier", n);}
  virtual void visit(Reserved *n) { apply("visit_reserved", n);}
  virtual void visit(This *n) { apply("visit_this", n);}
  virtual void visit(AtomAUTO *n) { apply("visit_auto", n);}
  virtual void visit(AtomBOOLEAN *n) { apply("visit_boolean", n);}
  virtual void visit(AtomCHAR *n) { apply("visit_char", n);}
  virtual void visit(AtomWCHAR *n) { apply("visit_wchar", n);}
  virtual void visit(AtomCONST *n) { apply("visit_const", n);}
  virtual void visit(AtomDOUBLE *n) { apply("visit_double", n);}
  virtual void visit(AtomEXTERN *n) { apply("visit_extern", n);}
  virtual void visit(AtomFLOAT *n) { apply("visit_float", n);}
  virtual void visit(AtomFRIEND *n) { apply("visit_friend", n);}
  virtual void visit(AtomINLINE *n) { apply("visit_inline", n);}
  virtual void visit(AtomINT *n) { apply("visit_int", n);}
  virtual void visit(AtomLONG *n) { apply("visit_long", n);}
  virtual void visit(AtomMUTABLE *n) { apply("visit_mutable", n);}
  virtual void visit(AtomNAMESPACE *n) { apply("visit_namespace", n);}
  virtual void visit(AtomPRIVATE *n) { apply("visit_private", n);}
  virtual void visit(AtomPROTECTED *n) { apply("visit_protected", n);}
  virtual void visit(AtomPUBLIC *n) { apply("visit_public", n);}
  virtual void visit(AtomREGISTER *n) { apply("visit_register", n);}
  virtual void visit(AtomSHORT *n) { apply("visit_short", n);}
  virtual void visit(AtomSIGNED *n) { apply("visit_signed", n);}
  virtual void visit(AtomSTATIC *n) { apply("visit_static", n);}
  virtual void visit(AtomUNSIGNED *n) { apply("visit_unsigned", n);}
  virtual void visit(AtomUSING *n) { apply("visit_using_", n);}
  virtual void visit(AtomVIRTUAL *n) { apply("visit_virtual", n);}
  virtual void visit(AtomVOID *n) { apply("visit_void", n);}
  virtual void visit(AtomVOLATILE *n) { apply("visit_volatile", n);}
  virtual void visit(Brace *n) { apply("visit_brace", n);}
  virtual void visit(Block *n) { apply("visit_block", n);}
  virtual void visit(ClassBody *n) { apply("visit_class_body", n);}
  virtual void visit(Typedef *n) { apply("visit_typedef", n);}
  virtual void visit(TemplateDecl *n) { apply("visit_template_decl", n);}
  virtual void visit(TemplateInstantiation *n) { apply("visit_template_instantiation", n);}
  virtual void visit(ExternTemplate *n) { apply("visit_extern_template", n);}
  virtual void visit(MetaclassDecl *n) { apply("visit_metaclass_decl", n);}
  virtual void visit(LinkageSpec *n) { apply("visit_linkage_spec", n);}
  virtual void visit(NamespaceSpec *n) { apply("visit_namespace_spec", n);}
  virtual void visit(NamespaceAlias *n) { apply("visit_namespace_alias", n);}
  virtual void visit(Using *n) { apply("visit_using", n);}
  virtual void visit(Declaration *n) { apply("visit_declaration", n);}
  virtual void visit(Declarator *n) { apply("visit_declarator", n);}
  virtual void visit(Name *n) { apply("visit_name", n);}
  virtual void visit(FstyleCastExpr *n) { apply("visit_fstyle_cast_expr", n);}
  virtual void visit(ClassSpec *n) { apply("visit_class_spec", n);}
  virtual void visit(EnumSpec *n) { apply("visit_enum_spec", n);}
  virtual void visit(AccessSpec *n) { apply("visit_access_spec", n);}
  virtual void visit(AccessDecl *n) { apply("visit_access_decl", n);}
  virtual void visit(UserAccessSpec *n) { apply("visit_user_access_spec", n);}
  virtual void visit(IfStatement *n) { apply("visit_if_statement", n);}
  virtual void visit(SwitchStatement *n) { apply("visit_switch_statement", n);}
  virtual void visit(WhileStatement *n) { apply("visit_while_statement", n);}
  virtual void visit(DoStatement *n) { apply("visit_do_statement", n);}
  virtual void visit(ForStatement *n) { apply("visit_for_statement", n);}
  virtual void visit(TryStatement *n) { apply("visit_try_statement", n);}
  virtual void visit(BreakStatement *n) { apply("visit_break_statement", n);}
  virtual void visit(ContinueStatement *n) { apply("visit_continue_statement", n);}
  virtual void visit(ReturnStatement *n) { apply("visit_return_statement", n);}
  virtual void visit(GotoStatement *n) { apply("visit_goto_statement", n);}
  virtual void visit(CaseStatement *n) { apply("visit_case_statement", n);}
  virtual void visit(DefaultStatement *n) { apply("visit_default_statement", n);}
  virtual void visit(LabelStatement *n) { apply("visit_label_statement", n);}
  virtual void visit(ExprStatement *n) { apply("visit_expr_statement", n);}
  virtual void visit(CommaExpr *n) { apply("visit_comma_expr", n);}
  virtual void visit(AssignExpr *n) { apply("visit_assign_expr", n);}
  virtual void visit(CondExpr *n) { apply("visit_cond_expr", n);}
  virtual void visit(InfixExpr *n) { apply("visit_infix_expr", n);}
  virtual void visit(PmExpr *n) { apply("visit_pm_expr", n);}
  virtual void visit(CastExpr *n) { apply("visit_cast_expr", n);}
  virtual void visit(UnaryExpr *n) { apply("visit_unary_expr", n);}
  virtual void visit(ThrowExpr *n) { apply("visit_throw_expr", n);}
  virtual void visit(SizeofExpr *n) { apply("visit_sizeof_expr", n);}
  virtual void visit(TypeidExpr *n) { apply("visit_typeid_expr", n);}
  virtual void visit(TypeofExpr *n) { apply("visit_typeof_expr", n);}
  virtual void visit(NewExpr *n) { apply("visit_new_expr", n);}
  virtual void visit(DeleteExpr *n) { apply("visit_delete_expr", n);}
  virtual void visit(ArrayExpr *n) { apply("visit_array_expr", n);}
  virtual void visit(FuncallExpr *n) { apply("visit_funcall_expr", n);}
  virtual void visit(PostfixExpr *n) { apply("visit_postfix_expr", n);}
  virtual void visit(DotMemberExpr *n) { apply("visit_dot_member_expr", n);}
  virtual void visit(ArrowMemberExpr *n) { apply("visit_arrow_member_expr", n);}
  virtual void visit(ParenExpr *n) { apply("visit_paren_expr", n);}

private:
  void apply(char const *f, Node* n) { bpl::call_method<void>(self, f, bpl::ptr(n));}

  PyObject *self;
};

std::string as_string(Encoding const &e) 
{
  std::ostringstream oss;
  oss << e;
  return oss.str();
}

void accept(Node *n, Visitor *v) { n->accept(v);}
std::string atom_value(Atom *a) { return std::string(a->position(), a->length());}
Node *car(List *l) { return l->car();}
Node *cdr(List *l) { return l->cdr();}
Node *first_(List *l) { return PTree::first(l);}
Node *second_(List *l) { return PTree::second(l);}
Node *third_(List *l) { return PTree::third(l);}
Node *nth_(List *l, size_t k) { return PTree::nth(l, k);}
Node *rest_(List *l) { return PTree::rest(l);}
Node *tail_(List *l, size_t k) { return PTree::tail(l, k);}

}

BOOST_PYTHON_MODULE(PTree)
{
  bpl::class_<Encoding> encoding("Encoding");
  encoding.def("__str__", as_string);
  encoding.add_property("is_simple_name", &Encoding::is_simple_name);
  encoding.add_property("is_global_scope", &Encoding::is_global_scope);
  encoding.add_property("is_qualified", &Encoding::is_qualified);
  encoding.add_property("is_function", &Encoding::is_function);
  encoding.add_property("is_template", &Encoding::is_template);

  bpl::class_<Visitor, boost::noncopyable, VisitorWrapper> visitor("Visitor");
  visitor.def("visit_node", &VisitorWrapper::visit_node);
  visitor.def("visit_atom", &VisitorWrapper::visit_atom);
  visitor.def("visit_list", &VisitorWrapper::visit_list);
  visitor.def("visit_literal", &VisitorWrapper::visit_literal);
  visitor.def("visit_commented_atom", &VisitorWrapper::visit_commented_atom);
  visitor.def("visit_dup_atom", &VisitorWrapper::visit_dup_atom);
  visitor.def("visit_identifier", &VisitorWrapper::visit_identifier);
  visitor.def("visit_reserved", &VisitorWrapper::visit_reserved);
  visitor.def("visit_this", &VisitorWrapper::visit_this);
  visitor.def("visit_auto", &VisitorWrapper::visit_auto);
  visitor.def("visit_boolean", &VisitorWrapper::visit_boolean);
  visitor.def("visit_char", &VisitorWrapper::visit_char);
  visitor.def("visit_wchar", &VisitorWrapper::visit_wchar);
  visitor.def("visit_const", &VisitorWrapper::visit_const);
  visitor.def("visit_double", &VisitorWrapper::visit_double);
  visitor.def("visit_extern", &VisitorWrapper::visit_extern);
  visitor.def("visit_float", &VisitorWrapper::visit_float);
  visitor.def("visit_friend", &VisitorWrapper::visit_friend);
  visitor.def("visit_inline", &VisitorWrapper::visit_inline);
  visitor.def("visit_int", &VisitorWrapper::visit_int);
  visitor.def("visit_long", &VisitorWrapper::visit_long);
  visitor.def("visit_mutable", &VisitorWrapper::visit_mutable);
  visitor.def("visit_namespace", &VisitorWrapper::visit_namespace);
  visitor.def("visit_private", &VisitorWrapper::visit_private);
  visitor.def("visit_protected", &VisitorWrapper::visit_protected);
  visitor.def("visit_public", &VisitorWrapper::visit_public);
  visitor.def("visit_register", &VisitorWrapper::visit_register);
  visitor.def("visit_short", &VisitorWrapper::visit_short);
  visitor.def("visit_signed", &VisitorWrapper::visit_signed);
  visitor.def("visit_static", &VisitorWrapper::visit_static);
  visitor.def("visit_unsigned", &VisitorWrapper::visit_unsigned);
  visitor.def("visit_using_", &VisitorWrapper::visit_using_);
  visitor.def("visit_virtual", &VisitorWrapper::visit_virtual);
  visitor.def("visit_void", &VisitorWrapper::visit_void);
  visitor.def("visit_volatile", &VisitorWrapper::visit_volatile);
  visitor.def("visit_brace", &VisitorWrapper::visit_brace);
  visitor.def("visit_block", &VisitorWrapper::visit_block);
  visitor.def("visit_class_body", &VisitorWrapper::visit_class_body);
  visitor.def("visit_typedef", &VisitorWrapper::visit_typedef);
  visitor.def("visit_template_decl", &VisitorWrapper::visit_template_decl);
  visitor.def("visit_template_instantiation", &VisitorWrapper::visit_template_instantiation);
  visitor.def("visit_extern_template", &VisitorWrapper::visit_extern_template);
  visitor.def("visit_metaclass_decl", &VisitorWrapper::visit_metaclass_decl);
  visitor.def("visit_linkage_spec", &VisitorWrapper::visit_linkage_spec);
  visitor.def("visit_namespace_spec", &VisitorWrapper::visit_namespace_spec);
  visitor.def("visit_namespace_alias", &VisitorWrapper::visit_namespace_alias);
  visitor.def("visit_using", &VisitorWrapper::visit_using);
  visitor.def("visit_declaration", &VisitorWrapper::visit_declaration);
  visitor.def("visit_declarator", &VisitorWrapper::visit_declarator);
  visitor.def("visit_name", &VisitorWrapper::visit_name);
  visitor.def("visit_fstyle_cast_expr", &VisitorWrapper::visit_fstyle_cast_expr);
  visitor.def("visit_class_spec", &VisitorWrapper::visit_class_spec);
  visitor.def("visit_enum_spec", &VisitorWrapper::visit_enum_spec);
  visitor.def("visit_access_spec", &VisitorWrapper::visit_access_spec);
  visitor.def("visit_access_decl", &VisitorWrapper::visit_access_decl);
  visitor.def("visit_user_access_spec", &VisitorWrapper::visit_user_access_spec);
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
  visitor.def("visit_comma_expr", &VisitorWrapper::visit_comma_expr);
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

  bpl::class_<Node, Node *, boost::noncopyable> node("Node", bpl::no_init);
  // no idea why we can't pass &Node::accept here, but if we do,
  // the registry will complain that there is no lvalue conversion...
  node.def("accept", accept);
  bpl::class_<Atom, bpl::bases<Node>, Atom *, boost::noncopyable> atom("Atom", bpl::no_init);
  atom.def("__str__", atom_value);
  atom.add_property("position", &Atom::position);
  atom.add_property("length", &Atom::length);
  bpl::class_<List, bpl::bases<Node>, List *, boost::noncopyable> list("List", bpl::no_init);
  // The PTree module uses garbage collection so just ignore memory management,
  // at least for now.
  list.def("car", car, bpl::return_value_policy<bpl::reference_existing_object>());
  list.def("cdr", cdr, bpl::return_value_policy<bpl::reference_existing_object>());
  list.def("first", first_, bpl::return_value_policy<bpl::reference_existing_object>());
  list.def("second", second_, bpl::return_value_policy<bpl::reference_existing_object>());
  list.def("third", third_, bpl::return_value_policy<bpl::reference_existing_object>());
  list.def("nth", nth_, bpl::return_value_policy<bpl::reference_existing_object>());
  list.def("rest", rest_, bpl::return_value_policy<bpl::reference_existing_object>());
  list.def("tail", tail_, bpl::return_value_policy<bpl::reference_existing_object>());
  list.add_property("name", &List::encoded_name);
  list.add_property("type", &List::encoded_type);

  // various atoms

  bpl::class_<Literal, bpl::bases<Atom>, Literal *, boost::noncopyable> literal("Literal", bpl::no_init);
  bpl::class_<CommentedAtom, bpl::bases<Atom>, CommentedAtom *, boost::noncopyable> comment_atom("CommentedAtom", bpl::no_init);
  bpl::class_<DupAtom, bpl::bases<CommentedAtom>, DupAtom *, boost::noncopyable> dup_atom("DupAtom", bpl::no_init);
  bpl::class_<Identifier, bpl::bases<CommentedAtom>, Identifier *, boost::noncopyable> identifier("Identifier", bpl::no_init);
  bpl::class_<Reserved, bpl::bases<CommentedAtom>, Reserved *, boost::noncopyable> reserved("Reserved", bpl::no_init);
  bpl::class_<This, bpl::bases<Reserved>, This *, boost::noncopyable> this_("This", bpl::no_init);
  bpl::class_<AtomAUTO, bpl::bases<Reserved>, AtomAUTO *, boost::noncopyable> atom_auto("AtomAUTO", bpl::no_init);
  bpl::class_<AtomBOOLEAN, bpl::bases<Reserved>, AtomBOOLEAN *, boost::noncopyable> atom_boolean("AtomBOOLEAN", bpl::no_init);
  bpl::class_<AtomCHAR, bpl::bases<Reserved>, AtomCHAR *, boost::noncopyable> atom_char("AtomCHAR", bpl::no_init);
  bpl::class_<AtomWCHAR, bpl::bases<Reserved>, AtomWCHAR *, boost::noncopyable> atom_wchar("AtomWCHAR", bpl::no_init);
  bpl::class_<AtomCONST, bpl::bases<Reserved>, AtomCONST *, boost::noncopyable> atom_const("AtomCONST", bpl::no_init);
  bpl::class_<AtomDOUBLE, bpl::bases<Reserved>, AtomDOUBLE *, boost::noncopyable> atom_double("AtomDOUBLE", bpl::no_init);
  bpl::class_<AtomEXTERN, bpl::bases<Reserved>, AtomEXTERN *, boost::noncopyable> atom_extern("AtomEXTERN", bpl::no_init);
  bpl::class_<AtomFLOAT, bpl::bases<Reserved>, AtomFLOAT *, boost::noncopyable> atom_float("AtomFLOAT", bpl::no_init);
  bpl::class_<AtomFRIEND, bpl::bases<Reserved>, AtomFRIEND *, boost::noncopyable> atom_friend("AtomFRIEND", bpl::no_init);
  bpl::class_<AtomINLINE, bpl::bases<Reserved>, AtomINLINE *, boost::noncopyable> atom_inline("AtomINLINE", bpl::no_init);
  bpl::class_<AtomINT, bpl::bases<Reserved>, AtomINT *, boost::noncopyable> atom_int("AtomINT", bpl::no_init);
  bpl::class_<AtomLONG, bpl::bases<Reserved>, AtomLONG *, boost::noncopyable> atom_long("AtomLONG", bpl::no_init);
  bpl::class_<AtomMUTABLE, bpl::bases<Reserved>, AtomMUTABLE *, boost::noncopyable> atom_mutable("AtomMUTABLE", bpl::no_init);
  bpl::class_<AtomNAMESPACE, bpl::bases<Reserved>, AtomNAMESPACE *, boost::noncopyable> atom_namespace("AtomNAMESPACE", bpl::no_init);
  bpl::class_<AtomPRIVATE, bpl::bases<Reserved>, AtomPRIVATE *, boost::noncopyable> atom_private("AtomPRIVATE", bpl::no_init);
  bpl::class_<AtomPROTECTED, bpl::bases<Reserved>, AtomPROTECTED *, boost::noncopyable> atom_protected("AtomPROTECTED", bpl::no_init);
  bpl::class_<AtomPUBLIC, bpl::bases<Reserved>, AtomPUBLIC *, boost::noncopyable> atom_public("AtomPUBLIC", bpl::no_init);
  bpl::class_<AtomREGISTER, bpl::bases<Reserved>, AtomREGISTER *, boost::noncopyable> atom_register("AtomREGISTER", bpl::no_init);
  bpl::class_<AtomSHORT, bpl::bases<Reserved>, AtomSHORT *, boost::noncopyable> atom_short("AtomSHORT", bpl::no_init);
  bpl::class_<AtomSIGNED, bpl::bases<Reserved>, AtomSIGNED *, boost::noncopyable> atom_signed("AtomSIGNED", bpl::no_init);
  bpl::class_<AtomSTATIC, bpl::bases<Reserved>, AtomSTATIC *, boost::noncopyable> atom_static("AtomSTATIC", bpl::no_init);
  bpl::class_<AtomUNSIGNED, bpl::bases<Reserved>, AtomUNSIGNED *, boost::noncopyable> atom_unsigned("AtomUNSIGNED", bpl::no_init);
  bpl::class_<AtomUSING, bpl::bases<Reserved>, AtomUSING *, boost::noncopyable> atom_using("AtomUSING", bpl::no_init);
  bpl::class_<AtomVIRTUAL, bpl::bases<Reserved>, AtomVIRTUAL *, boost::noncopyable> atom_virtual("AtomVIRTUAL", bpl::no_init);
  bpl::class_<AtomVOID, bpl::bases<Reserved>, AtomVOID *, boost::noncopyable> atom_void("AtomVOID", bpl::no_init);
  bpl::class_<AtomVOLATILE, bpl::bases<Reserved>, AtomVOLATILE *, boost::noncopyable> atom_volatile("AtomVOLATILE", bpl::no_init);
  bpl::class_<AtomUserKeyword2, bpl::bases<Reserved>, AtomUserKeyword2 *, boost::noncopyable> atom_user_kwd2("AtomUserKeyword2", bpl::no_init);

  // various lists

  bpl::class_<Brace, bpl::bases<List>, Brace *, boost::noncopyable> brace("Brace", bpl::no_init);
  bpl::class_<Block, bpl::bases<Brace>, Block *, boost::noncopyable> block("Block", bpl::no_init);
  bpl::class_<ClassBody, bpl::bases<Brace>, ClassBody *, boost::noncopyable> class_body("ClassBody", bpl::no_init);
  bpl::class_<Typedef, bpl::bases<List>, Typedef *, boost::noncopyable> typedef_("Typedef", bpl::no_init);
  bpl::class_<TemplateDecl, bpl::bases<List>, TemplateDecl *, boost::noncopyable> template_decl("TemplateDecl", bpl::no_init);
  bpl::class_<TemplateInstantiation, bpl::bases<List>, TemplateInstantiation *, boost::noncopyable> template_instantiation("TemplateInstantiation", bpl::no_init);
  bpl::class_<ExternTemplate, bpl::bases<List>, ExternTemplate *, boost::noncopyable> extern_template("ExternTemplate", bpl::no_init);
  bpl::class_<MetaclassDecl, bpl::bases<List>, MetaclassDecl *, boost::noncopyable> meta_class_decl("MetaclassDecl", bpl::no_init);
  bpl::class_<LinkageSpec, bpl::bases<List>, LinkageSpec *, boost::noncopyable> linkage_spec("LinkageSpec", bpl::no_init);
  bpl::class_<NamespaceSpec, bpl::bases<List>, NamespaceSpec *, boost::noncopyable> namespace_spec("NamespaceSpec", bpl::no_init);
  bpl::class_<NamespaceAlias, bpl::bases<List>, NamespaceAlias *, boost::noncopyable> namespace_alias("NamespaceAlias", bpl::no_init);
  bpl::class_<Using, bpl::bases<List>, Using *, boost::noncopyable> using_("Using", bpl::no_init);
  bpl::class_<Declaration, bpl::bases<List>, Declaration *, boost::noncopyable> declaration("Declaration", bpl::no_init);
  bpl::class_<Declarator, bpl::bases<List>, Declarator *, boost::noncopyable> declarator("Declarator", bpl::no_init);
  bpl::class_<Name, bpl::bases<List>, Name *, boost::noncopyable> name("Name", bpl::no_init);
  bpl::class_<FstyleCastExpr, bpl::bases<List>, FstyleCastExpr *, boost::noncopyable> fstyle_cast_expr("FstyleCastExpr", bpl::no_init);
  bpl::class_<ClassSpec, bpl::bases<List>, ClassSpec *, boost::noncopyable> class_spec("ClassSpec", bpl::no_init);
  bpl::class_<EnumSpec, bpl::bases<List>, EnumSpec *, boost::noncopyable> enum_spec("EnumSpec", bpl::no_init);
  bpl::class_<AccessSpec, bpl::bases<List>, AccessSpec *, boost::noncopyable> access_spec("AccessSpec", bpl::no_init);
  bpl::class_<AccessDecl, bpl::bases<List>, AccessDecl *, boost::noncopyable> access_decl("AccessDecl", bpl::no_init);
  bpl::class_<UserAccessSpec, bpl::bases<List>, UserAccessSpec *, boost::noncopyable> user_access_spec("UserAccessSpec", bpl::no_init);
  bpl::class_<UserdefKeyword, bpl::bases<List>, UserdefKeyword *, boost::noncopyable> user_def_kwd("UserdefKeyword", bpl::no_init);

  // statements

  bpl::class_<IfStatement, bpl::bases<List>, IfStatement *, boost::noncopyable> if_statement("IfStatement", bpl::no_init);
  bpl::class_<SwitchStatement, bpl::bases<List>, SwitchStatement *, boost::noncopyable> switch_statement("SwitchStatement", bpl::no_init);
  bpl::class_<WhileStatement, bpl::bases<List>, WhileStatement *, boost::noncopyable> while_statement("WhileStatement", bpl::no_init);
  bpl::class_<DoStatement, bpl::bases<List>, DoStatement *, boost::noncopyable> do_statement("DoStatement", bpl::no_init);
  bpl::class_<ForStatement, bpl::bases<List>, ForStatement *, boost::noncopyable> for_statement("ForStatement", bpl::no_init);
  bpl::class_<TryStatement, bpl::bases<List>, TryStatement *, boost::noncopyable> try_statement("TryStatement", bpl::no_init);
  bpl::class_<BreakStatement, bpl::bases<List>, BreakStatement *, boost::noncopyable> break_statement("BreakStatement", bpl::no_init);
  bpl::class_<ContinueStatement, bpl::bases<List>, ContinueStatement *, boost::noncopyable> continue_statement("ContinueStatement", bpl::no_init);
  bpl::class_<ReturnStatement, bpl::bases<List>, ReturnStatement *, boost::noncopyable> return_statement("ReturnStatement", bpl::no_init);
  bpl::class_<GotoStatement, bpl::bases<List>, GotoStatement *, boost::noncopyable> goto_statement("GotoStatement", bpl::no_init);
  bpl::class_<CaseStatement, bpl::bases<List>, CaseStatement *, boost::noncopyable> case_statement("CaseStatement", bpl::no_init);
  bpl::class_<DefaultStatement, bpl::bases<List>, DefaultStatement *, boost::noncopyable> default_statement("DefaultStatement", bpl::no_init);
  bpl::class_<LabelStatement, bpl::bases<List>, LabelStatement *, boost::noncopyable> label_statement("LabelStatement", bpl::no_init);
  bpl::class_<ExprStatement, bpl::bases<List>, ExprStatement *, boost::noncopyable> expr_statement("ExprStatement", bpl::no_init);

  // expressions

  bpl::class_<CommaExpr, bpl::bases<List>, CommaExpr *, boost::noncopyable> comma_expr("CommaExpr", bpl::no_init);
  bpl::class_<AssignExpr, bpl::bases<List>, AssignExpr *, boost::noncopyable> assign_expr("AssignExpr", bpl::no_init);
  bpl::class_<CondExpr, bpl::bases<List>, CondExpr *, boost::noncopyable> cond_expr("CondExpr", bpl::no_init);
  bpl::class_<InfixExpr, bpl::bases<List>, InfixExpr *, boost::noncopyable> infix_expr("InfixExpr", bpl::no_init);
  bpl::class_<PmExpr, bpl::bases<List>, PmExpr *, boost::noncopyable> pm_expr("PmExpr", bpl::no_init);
  bpl::class_<CastExpr, bpl::bases<List>, CastExpr *, boost::noncopyable> cast_expr("CastExpr", bpl::no_init);
  bpl::class_<UnaryExpr, bpl::bases<List>, UnaryExpr *, boost::noncopyable> unary_expr("UnaryExpr", bpl::no_init);
  bpl::class_<ThrowExpr, bpl::bases<List>, ThrowExpr *, boost::noncopyable> throw_expr("ThrowExpr", bpl::no_init);
  bpl::class_<SizeofExpr, bpl::bases<List>, SizeofExpr *, boost::noncopyable> sizeof_expr("SizeofExpr", bpl::no_init);
  bpl::class_<TypeidExpr, bpl::bases<List>, TypeidExpr *, boost::noncopyable> typeid_expr("TypeidExpr", bpl::no_init);
  bpl::class_<TypeofExpr, bpl::bases<List>, TypeofExpr *, boost::noncopyable> typeof_expr("TypeofExpr", bpl::no_init);
  bpl::class_<NewExpr, bpl::bases<List>, NewExpr *, boost::noncopyable> new_expr("NewExpr", bpl::no_init);
  bpl::class_<DeleteExpr, bpl::bases<List>, DeleteExpr *, boost::noncopyable> delete_expr("DeleteExpr", bpl::no_init);
  bpl::class_<ArrayExpr, bpl::bases<List>, ArrayExpr *, boost::noncopyable> array_expr("ArrayExpr", bpl::no_init);
  bpl::class_<FuncallExpr, bpl::bases<List>, FuncallExpr *, boost::noncopyable> funcall_expr("FuncallExpr", bpl::no_init);
  bpl::class_<PostfixExpr, bpl::bases<List>, PostfixExpr *, boost::noncopyable> postfix_expr("PostfixExpr", bpl::no_init);
  bpl::class_<UserStatementExpr, bpl::bases<List>, UserStatementExpr *, boost::noncopyable> user_statement_expr("UserStatementExpr", bpl::no_init);
  bpl::class_<DotMemberExpr, bpl::bases<List>, DotMemberExpr *, boost::noncopyable> dot_member_expr("DotMemberExpr", bpl::no_init);
  bpl::class_<ArrowMemberExpr, bpl::bases<List>, ArrowMemberExpr *, boost::noncopyable> arrow_member_expr("ArrowMemberExpr", bpl::no_init);
  bpl::class_<ParenExpr, bpl::bases<List>, ParenExpr *, boost::noncopyable> paren_expr("ParenExpr", bpl::no_init);
  bpl::class_<StaticUserStatementExpr, bpl::bases<List>, StaticUserStatementExpr *, boost::noncopyable> static_user_statement_expr("StaticUserStatementExpr", bpl::no_init);
}
