/**
 * FireEagle OAuth+API C++ bindings
 *
 * Copyright (C) 2009 Yahoo! Inc
 *
 */
#ifndef FIREEAGLE_PARSER_IFACE_H
#define FIREEAGLE_PARSER_IFACE_H

#include <string>
#include <list>

using namespace std;

/**
 * This class is an abstraction. We need to fit this to the proper parser and
 * response format (XML or JSON). A note about the property 'text()' is in order...
 * Because of the fact that XML elements can have both text as well as attributes,
 * which JSON cannot, we have decided that the text of an XML element will be
 * accessed as a special attribute (property) called 'text()'.
 */
class FE_ParsedNode {
  public:
    /** Retrieves the name of this node */
    virtual const string &name() const = 0;

    /**
     * Return a list of child node of the given name. Caller should ideally
     * not free the pointers to allow for sloppy implementation.
     * @param name Name of the sub nodes to match.
     * @return The list of pointers to child objects found, or empty list.
     */
    virtual list<const FE_ParsedNode *> get_children(const string &name) const = 0;

    /** Get the list of all child nodes
     * @return The count of child nodes
     */
    virtual unsigned int child_count() const = 0;

    /** Get the child nodes in some enumeration order.
     * @param i Index.
     * @return const reference to the child.
     */
    virtual const FE_ParsedNode &child(unsigned int i) const = 0;

    /**
     * Checks if an attribute (property) of the current object exists by the
     * given name. If the name matches a sub node, the return is false.
     * @param name Name of the property being searched.
     * @return true if found, false otherwise.
     */
    virtual bool has_property(const string &name) const = 0;

    /**
     * Get an attribute (property) of the current object as a string. Callers
     * should use has_property to ensure that the property exists and invalid
     * value is not returned.
     * @param name Name of the property being searched.
     * @return Valid value or empty string. Does not throw exception when the
     * name matches a sub object.
     */
    virtual const string &get_string_property(const string &name) const = 0;

    /**
     * Get an attribute (property) of the current object as a long. Callers
     * should use has_property to ensure that the property exists and invalid
     * value is not returned.
     * @param name Name of the property being searched.
     * @param error Out argument pointer to indicate parse error in converting
     * property value from string to long.
     * @return If the correctly parsed value is found, return valid value,
     * else return garbage.
     */
    virtual long get_long_property(const string &name, bool *error = NULL) const = 0;

    /**
     * Get an attribute (property) of the current object as a double. Callers
     * should use has_property to ensure that the property exists and invalid
     * value is not returned.
     * @param name Name of the property being searched.
     * @param error Out argument pointer to indicate parse error in converting
     * property value from string to double.
     * @return If the correctly parsed value is found, return valid value,
     * else return garbage.
     */
    virtual double get_double_property(const string &name, bool *error = NULL) const = 0;

    /**
     * Get an attribute (property) of the current object as a boolean. Callers
     * should use has_property to ensure that the property exists and invalid
     * value is not returned.
     * @param name Name of the property being searched.
     * @param error Out argument pointer to indicate parse error in converting
     * property value from string to boolean.
     * @return If the correctly parsed value is found, return valid value,
     * else return garbage.
     */
    virtual bool get_bool_property(const string &name, bool *error = NULL) const = 0;

    /** The destructor, ofcourse, has to be virtual */
    virtual ~FE_ParsedNode() {}
};

/** An interface class to the actual parser implementation. */
class FE_Parser {
  public:
    /** The destructor is virtual */
    virtual ~FE_Parser() {}

    /**
     * Pass the string to be parsed and get back a parsed node tree.
     * @param data String to be parsed.
     * @return Top pointer to FE_ParsedNode. This should be deleted by the
     * destructor of the derived class of this class. If parsing fails, return NULL.
     */
    virtual FE_ParsedNode *parse(const string &data) = 0;
};

#endif /* FIREEAGLE_PARSER_IFACE_H */
