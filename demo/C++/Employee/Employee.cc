/*
 * This file tests Java-style and Javadoc comments.
 * $id$
 * $changelog$
 */

#include <iostream.h>
#include <string>

/**
 * Representation of an Employee. This class follows the (in)famous example of
 * being an Employee, used in many C++ courses. Employees have names and IDs.
 * If this were a bigger example they might have more.
 * @see Employee.Employee
 * @see Employee.Employee()
 * @see Employee.Employee(string,string)
 * @see Employee.name()
 * @see Employee.id()
 * @see name()
 * @see id()
 * @see string
 * @link http://www.c++.org/ Your C++ Manual
 */
class Employee {
public:
    /** Constructor. id may be empty, in which case a new ID is generated. */
    Employee(string name, string id);

    /** Returns the name of this Employee */
    string name();

    /** Returns the id of this Employee */
    string id();
private:
    /** The name */
    string name;
    /** The ID. This is a sequence of alphanumeric digits of length 10 and
     * starting with 'e'. */
    string id;
};
