/**
 * FireEagle OAuth+API C++ bindings
 *
 * Copyright (C) 2009 Yahoo! Inc
 *
 */
#ifndef EXPAT_PARSER_H
#define EXPAT_PARSER_H

#include <string>
#include <map>
#include <vector>
#include <list>
#include <stack>

#include "parser_iface.h"

using namespace std;

class FE_XMLNode : public FE_ParsedNode {
  private:
    string _element;
    map<string,string> _attribute;
    FE_XMLNode **_child;
    int children;
    int _child_size;
    string _text;
    static string empty_value;

  public:
    FE_XMLNode(const string &name);
    ~FE_XMLNode();

    const string &text() const;

    void append_text(const char *fragment);

    void add_attribute(const char *name, const char *value);

    unsigned int attribute_count() const;
    list<string> attributes() const;

    FE_XMLNode &add_child(const char *element);

    //Debug
    void print(int indent = 0) const;

    virtual unsigned int child_count() const;

    virtual const FE_ParsedNode &child(unsigned int i) const;

    virtual const string &name() const;

    virtual list<const FE_ParsedNode *> get_children(const string &name) const;

    virtual bool has_property(const string &name) const;

    virtual const string &get_string_property(const string &name) const;

    virtual long get_long_property(const string &name, bool *error = NULL) const;

    virtual double get_double_property(const string &name, bool *error = NULL) const;

    virtual bool get_bool_property(const string &name, bool *error = NULL) const;
};

class FE_XMLParser : public FE_Parser {
  private:
    FE_XMLNode *root;
    stack<FE_XMLNode *> _stack;

  public:
    FE_XMLParser();
    ~FE_XMLParser();

    FE_ParsedNode *parse(const string &document);

    void push(FE_XMLNode &node);

    void pop();

    FE_XMLNode *top() const;

    void set_root(FE_XMLNode *_rp);
};

#endif /* EXPAT_PARSER_H */

