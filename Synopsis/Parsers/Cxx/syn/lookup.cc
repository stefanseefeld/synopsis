// Synopsis C++ Parser: lookup.cc source file
// Implementation of the Lookup class

// $Id: lookup.cc,v 1.10 2002/11/17 12:11:44 chalky Exp $
//
// This file is a part of Synopsis.
// Copyright (C) 2001, 2002 Stephen Davies
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

// $Log: lookup.cc,v $
// Revision 1.10  2002/11/17 12:11:44  chalky
// Reformatted all files with astyle --style=ansi, renamed fakegc.hh
//
//

#include <map>
#include <typeinfo>
#include <sstream>
#include <algorithm>

#include "lookup.hh"
#include "builder.hh"
#include "type.hh"
#include "dict.hh"
#include "swalker.hh"
#include "scopeinfo.hh"
#include "typeinfo.hh"

// for debugging
#include "dumper.hh"
#include "strace.hh"

//. Simplify names. Typically only used for accessing vectors and iterators
using namespace Types;
using namespace AST;

//
// Class ScopeInfo
//
ScopeInfo::ScopeInfo(AST::Scope* s)
        : is_using(false)
{
    scope_decl = s;
    search.push_back(this);
    dict = new Dictionary();
    access = AST::Default;
}

// FIXME: why is there a using scope?
ScopeInfo::ScopeInfo(ScopeInfo* s)
        : is_using(true)
{
    scope_decl = s->scope_decl;
    dict = s->dict;
}

ScopeInfo::~ScopeInfo()
{
    //if (is_using == false)
    //  delete dict;
}

int
ScopeInfo::getCount(const std::string& name)
{
    return ++nscounts[name];
}


/////// FIXME: wtf is this doing here??
namespace
{
//. This class is very similar to ostream_iterator, except that it works on
//. pointers to types
template <typename T>
class ostream_ptr_iterator
{
    std::ostream* out;
    const char* sep;
public:
    ostream_ptr_iterator(std::ostream& o, const char* s) : out(&o), sep(s)
    {}
    ostream_ptr_iterator<T>& operator =(const T* value)
    {
        *out << *value;
        if (sep)
            *out << sep;
        return *this;
    }
    ostream_ptr_iterator<T>& operator *()
    {
        return *this;
    }
    ostream_ptr_iterator<T>& operator ++()
    {
        return *this;
    }
    ostream_ptr_iterator<T>& operator ++(int)
    {
        return *this;
    }
};
}

// defined in common.hh
std::string join(const ScopedName& strs, const std::string& sep)
{
    ScopedName::const_iterator iter = strs.begin();
    if (iter == strs.end())
        return "";
    std::string str = *iter++;
    while (iter != strs.end())
        str += sep + *iter++;
    return str;
}

//. Output operator for debugging
std::ostream& operator << (std::ostream& o, TypeInfo& i)
{
    TypeFormatter tf;
    o << "[" << tf.format(i.type);
    if (i.is_const)
        o << " (const)";
    if (i.is_volatile)
        o << " (volatile)";
    if (i.deref)
        o << " " << i.deref << "*";
    o << "]";
    return o;
}


//
// Struct Lookup::Private
//

#if 0
struct Lookup::Private
{
    //. A map of AST::Scope's ScopeInfo objects
    typedef std::map<AST::Scope*, ScopeInfo*> ScopeMap;
    ScopeMap map;

    //. A map of name references
    typedef std::map<ScopedName, std::vector<AST::Reference> > RefMap;
    RefMap refs;
};
#endif

//
// Class Lookup
//

Lookup::Lookup(Builder* builder)
{
    m_builder = builder;
}

Lookup::~Lookup()
{
    // TODO Delete all ...
    //delete m;
}

//. Finds or creates a cached Scope
ScopeInfo* Lookup::find_info(AST::Scope* decl)
{
    return m_builder->find_info(decl);
}

AST::Scope* Lookup::global()
{
    return m_builder->global();
}

AST::Scope* Lookup::scope()
{
    return m_builder->scope();
}

//
// Type Methods
//

//. Predicate class that is true if the object passed to the constructor is a
//. type, as opposed to a modifier or a parametrized, etc
class isType : public Types::Visitor
{
    bool m_value;
public:
    //. constructor. Visits the given type thereby setting the value
    isType(Types::Named* type) : m_value(false)
    {
        type->accept(this);
    }
    //. bool operator, returns the value determined by visitation during
    //. construction
    operator bool()
    {
        return m_value;
    }
    //. Okay
    void visit_base(Types::Base*)
    {
        m_value = true;
    }
    //. Okay
    void visit_unknown(Types::Unknown*)
    {
        m_value = true;
    }
    //. Okay if not a function declaration
    void visit_declared(Types::Declared* type)
    {
        // Depends on what was declared: Everything but function is okay
        if (dynamic_cast<AST::Function*>(type->declaration()))
            m_value = false;
        else
            m_value = true;
    }
    //. Okay if a template dependent arg
    void visit_dependent(Types::Dependent*)
    {
        m_value = true;
    }
    //. Fallback: Not okay
    void visit_type(Types::Type*)
    {
        m_value = false;
    }
};

// Public method to lookup a type
Types::Named* Lookup::lookupType(const std::string& name, bool func_okay)
{
    STrace trace("Lookup::lookupType(name, func_okay)");
    Types::Named* type = lookup(name, func_okay);
    if (type)
        return type;
    // Not found, declare it unknown
    //cout << "Warning: Name "<<name<<" not found in "<<m_filename<<endl;
    return m_builder->create_unknown(name);
}

// Private method to lookup a type in the current scope
Types::Named* Lookup::lookup(const std::string& name, bool func_okay)
{
    STrace trace("Lookup::lookup(name, func_okay)");
    const ScopeSearch& search = m_builder->scopeinfo()->search;
    return lookup(name, search, func_okay);
}

//. Looks up the name in the scope of the given scope
Types::Named* Lookup::lookupType(const std::string& name, AST::Scope* decl)
{
    STrace trace("Lookup::lookupType(name,scope)");
    ScopeInfo* scope = find_info(decl);
    return lookup(name, scope->search);
}

class FunctionHeuristic
{
    typedef std::vector<Types::Type*> v_Type;
    typedef v_Type::iterator vi_Type;
    typedef std::vector<AST::Parameter*> v_Param;
    typedef v_Param::iterator vi_Param;

    v_Type m_args;
    int cost;
#ifdef DEBUG

    STrace trace;
public:
    //. Constructor - takes arguments to match functions against
    FunctionHeuristic(const v_Type& v)
            : m_args(v), trace("FunctionHeuristic")
    {
        TypeFormatter tf;
        std::ostringstream buf;
        for (size_t index = 0; index < v.size(); index++)
        {
            if (index)
                buf << ", ";
            buf << tf.format(v[index]);
        }
        //buf << std::ends;
        LOG("Function arguments: " << buf.str());
    }
#else
public:
    //. Constructor - takes arguments to match functions against
    FunctionHeuristic(const v_Type& v)
            : m_args(v)
    { }
#endif

    //. Heuristic operator, returns 'cost' of given function - higher is
    //. worse, 1000 means no match
    int operator ()(AST::Function* func)
    {
        cost = 0;
        int num_args = m_args.size();
        v_Param* params =& func->parameters();
        bool func_ellipsis = hasEllipsis(params);
        int num_params = params->size() - func_ellipsis;
        int num_default = countDefault(params);

        // Check number of params first
        if (!func_ellipsis && num_args > num_params)
            cost = 1000;
        if (num_args < num_params - num_default)
            cost = 1000;

        if (cost < 1000)
        {
            // Now calc cost of each argument in turn
            int max_arg = num_args > num_params ? num_params : num_args;
            for (int index = 0; index < max_arg; index++)
                calcCost(m_args[index], (*params)[index]->type());
        }

#ifdef DEBUG
        LOG("Function: " << func->name() << " -- Cost is " << cost);
#endif

        return cost;
    }

private:
    //. Find an ellipsis as the last arg
    bool hasEllipsis(v_Param* params)
    {
        if (params->size() == 0)
            return false;
        Types::Type* back = params->back()->type();
        if (Types::Base* base = dynamic_cast<Types::Base*>(back))
            if (base->name().size() == 1 && base->name().front() == "...")
                return true;
        return false;
    }

    //. Returns the number of parameters with default values. Counts from the
    //. back and stops when it finds one without a default.
    int countDefault(v_Param* params)
    {
        v_Param::reverse_iterator iter = params->rbegin(), end = params->rend();
        int count = 0;
        while (iter != end)
        {
            AST::Parameter* param = *iter++;
            if (!param->value().size())
                break;
            count++;
        }
        return count;
    }

    //. Calculate the cost of converting 'arg' into 'param'. The cost is
    //. accumulated on the 'cost' member variable.
    void calcCost(Types::Type* arg_t, Types::Type* param_t)
    {
        TypeFormatter tf;
        if (!arg_t)
            return;
        TypeInfo arg(arg_t), param(param_t);
#ifdef DEBUG
        // std::cout << arg << param << std::endl;
        // // std::cout << tf.format(type) <<","<<tf.format(param_type) << std::endl;
#endif
        // Null types convert to any ptr with no cost
        if (arg.is_null && param.deref > 0)
            return;
        // Different types is bad
        if (arg.type != param.type)
            cost += 10;
        // Different * levels is also bad
        if (arg.deref != param.deref)
            cost += 10;
        // Worse constness is bad
        if (arg.is_const > param.is_const)
            cost += 5;
    }
};

//. Looks up the function in the given scope with the given args.
AST::Function* Lookup::lookupFunc(const std::string& name, AST::Scope* decl, const std::vector<Types::Type*>& args)
{
    STrace trace("Lookup::lookupFunc");
    TypeFormatter tf;
    // Now loop over the search scopes
    const ScopeSearch& search = find_info(decl)->search;
    ScopeSearch::const_iterator s_iter = search.begin();
    typedef std::vector<AST::Function*> v_Function;
    v_Function functions;

    // Loop over precalculated search list
    while (s_iter != search.end())
    {
        ScopeInfo* scope = *s_iter++;

        // Check if dict has it
        if (scope->dict->has_key(name))
        {
            findFunctions(name, scope, functions);
        }
        // If not a dummy scope, resolve the set
        if (scope->is_using == false && !functions.empty())
        {
            // Return best function (or throw error)
            int cost;
            AST::Function* func = bestFunction(functions, args, cost);
            if (cost < 1000)
                return func;
            throw ERROR("No appropriate function found.");
        }
    }

    throw ERROR("No matching functions found.");
}

// Operator lookup
AST::Function* Lookup::lookupOperator(const std::string& oper, Types::Type* left_type, Types::Type* right_type)
{
    STrace trace("Lookup::lookupOperator("+oper+",left,right)");
    // Find some info about the two types
    TypeInfo left(left_type), right(right_type);
    bool left_user = !!dynamic_cast<Types::Declared*>(left_type) && !left.deref;
    bool right_user = !!dynamic_cast<Types::Declared*>(right_type) && !right.deref;

    // First check if the types are user-def or enum
    if (!left_user && !right_user)
        return NULL;

    std::vector<AST::Function*> functions;
    std::vector<Types::Type*> args;
    AST::Function* best_method = NULL, *best_func = NULL;
    int best_method_cost, best_func_cost;

    // Member methods of left_type
    try
    {
        AST::Class* clas = Types::declared_cast<AST::Class>(left.type);
        // Construct the argument list
        args.push_back(right_type);

        try
        {
            findFunctions(oper, find_info(clas), functions);

            best_method = bestFunction(functions, args, best_method_cost);
        }
        catch (const Dictionary::KeyError&)
        {
            best_method = NULL;
        }

        // Clear functions and args for use below
        functions.clear();
        args.clear();
    }
    catch (const Types::wrong_type_cast&)
    { /* ignore: not a class */
    }

    // Non-member functions
    // Loop over the search scopes
    const ScopeSearch& search = m_builder->scopeinfo()->search;
    ScopeSearch::const_iterator s_iter = search.begin();
    while (s_iter != search.end())
    {
        ScopeInfo* scope = *s_iter++;
        // Check if dict has any names that match
        if (!scope->dict->has_key(oper))
            continue;

        // Get the matching functions from the dictionary
        findFunctions(oper, scope, functions);

        // Scope rules say once you find any results: stop
        break;
    }

    // Koenig Rule: add operators from namespaces of arguments
    // void findKoenigFunctions(oper, functions, args);
    if (left_user)
        try
        {
            ScopedName enclosing_name = Types::type_cast<Types::Named>(left.type)->name();
            enclosing_name.pop_back();
            if (enclosing_name.size())
            {
                ScopeInfo* scope = find_info( Types::declared_cast<AST::Scope>(lookupType(enclosing_name, false, global())) );
                findFunctions(oper, scope, functions);
            }
        }
        catch (const Types::wrong_type_cast& e)
        {}

    if (right_user)
        try
        {
            ScopedName enclosing_name = Types::type_cast<Types::Named>(right.type)->name();
            enclosing_name.pop_back();
            if (enclosing_name.size())
            {
                ScopeInfo* scope = find_info( Types::declared_cast<AST::Scope>(lookupType(enclosing_name, false, global())) );
                findFunctions(oper, scope, functions);
            }
        }
        catch (const Types::wrong_type_cast& e)
        {}

    // Add builtin operators to aide in best-function resolution
    // NYI

    // Puts left and right types into args
    args.push_back(left_type);
    args.push_back(right_type);
    // Find best non-method function
    best_func = bestFunction(functions, args, best_func_cost);

    // Return best method or function
    if (best_method)
    {
        if (best_func)
        {
            if (best_func_cost < best_method_cost)
                return best_func;
            else
                return best_method;
        }
        else
        {
            return best_method;
        }
    }
    else
    {
        if (best_func)
            return best_func;
        else
            return NULL;
    }
}

void Lookup::findFunctions(const std::string& name, ScopeInfo* scope, AST::Function::vector& functions)
{
    STrace trace("Lookup::findFunctions");

    // Get the matching names from the dictionary
    try
    {
        Named::vector types = scope->dict->lookupMultiple(name);

        // Put only the AST::Functions into 'functions'
        for (Named::vector::iterator iter = types.begin(); iter != types.end();)
            try
            {
                functions.push_back( Types::declared_cast<AST::Function>(*iter++) );
            }
            catch (const Types::wrong_type_cast& )
            {
                throw ERROR("looked up func '"<<name<<"'wasnt a func!");
            }
    }
    catch (Dictionary::KeyError)
    { }
}

AST::Function* Lookup::bestFunction(const AST::Function::vector& functions, const Types::Type::vector& args, int& cost)
{
    // Quick sanity check
    if (!functions.size())
        return NULL;
    // Find best function using a heuristic
    FunctionHeuristic heuristic(args);
    Function::vector::const_iterator iter = functions.begin(), end = functions.end();
    AST::Function* best_func = *iter++;
    int best = heuristic(best_func);
    while (iter != end)
    {
        AST::Function* func = *iter++;
        int cost = heuristic(func);
        if (cost < best)
        {
            best = cost;
            best_func = func;
        }
    }
    cost = best;
    return best_func;
}

// Private method to lookup a type in the specified search space
Types::Named* Lookup::lookup(const std::string& name, const ScopeSearch& search, bool func_okay) throw ()
{
    STrace trace("Lookup::lookup(name,search,func_okay)");
    ScopeSearch::const_iterator s_iter = search.begin();
    Named::vector results;
    while (s_iter != search.end())
    {
        ScopeInfo* scope = *s_iter++;

        // Check if dict has it
        if (scope->dict->has_key(name))
        {
            if (results.empty())
                results = scope->dict->lookupMultiple(name);
            else
            {
                Named::vector temp_result = scope->dict->lookupMultiple(name);
                std::copy(temp_result.begin(), temp_result.end(),
                          std::back_inserter(results));
            }
        }
        // If not a dummy scope, resolve the set
        if (scope->is_using == false && !results.empty())
        {
#ifdef DEBUG
            Named::vector save_results = results;
#endif
            // Remove the unknowns
            Types::Unknown* unknown = NULL;
            Named::vector::iterator r_iter = results.begin();
            while (r_iter != results.end())
                if ((unknown = dynamic_cast<Types::Unknown*>(*r_iter)) != NULL)
                    r_iter = results.erase(r_iter);
                else if (!func_okay && !isType(*r_iter))
                    r_iter = results.erase(r_iter);
                else
                    ++r_iter;
            // Should be either 1 non-unknowns left or nothing but with
            // 'unknown' set
            if (results.size() == 0 && unknown != NULL)
                return unknown;
            if (results.size() == 0)
                // This means there was only functions in the list, which we are
                // ignoring
                continue;
            if (results.size() == 1)
                // Exactly one match! return it
                return results[0];
            // Store in class var?
            LOG("Multiple candidates!");
#ifdef DEBUG

            for (r_iter = save_results.begin(); r_iter != save_results.end(); ++r_iter)
                LOG(" - '" << (*r_iter)->name() << "' - " << typeid(**r_iter).name());
#endif

            return results[0];
        }
    }
    return NULL;
}

class InheritanceAdder
{
    std::list<AST::Class*>& open_list;
public:
    InheritanceAdder(std::list<AST::Class*>& l) : open_list(l)
    {}
    InheritanceAdder(const InheritanceAdder& i) : open_list(i.open_list)
    {}
    void operator() (AST::Inheritance* i)
    {
        try
        {
            AST::Class* parent = Types::declared_cast<AST::Class>(i->parent());
            open_list.push_back(parent);
        }
        catch (const Types::wrong_type_cast&)
        {
            // ?? ignore for now
        }
    }
};

//. Private Qualified type lookup
Types::Named* Lookup::lookupQual(const std::string& name, const ScopeInfo* scope, bool func_okay)
{
    STrace trace("Lookup::lookupQual");
    //LOG("name: " << name << " in: " << scope->scope_decl->name());
    // First determine: class or namespace
    if (AST::Class* the_class = dynamic_cast<AST::Class*>(scope->scope_decl))
    {
        // A class: search recursively, in order, through base classes
        // FIXME: read up about overriding, hiding, virtual bases and funcs,
        // etc
        std::list<AST::Class*> open_list;
        open_list.push_back(the_class);
        while (!open_list.empty())
        {
            AST::Class* clas = open_list.front();
            open_list.pop_front();
            ScopeInfo* scope = find_info(clas);
            if (scope->dict->has_key(name))
            {
                try
                {
                    Types::Named* named = scope->dict->lookup(name);
                    if (func_okay || isType(named))
                    {
                        return named;
                    }
                    // Else it's a function and a type was wanted: keep looking
                }
                catch (const Dictionary::MultipleError& e)
                {
                    // FIXME: check for duplicates etc etc
                }
                catch (const Dictionary::KeyError& e)
                {
                    std::cerr << "Warning: Key error when has_key said yes" << std::endl;
                }
            }
            // Add base classes to open list
            std::for_each(clas->parents().begin(), clas->parents().end(),
                          InheritanceAdder(open_list));
        }
    }
    else if (dynamic_cast<AST::Namespace*>(scope->scope_decl))
    {
        // A namespace: search recursively through using declarations
        // constructing a conflict set - dont traverse using declarations of
        // any namespace which has an entry for 'name'. Each NS only once
        std::list<const ScopeInfo*> open, closed;
        open.push_back(scope);
        std::vector<Types::Named*> results;
        while (!open.empty())
        {
            const ScopeInfo* ns = open.front();
            open.pop_front();
            // Check if 'ns' is on closed list
            if (std::find(closed.begin(), closed.end(), ns) != closed.end())
                continue;
            // Add to closed list
            closed.push_back(ns);
            // Check if 'ns' has 'name'
            if (ns->dict->has_key(name))
            {
                // Add all results to results list
                if (results.empty())
                    results = ns->dict->lookupMultiple(name);
                else
                {
                    std::vector<Types::Named*> temp = ns->dict->lookupMultiple(name);
                    std::copy(temp.begin(), temp.end(),
                              back_inserter(results));
                }
            }
            else
            {
                // Add 'using' Scopes to open list
                std::copy(ns->using_scopes.begin(), ns->using_scopes.end(),
                          back_inserter(open));
            }
        }
        // Now we have a set of results
        if (!results.size())
        {
            LOG("No results! Looking up '" << name << "'");
            return NULL;
        }
        // FIXME: figure out what to do about multiple
        Types::Named* best = NULL;
        int best_score = -1;
        for (std::vector<Types::Named*>::iterator iter = results.begin();
                iter != results.end(); iter++)
        {
            // Fixme.. create a Score visitor
            int score = 0;
            Types::Named* type = *iter;
            if (Types::Declared* declared = dynamic_cast<Types::Declared*>(type))
            {
                score++;
                if (AST::Declaration* decl = declared->declaration())
                {
                    score++;
                    if (dynamic_cast<AST::Forward*>(decl))
                        score--;
                }
            }
            if (score > best_score)
            {
                best_score = score;
                best = type;
            }
        }

        return best;
    }
    // Not class or NS - which is illegal for a qualified (but coulda been
    // template etc?:)
    LOG("Not class or namespace: " << typeid(scope->scope_decl).name());
    return NULL;
}

//. Public Qualified Type Lookup
Types::Named* Lookup::lookupType(const std::vector<std::string>& names, bool func_okay, AST::Scope* start_scope)
{
    STrace trace("Lookup::lookupType(vector names,search,func_okay)");
    //LOG("looking up '" << names << "' in " << (start_scope?start_scope->name() : m_scope->name()));
    Types::Named* type = NULL;
    ScopeInfo* scope = NULL;
    std::vector<std::string>::const_iterator n_iter = names.begin();
    // Setup the initial scope
    std::string name = *n_iter;
    if (!name.size())
    {
        // Qualified name starts with :: so always start at global scope
        type = global()->declared();
    }
    else
    {
        // Lookup first name as usual
        if (start_scope)
            type = lookupType(name, start_scope);
        else
            type = lookupType(name);
    }
    ++n_iter;

    // Look over every identifier in the qualified name
    while (n_iter != names.end())
    {
        name = *n_iter++;
        try
        {
            // FIXME: this should use some sort of visitor
            AST::Declaration* decl = Types::declared_cast<AST::Declaration>(type);
            if (AST::Typedef* tdef = dynamic_cast<AST::Typedef*>(decl))
            {
                type = Types::type_cast<Types::Named>(tdef->alias());
            }
            // Find cached scope from 'type'
            scope = find_info( Types::declared_cast<AST::Scope>(type) );
        }
        catch (const Types::wrong_type_cast& )
        {
            // Abort lookup
            throw ERROR("qualified lookup found a type (" << type->name() << ") that wasn't a scope finding: " << names);
        }
        // Find the named type in the current scope
        type = lookupQual(name, scope, func_okay && n_iter == names.end());
        if (!type)
            // Abort lookup
            break;
    }

    if (!type)
    {
        LOG("Not found -> creating Unknown");
        // Not found! Add Type.Unknown of scoped name
        std::string name = names[0];
        for (n_iter = names.begin(); ++n_iter != names.end();)
            name += "::" + *n_iter;
        return m_builder->create_unknown(name);
    }
    return type;
}

//. Maps a scoped name into a vector of scopes and the final type. Returns
//. true on success.
bool Lookup::mapName(const ScopedName& names, std::vector<AST::Scope*>& o_scopes, Types::Named*& o_type)
{
    STrace trace("Lookup::mapName");
    AST::Scope* ast_scope = global();
    ScopedName::const_iterator iter = names.begin();
    ScopedName::const_iterator last = names.end();
    last--;
    ScopedName scoped_name;

    // Start scope name at global level
    scoped_name.push_back("");

    // Sanity check
    if (iter == names.end())
        return false;

    // Loop through all containing scopes
    while (iter != last)
    {
        //const std::string& name = *iter++;
        scoped_name.push_back(*iter++);
        Types::Named* type = lookupType(scoped_name);
        if (!type)
        {
            LOG("Warning: failed to lookup " << scoped_name << " in global scope");
            return false;
        }
        try
        {
            ast_scope = Types::declared_cast<AST::Scope>(type);
        }
        catch (const Types::wrong_type_cast&)
        {
            LOG("Warning: looked up scope wasnt a scope!" << scoped_name);
            return false;
        }
        o_scopes.push_back(ast_scope);
    }

    // iter now == last, which can be any type
    scoped_name.push_back(*iter);
    Types::Named* type = lookupType(scoped_name, true);
    if (!type)
    {
        //find_info(ast_scope)->dict->dump();
        LOG("\nWarning: final type lookup wasn't found!" << *iter);
        return false;
    }

    o_type = type;
    return true;
}

Types::Type* Lookup::arrayOperator(Types::Type* object, Types::Type* arg, AST::Function*& func_oper)
{
    STrace trace("Lookup::arrayOperator");
    func_oper = NULL;
    // First decide if should use derefence or methods
    TypeInfo info(object);
    if (info.deref)
    {
        // object is of pointer type, so just deref it
        // Check for typedef
        try
        {
            object = Types::declared_cast<AST::Typedef>(object)->alias();
        }
        catch (const Types::wrong_type_cast&)
        { /* ignore -- not a typedef */
        }
        // Check for modifier
        if (Types::Modifier* mod = dynamic_cast<Types::Modifier*>(object))
        {
            typedef Types::Type::Mods Mods;
            Types::Modifier* newmod = new Types::Modifier(mod->alias(), mod->pre(), mod->post());
            for (Mods::iterator iter = newmod->post().begin(); iter != newmod->post().end(); iter++)
            {
                if (*iter == "*" || *iter == "[]")
                {
                    newmod->post().erase(iter);
                    return newmod;
                }
            }
            //delete newmod;
            throw ERROR("Unable to dereference type for array operator!");
        }
        throw ERROR("Unknown type for array operator!");
    }

    // Hmm object type -- try for [] method
    AST::Class* clas;
    try
    {
        clas = Types::declared_cast<AST::Class>(info.type);
    }
    catch (const Types::wrong_type_cast&)
    {
        TypeFormatter tf;
        throw ERROR("Not deref and not class type for array operator! " << tf.format(info.type) << " <-- " << tf.format(object));
    }

    Function::vector functions;
    try
    {
        findFunctions("[]", find_info(clas), functions);
    }
    catch (const Dictionary::KeyError&)
    {
        throw ERROR("No array operator for class " << clas->name());
    }

    // Make args list
    std::vector<Types::Type*> args;
    args.push_back(arg);

    // Find best function
    int cost;
    AST::Function* func = bestFunction(functions, args, cost);
    if (!func || cost >= 1000)
        throw ERROR("No best function found for array operator.");
    func_oper = func;
    return func->return_type();
}

Types::Named* Lookup::resolveType(Types::Named* type)
{
    STrace trace("Lookup::resolveType(named)");
    try
    {
        ScopedName& name = type->name();
        LOG("Resolving '" << name << "'");

        ScopedName::iterator iter = name.begin(), end = name.end() - 1;
        AST::Scope* scope = global();
        while (iter != end)
        {
            // Find *iter in scope
            Types::Named* scope_type = find_info(scope)->dict->lookup(*iter++);
            scope = Types::declared_cast<AST::Scope>(scope_type);
        }
        LOG("Looking up '"<<(*iter)<<"' in '"<< ((scope==global())?"global":scope->name().back()) << "'");
        // Scope is now the containing scope of the type we are checking
        return find_info(scope)->dict->lookup(*iter);
    }
    catch (const Types::wrong_type_cast& )
    {
        LOG("resolveType failed! bad cast.");
    }
    catch (Dictionary::KeyError e)
    {
        LOG("resolveType failed! key error: '"<<e.name<<"'");
    }
    catch (Dictionary::MultipleError e)
    {
        LOG("resolveType failed! multiple:" << e.types.size());
        std::vector<Types::Named*>::iterator iter = e.types.begin();
        while (iter != e.types.end())
        {
            LOG(" +" << (*iter)->name());
            iter++;
        }
    }
    catch (...)
    {
        // There shouldn't be any errors, but just in case...
        throw ERROR("resolveType failed with unknown error!");
    }
    return type;
}

std::string Lookup::dumpSearch(ScopeInfo* scope)
{
    ScopeSearch& search = scope->search;
    std::ostringstream buf;
    buf << "Search for ";
    if (scope->scope_decl->name().size() == 0)
        buf << "global";
    else
        buf << this->scope()->name();
    buf << " is now: ";
    ScopeSearch::iterator iter = search.begin();
    while (iter != search.end())
    {
        buf << (iter==search.begin() ? "" : ", ");
        const ScopedName& name = (*iter)->scope_decl->name();
        if (name.size())
            if ( (*iter)->is_using )
                buf << "(" << name << ")";
            else
                buf << name;
        else
            buf << "global";
        ++iter;
    }
    //buf << std::ends;
    return buf.str();
}


// vim: set ts=8 sts=4 sw=4 et:
