#include "synopsis.hh"
#include <iostream>

#include <map>

#ifdef DO_TRACE
int Trace::level = 0;
#endif

#define assertObject(pyo) if (!pyo) PyErr_Print(); assert(pyo)

// This func is called so you can breakpoint on it
void nullObj()
{
  std::cout << "Null ptr." << std::endl;
  int* i = 0;
  *i = 1;
}

//. A functor that returns true if the declaration is 'main'
struct is_main
{
  is_main(bool onlymain, const std::string &mainfile)
    : m_onlymain(onlymain), m_mainfile(mainfile) {}
  bool operator()(AST::Declaration* decl) {
      // returns true if:
      return !m_onlymain // only_main not set
	  || decl->filename() == m_mainfile // filename is the main file
	  || (dynamic_cast<AST::Namespace*>(decl) != 0); // decl is a namespace
  }
  bool m_onlymain;
  std::string m_mainfile;
};

int count_main(AST::Scope* scope, is_main& func)
{
    int count = 0;
    std::vector<AST::Declaration*>::iterator iter = scope->declarations().begin();
    while (iter != scope->declarations().end()) {
	AST::Scope* subscope = dynamic_cast<AST::Scope*>(*iter);
	if (subscope) count += count_main(subscope, func);
	count += func(*iter++);
    }
    return count;
}

// The compiler firewalled private stuff
struct Synopsis::Private {
    //. Constructor
    Private(Synopsis* s) : m_syn(s), m_main(false, s->m_mainfile) {
	m_cxx = PyString_InternFromString("C++");
	Py_INCREF(Py_None);
	add((AST::Declaration*)NULL, Py_None);
	Py_INCREF(Py_None);
	add((Types::Type*)NULL, Py_None);
    }
    //. Reference to parent synopsis object
    Synopsis* m_syn;
    //. Interned string for "C++"
    PyObject* m_cxx;
    //. Returns the string for "C++"
    PyObject* cxx() { Py_INCREF(m_cxx); return m_cxx; }
    //. is_main functor
    is_main m_main;
    // Sugar
    typedef std::map<void*, PyObject*> ObjMap;
    // Maps from C++ objects to PyObjects
    ObjMap obj_map;

    // Note that these methods always succeed
    
    //. Return the PyObject for the given AST::Declaration
    PyObject* py(AST::Declaration*);
    //. Return the PyObject for the given AST::Inheritance
    PyObject* py(AST::Inheritance*);
    //. Return the PyObject for the given AST::Parameter
    PyObject* py(AST::Parameter*);
    //. Return the PyObject for the given AST::Comment
    PyObject* py(AST::Comment*);
    //. Return the PyObject for the given Types::Type
    PyObject* py(Types::Type*);
    //. Return the PyObject for the given string
    PyObject* py(const std::string &);

    //. Add the given pair
    void add(void* cobj, PyObject* pyobj) {
	if (!pyobj) { nullObj(); }
	obj_map.insert(ObjMap::value_type(cobj, pyobj));
    }

    // Convert a vector to a List
    template <class T>
    PyObject* List(const std::vector<T*>& vec) {
	PyObject* list = PyList_New(vec.size());
	int index = 0;
	std::vector<T*>::const_iterator iter = vec.begin();
	while (iter != vec.end())
	    PyList_SET_ITEM(list, index++, py(*iter++));
	return list;
    }

    // Convert a vector to a Tuple
    template <class T>
    PyObject* Tuple(const std::vector<T*>& vec) {
	PyObject* tuple = PyTuple_New(vec.size());
	int index = 0;
	std::vector<T*>::const_iterator iter = vec.begin();
	while (iter != vec.end())
	    PyTuple_SET_ITEM(tuple, index++, py(*iter++));
	return tuple;
    }

    // Convert a string vector to a List
    PyObject* List(const std::vector<std::string> &vec) {
	PyObject* list = PyList_New(vec.size());
	int index = 0;
	std::vector<std::string>::const_iterator iter = vec.begin();
	while (iter != vec.end())
	    PyList_SET_ITEM(list, index++, py(*iter++));
	return list;
    }
    
    // Convert a string vector to a Tuple
    PyObject* Tuple(const std::vector<std::string> &vec) {
	PyObject* tuple = PyTuple_New(vec.size());
	int index = 0;
	std::vector<std::string>::const_iterator iter = vec.begin();
	while (iter != vec.end())
	    PyTuple_SET_ITEM(tuple, index++, py(*iter++));
	return tuple;
    }
    
};

PyObject* Synopsis::Private::py(AST::Declaration* decl)
{
    ObjMap::iterator iter = obj_map.find(decl);
    if (iter == obj_map.end()) {
	// Need to convert object first
	decl->accept(m_syn);
	iter = obj_map.find(decl);
	if (iter == obj_map.end()) {
	    std::cout << "Fatal: Still not PyObject after converting." << std::endl;
	    throw "Synopsis::Private::py(AST::Declaration*)";
	}
	// Force Declared creation (and inclusion into dictionary)
	py(decl->declared());
    }
    PyObject* obj = iter->second;
    Py_INCREF(obj);
    return obj;
}

PyObject* Synopsis::Private::py(AST::Inheritance* decl)
{
    ObjMap::iterator iter = obj_map.find(decl);
    if (iter == obj_map.end()) {
	// Need to convert object first
	decl->accept(m_syn);
	iter = obj_map.find(decl);
	if (iter == obj_map.end()) {
	    std::cout << "Fatal: Still not PyObject after converting." << std::endl;
	    throw "Synopsis::Private::py(AST::Inheritance*)";
	}
    }
    PyObject* obj = iter->second;
    Py_INCREF(obj);
    return obj;
}
PyObject* Synopsis::Private::py(AST::Parameter* decl)
{
    ObjMap::iterator iter = obj_map.find(decl);
    if (iter == obj_map.end()) {
	// Need to convert object first
	decl->accept(m_syn);
	iter = obj_map.find(decl);
	if (iter == obj_map.end()) {
	    std::cout << "Fatal: Still not PyObject after converting." << std::endl;
	    throw "Synopsis::Private::py(AST::Parameter*)";
	}
    }
    PyObject* obj = iter->second;
    Py_INCREF(obj);
    return obj;
}

PyObject* Synopsis::Private::py(AST::Comment* decl)
{
    ObjMap::iterator iter = obj_map.find(decl);
    if (iter == obj_map.end()) {
	// Need to convert object first
	m_syn->visit_comment(decl);
	iter = obj_map.find(decl);
	if (iter == obj_map.end()) {
	    std::cout << "Fatal: Still not PyObject after converting." << std::endl;
	    throw "Synopsis::Private::py(AST::Comment*)";
	}
    }
    PyObject* obj = iter->second;
    Py_INCREF(obj);
    return obj;
}


PyObject* Synopsis::Private::py(Types::Type* type)
{
    ObjMap::iterator iter = obj_map.find(type);
    if (iter == obj_map.end()) {
	// Need to convert object first
	type->accept(m_syn);
	iter = obj_map.find(type);
	if (iter == obj_map.end()) {
	    std::cout << "Fatal: Still not PyObject after converting." << std::endl;
	    throw "Synopsis::Private::py(Types::Type*)";
	}
    }
    PyObject* obj = iter->second;
    Py_INCREF(obj);
    return obj;
}

PyObject* Synopsis::Private::py(const std::string &str)
{
    PyObject* pystr = PyString_FromStringAndSize(str.data(), str.size());
    PyString_InternInPlace(&pystr);
    return pystr;
}






//
// Class Synopsis
//

Synopsis::Synopsis(const std::string &mainfile, PyObject *decl, PyObject *dict)
        : m_declarations(decl), m_dictionary(dict), m_mainfile(mainfile)
{
    Trace trace("Synopsis::Synopsis");
    m_ast  = PyImport_ImportModule("Synopsis.Core.AST");
    assertObject(m_ast);
    m_type = PyImport_ImportModule("Synopsis.Core.Type");
    assertObject(m_type);

    m_onlymain = false;
    m = new Private(this);
}

Synopsis::~Synopsis()
{
    Trace trace("Synopsis::~Synopsis");
    /*
    PyObject *file = PyObject_CallMethod(scopes.back(), "declarations", 0);
    size_t size = PyList_Size(file);
    for (size_t i = 0; i != size; i++)
        PyObject_CallMethod(declarations, "append", "O", PyList_GetItem(file, i));
    */
    Py_DECREF(m_type);
    Py_DECREF(m_ast);
    delete m;
}

void Synopsis::onlyTranslateMain()
{
    m_onlymain = true;
    m->m_main.m_onlymain = true;
}

void Synopsis::translate(AST::Scope* scope)
{
    PyObject_CallMethod(m_declarations, "extend", "O",
	m->List(scope->declarations())
    );
}

//
// Type object factories
//

PyObject *Synopsis::Base(Types::Base* type)
{
    Trace trace("Synopsis::Base");
    PyObject *name, *base = PyObject_CallMethod(m_type, "Base", "OO",
	m->cxx(), name = m->Tuple(type->name())
    );
    PyObject_SetItem(m_dictionary, name, base);
    return base;
}

PyObject *Synopsis::Unknown(Types::Named* type)
{
    Trace trace("Synopsis::Unknown");
    PyObject *name, *unknown = PyObject_CallMethod(m_type, "Unknown", "OO",
	m->cxx(), name = m->Tuple(type->name())
    );
    PyObject_SetItem(m_dictionary, name, unknown);
    return unknown;
}

PyObject *Synopsis::Declared(Types::Declared* type)
{
    Trace trace("Synopsis::Declared");
    PyObject *name, *declared = PyObject_CallMethod(m_type, "Declared", "OOO", 
	m->cxx(), name = m->Tuple(type->name()), m->py(type->declaration())
    );
    // Skip zero-length names (eg: dummy declarators/enumerators)
    if (type->name().size())
	PyObject_SetItem(m_dictionary, name, declared);
    return declared;
}

PyObject *Synopsis::Template(Types::Template* type)
{
    Trace trace("Synopsis::Template");
    PyObject *name, *templ = PyObject_CallMethod(m_type, "Template", "OOOO",
	m->cxx(), name = m->Tuple(type->name()), m->py(type->declaration()),
	m->List(type->parameters())
    );
    PyObject_SetItem(m_dictionary, name, templ);
    return templ;
}

PyObject *Synopsis::Modifier(Types::Modifier* type)
{
    Trace trace("Synopsis::Modifier");
    PyObject *modifier = PyObject_CallMethod(m_type, "Modifier", "OOOO",
	m->cxx(), m->py(type->alias()), m->List(type->pre()), m->List(type->post())
    );
    return modifier;
}

PyObject *Synopsis::Array(Types::Array *type)
{
    Trace trace("Synopsis::Array");
    return PyObject_CallMethod(m_type, "Array", "OOO", m->cxx(), m->py(type->alias()), m->List(type->sizes()));
}

PyObject *Synopsis::Parameterized(Types::Parameterized* type)
{
    Trace trace("Synopsis::Parametrized");
    PyObject *parametrized = PyObject_CallMethod(m_type, "Parametrized", "OOO",
	m->cxx(), m->py(type->template_type()), m->List(type->parameters())
    );
    return parametrized;
}

PyObject *Synopsis::FuncPtr(Types::FuncPtr* type)
{
    Trace trace("Synopsis::FuncType");
    PyObject *func = PyObject_CallMethod(m_type, "Function", "OOOO",
	m->cxx(), m->py(type->returnType()), m->List(type->pre()),
	m->List(type->parameters())
    );
    return func;
}


//
// AST object factories
//

void Synopsis::addComments(PyObject* pydecl, AST::Declaration* cdecl)
{
    PyObject* comments = PyObject_CallMethod(pydecl, "comments", NULL);
    PyObject_CallMethod(comments, "extend", "O", m->List(cdecl->comments()));
    // Also set the accessability..
    PyObject_CallMethod(pydecl, "set_accessibility", "i", int(cdecl->access()));
}

PyObject *Synopsis::Declaration(AST::Declaration* decl)
{
    Trace trace("Synopsis::addDeclaration");
    PyObject *pydecl = PyObject_CallMethod(m_ast, "Declaration", "OiOOO",
	m->py(decl->filename()), decl->line(), m->cxx(),
	m->py(decl->type()), m->Tuple(decl->name())
    );
    assertObject(pydecl);
    addComments(pydecl, decl);
    return pydecl;
}

PyObject *Synopsis::Forward(AST::Forward* decl)
{
    Trace trace("Synopsis::addForward");
    PyObject *forward = PyObject_CallMethod(m_ast, "Forward", "OiOOO", 
	m->py(decl->filename()), decl->line(), m->cxx(),
	m->py(decl->type()), m->Tuple(decl->name())
    );
    addComments(forward, decl);
    return forward;
}

PyObject *Synopsis::Comment(AST::Comment* decl)
{
    Trace trace("Synopsis::addComment");
    PyObject *text = PyString_FromStringAndSize((decl->text()+"\n").data(), decl->text().size()+1);
    PyObject *comment = PyObject_CallMethod(m_ast, "Comment", "OOi", 
	text, m->py(decl->filename()), decl->line()
    );
    return comment;
}

PyObject *Synopsis::Scope(AST::Scope* decl)
{
    Trace trace("Synopsis::addScope");
    PyObject *scope = PyObject_CallMethod(m_ast, "Scope", "OiOOO", 
	m->py(decl->filename()), decl->line(), m->cxx(),
	m->py(decl->type()), m->Tuple(decl->name())
    );
    PyObject *decls = PyObject_CallMethod(scope, "declarations", NULL);
    PyObject_CallMethod(decls, "extend", "O", m->List(decl->declarations()));
    addComments(scope, decl);
    return scope;
}

PyObject *Synopsis::Namespace(AST::Namespace* decl)
{
    Trace trace("Synopsis::addNamespace");
    PyObject *module = PyObject_CallMethod(m_ast, "Module", "OiOOO", 
	m->py(decl->filename()), decl->line(), m->cxx(),
	m->py(decl->type()), m->Tuple(decl->name())
    );
    PyObject *decls = PyObject_CallMethod(module, "declarations", NULL);
    PyObject_CallMethod(decls, "extend", "O", m->List(decl->declarations()));
    addComments(module, decl);
    return module;
}

PyObject *Synopsis::Inheritance(AST::Inheritance* decl)
{
    Trace trace("Synopsis::Inheritance");
    PyObject *inheritance = PyObject_CallMethod(m_ast, "Inheritance", "sOO", 
	"inherits", m->py(decl->parent()), m->List(decl->attributes())
    );
    return inheritance;
}

PyObject *Synopsis::Class(AST::Class* decl)
{
    Trace trace("Synopsis::addClass");
    PyObject *clas = PyObject_CallMethod(m_ast, "Class", "OiOOO", 
	m->py(decl->filename()), decl->line(), m->cxx(),
	m->py(decl->type()), m->Tuple(decl->name())
    );
    // This is necessary to prevent inf. loops in several places
    m->add(decl, clas);
    PyObject *decls = PyObject_CallMethod(clas, "declarations", NULL);
    PyObject_CallMethod(decls, "extend", "O", m->List(decl->declarations()));
    PyObject *parents = PyObject_CallMethod(clas, "parents", NULL);
    PyObject_CallMethod(parents, "extend", "O", m->List(decl->parents()));
    if (decl->template_type()) {
	PyObject_CallMethod(clas, "set_template", "O", m->py(decl->template_type()));
    }
    addComments(clas, decl);
    return clas;
}

PyObject *Synopsis::Typedef(AST::Typedef* decl)
{
    Trace trace("Synopsis::addTypedef");
    // FIXME: what to do about the declarator?
    PyObject *tdef = PyObject_CallMethod(m_ast, "Typedef", "OiOOOOi", 
	m->py(decl->filename()), decl->line(), m->cxx(),
	m->py(decl->type()), m->Tuple(decl->name()),
	m->py(decl->alias()), decl->constructed()
    );
    addComments(tdef, decl);
    return tdef;
}

PyObject *Synopsis::Enumerator(AST::Enumerator* decl)
{
    Trace trace("Synopsis::addEnumerator");
    PyObject *enumor = PyObject_CallMethod(m_ast, "Enumerator", "OiOOs", 
	m->py(decl->filename()), decl->line(), m->cxx(),
	m->Tuple(decl->name()), decl->value().c_str()
    );
    addComments(enumor, decl);
    return enumor;
}

PyObject *Synopsis::Enum(AST::Enum* decl)
{
    Trace trace("Synopsis::addEnum");
    PyObject *enumor = PyObject_CallMethod(m_ast, "Enum", "OiOOO", 
	m->py(decl->filename()), decl->line(), m->cxx(),
	m->Tuple(decl->name()), m->List(decl->enumerators())
    );
    addComments(enumor, decl);
    return enumor;
}

PyObject *Synopsis::Variable(AST::Variable* decl)
{
    Trace trace("Synopsis::addVariable");
    PyObject *var = PyObject_CallMethod(m_ast, "Variable", "OiOOOOi", 
	m->py(decl->filename()), decl->line(), m->cxx(),
	m->py(decl->type()), m->Tuple(decl->name()),
	m->py(decl->vtype()), decl->constructed()
    );
    addComments(var, decl);
    return var;
}

PyObject *Synopsis::Const(AST::Const* decl)
{
    Trace trace("Synopsis::addConst");
    PyObject *cons = PyObject_CallMethod(m_ast, "Const", "OiOOOOOs", 
	m->py(decl->filename()), decl->line(), m->cxx(),
	m->py(decl->type()),
	m->py(decl->ctype()), m->Tuple(decl->name()), decl->value().c_str()
    );
    addComments(cons, decl);
    return cons;
}

PyObject *Synopsis::Parameter(AST::Parameter* decl)
{
    Trace trace("Synopsis::Parameter");
    PyObject *param = PyObject_CallMethod(m_ast, "Parameter", "OOOOO", 
	m->py(decl->premodifier()), m->py(decl->type()), m->py(decl->postmodifier()),
	m->py(decl->name()), m->py(decl->value())
    );
    return param;
}

PyObject *Synopsis::Function(AST::Function* decl)
{
    Trace trace("Synopsis::addFunction");
    PyObject *func = PyObject_CallMethod(m_ast, "Function", "OiOOOOOO", 
	m->py(decl->filename()), decl->line(), m->cxx(),
	m->py(decl->type()), m->List(decl->premodifier()), m->py(decl->return_type()),
	m->Tuple(decl->name()), m->py(decl->realname())
    );
    PyObject* params = PyObject_CallMethod(func, "parameters", NULL);
    PyObject_CallMethod(params, "extend", "O", m->List(decl->parameters()));
    addComments(func, decl);
    return func;
}

PyObject *Synopsis::Operation(AST::Operation* decl)
{
    Trace trace("Synopsis::addOperation");
    PyObject *oper = PyObject_CallMethod(m_ast, "Operation", "OiOOOOOO", 
	m->py(decl->filename()), decl->line(), m->cxx(),
	m->py(decl->type()), m->List(decl->premodifier()), m->py(decl->return_type()),
	m->Tuple(decl->name()), m->py(decl->realname())
    );
    PyObject* params = PyObject_CallMethod(oper, "parameters", NULL);
    PyObject_CallMethod(params, "extend", "O", m->List(decl->parameters()));
    addComments(oper, decl);
    return oper;
}



////////////////MISC CRAP

/*
void Synopsis::Inheritance(Inheritance* type)
{
    PyObject *to = PyObject_CallMethod(clas, "parents", 0);
    PyObject *from = V2L(parents);
    size_t size = PyList_Size(from);
    for (size_t i = 0; i != size; ++i)
        PyList_Append(to, PyList_GetItem(from, i));
    Py_DECREF(to);
    Py_DECREF(from);
}

PyObject *Synopsis::N2L(const std::string &name)
{
    Trace trace("Synopsis::N2L");
    std::vector<std::string> scope;
    std::string::size_type b = 0;
    while (b < name.size())
    {
        std::string::size_type e = name.find("::", b);
        scope.push_back(name.substr(b, e == std::string::npos ? e : e - b));
        b = e == string::npos ? std::string::npos : e + 2;
    }
    PyObject *pylist = PyList_New(scope.size());
    for (size_t i = 0; i != scope.size(); ++i)
        PyList_SetItem(pylist, i, PyString_FromString(scope[i].c_str()));
    return pylist;
}

PyObject *Synopsis::V2L(const std::vector<std::string> &strings)
{
    Trace trace("Synopsis::V2L(std::vector<std::string>)");
    PyObject *pylist = PyList_New(strings.size());
    for (size_t i = 0; i != strings.size(); ++i)
        PyList_SetItem(pylist, i, PyString_FromString(strings[i].c_str()));
    return pylist;
}

PyObject *Synopsis::V2L(const std::vector<PyObject *> &objs)
{
    Trace trace("Synopsis::V2L(std::vector<PyObject*>)");
    PyObject *pylist = PyList_New(objs.size());
    for (size_t i = 0; i != objs.size(); ++i)
        PyList_SetItem(pylist, i, objs[i]);
    return pylist;
}

PyObject *Synopsis::V2L(const std::vector<size_t> &sizes)
{
    //Trace trace("Synopsis::V2L");
    PyObject *pylist = PyList_New(sizes.size());
    for (size_t i = 0; i != sizes.size(); ++i)
        PyList_SetItem(pylist, i, PyInt_FromLong(sizes[i]));
    return pylist;
}

void Synopsis::Declaration(Declaration* type)
{
    PyObject *scope = scopes.back();
    PyObject *declarations = PyObject_CallMethod(scope, "declarations", 0);
    PyObject_CallMethod(declarations, "append", "O", declaration);
    PyObject_CallMethod(declaration, "set_accessability", "i", m_accessability);
}
*/

//
// AST::Visitor methods
//
void Synopsis::visit_declaration(AST::Declaration* decl) {
    // Assume this is a dummy declaration
    m->add(decl, Declaration(decl));
}
void Synopsis::visit_scope(AST::Scope* decl) {
    if (count_main(decl, m->m_main))
	m->add(decl, Scope(decl));
    else
	m->add(decl, Forward(new AST::Forward(decl)));
}
void Synopsis::visit_namespace(AST::Namespace* decl) {
    // Namespaces are always included, because the Linker knows to combine
    // them always
    m->add(decl, Namespace(decl));
}
void Synopsis::visit_class(AST::Class* decl) {
    if (count_main(decl, m->m_main))
	m->add(decl, Class(decl));
    else
	m->add(decl, Forward(new AST::Forward(decl)));
}
void Synopsis::visit_forward(AST::Forward* decl) {
    if (m->m_main(decl))
	m->add(decl, Forward(decl));
    else
	m->add(decl, Forward(new AST::Forward(decl)));
}
void Synopsis::visit_typedef(AST::Typedef* decl) {
    if (m->m_main(decl))
	m->add(decl, Typedef(decl));
    else
	m->add(decl, Forward(new AST::Forward(decl)));
}
void Synopsis::visit_variable(AST::Variable* decl) {
    if (m->m_main(decl))
	m->add(decl, Variable(decl));
    else
	m->add(decl, Forward(new AST::Forward(decl)));
}
void Synopsis::visit_const(AST::Const* decl) {
    if (m->m_main(decl))
	m->add(decl, Const(decl));
    else
	m->add(decl, Forward(new AST::Forward(decl)));
}
void Synopsis::visit_enum(AST::Enum* decl) {
    if (m->m_main(decl))
	m->add(decl, Enum(decl));
    else
	m->add(decl, Forward(new AST::Forward(decl)));
}
void Synopsis::visit_enumerator(AST::Enumerator* decl) {
    m->add(decl, Enumerator(decl));
}
void Synopsis::visit_function(AST::Function* decl) {
    if (m->m_main(decl))
	m->add(decl, Function(decl));
    else
	m->add(decl, Forward(new AST::Forward(decl)));
}
void Synopsis::visit_operation(AST::Operation* decl) {
    if (m->m_main(decl))
	m->add(decl, Operation(decl));
    else
	m->add(decl, Forward(new AST::Forward(decl)));
}

void Synopsis::visit_inheritance(AST::Inheritance* decl) {
    m->add(decl, Inheritance(decl));
}
void Synopsis::visit_parameter(AST::Parameter* decl) {
    m->add(decl, Parameter(decl));
}
void Synopsis::visit_comment(AST::Comment* decl) {
    m->add(decl, Comment(decl));
}

//
// Types::Visitor methods
//
/*void Synopsis::visitType(Types::Type* type) {
    m->add(type, this->Type(type));
}*/
void Synopsis::visit_unknown(Types::Unknown* type) {
    m->add(type, Unknown(type));
}
void Synopsis::visit_modifier(Types::Modifier* type) {
    m->add(type, Modifier(type));
}
void Synopsis::visit_array(Types::Array *type) { m->add(type, Array(type));}
/*void Synopsis::visitNamed(Types::Named* type) {
    m->add(type, Named(type));
}*/
void Synopsis::visit_base(Types::Base* type) {
    m->add(type, Base(type));
}
void Synopsis::visit_declared(Types::Declared* type) {
    if (!m->m_main(type->declaration()))
	m->add(type, Unknown(type));
    else
	m->add(type, Declared(type));
}
void Synopsis::visit_template_type(Types::Template* type) {
    if (!m->m_main(type->declaration()))
	m->add(type, Unknown(type));
    else
	m->add(type, Template(type));
}
void Synopsis::visit_parameterized(Types::Parameterized* type) {
    m->add(type, Parameterized(type));
}
void Synopsis::visit_func_ptr(Types::FuncPtr* type) {
    m->add(type, FuncPtr(type));
}


