/**
 * FireEagle OAuth+API C++ bindings
 *
 * Copyright (C) 2009 Yahoo! Inc
 *
 */
#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <list>
#include <stack>

#include "expat_parser.h"

#include <string.h>
#include <assert.h>
#include <expat.h>
#include <stdlib.h>

using namespace std;

string FE_XMLNode::empty_value;

FE_XMLNode::FE_XMLNode(const string &name) : _element(name) {
    children = 0;
    _child = (FE_XMLNode **) malloc(sizeof(FE_XMLNode *) * 5);
    _child_size = 5;
}

FE_XMLNode::~FE_XMLNode() {
    for (int i = 0 ; i < children ; i++) {
        delete(_child[i]);
    }
    free(_child);
}

const string &FE_XMLNode::text() const { return _text; }

void FE_XMLNode::append_text(const char *fragment) {
    if (children > 0)
        return;
    if (fragment)
        _text.append(fragment);
}

void FE_XMLNode::add_attribute(const char *name, const char *value) {
    if (name && value) {
        string _name(name);
        string _value(value);
        _attribute[name] = value;
    }
}

unsigned int FE_XMLNode::attribute_count() const { return _attribute.size(); }
list<string> FE_XMLNode::attributes() const {
    list<string> names;

    for (map<string,string>::const_iterator iter = _attribute.begin();
         iter != _attribute.end() ; iter++) {
        names.push_back(iter->second);
    }

    return names;
}

FE_XMLNode &FE_XMLNode::add_child(const char *element) {
    assert(element);
    FE_XMLNode *newNode = new FE_XMLNode(element);
    if (_child_size == children) {
        _child = (FE_XMLNode **) realloc(_child, sizeof(FE_XMLNode *) * (_child_size + 5));
        _child_size += 5;
    }
    _child[children] = newNode;
    children++;
    if (children == 1)
        _text = "";
    return *newNode;
}

const string &FE_XMLNode::name() const { return _element; }

list<const FE_ParsedNode *> FE_XMLNode::get_children(const string &name) const {
    list<const FE_ParsedNode *> child_list;

    for (int i = 0 ; i < children ; i++) {
        if (_child[i]->name() == name)
            child_list.push_back(_child[i]);
    }

    return child_list;
}

bool FE_XMLNode::has_property(const string &name) const {
    if (name == "text()")
        return (_text.length() > 0);

    return (_attribute.find(name) != _attribute.end());
}

const string &FE_XMLNode::get_string_property(const string &name) const {
    map<string,string>::const_iterator iter = _attribute.find(name);
    if (iter != _attribute.end())
        return iter->second;
    return FE_XMLNode::empty_value;
}

long FE_XMLNode::get_long_property(const string &name, bool *error) const {
    if (error)
        *error = false;
    map<string,string>::const_iterator iter = _attribute.find(name);
    if (iter == _attribute.end()) {
        if (error)
            *error = true;
        return 0;
    }

    char *e;
    const char *c = iter->second.c_str();
    long val = strtol(c, &e, 0);

    if ((*e != 0) && error)
        *error = true;

    return val;
}

double FE_XMLNode::get_double_property(const string &name, bool *error) const {
    if (error)
        *error = false;
    map<string,string>::const_iterator iter = _attribute.find(name);
    if (iter == _attribute.end()) {
        if (error)
            *error = true;
        return 0;
    }

    char *e;
    const char *c = iter->second.c_str();
    double val = strtod(c, &e);

    if ((*e != 0) && error)
        *error = true;

    return val;
}

bool FE_XMLNode::get_bool_property(const string &name, bool *error) const {
    if (error)
        *error = false;
    map<string,string>::const_iterator iter = _attribute.find(name);
    if (iter == _attribute.end()) {
        if (error)
            *error = true;
        return false;
    }

    if (iter->second == "true")
        return true;
    if ((iter->second != "false") && error)
        *error = true;
    return false;
}

unsigned int FE_XMLNode::child_count() const { return children; }
const FE_ParsedNode &FE_XMLNode::child(unsigned int i) const {
    if (i < children)
        return *(_child[i]);

    assert(!"Index past the maximum children count");
}

//Debug
void FE_XMLNode::print(int indent) const {
    for (int i = 0 ; i < indent ; i++)
        printf("    ");
    printf("Element: %s (Children = %d)\n", name().c_str(), children);
    for (map<string,string>::const_iterator iter = _attribute.begin() ;
         iter != _attribute.end() ; iter++) {
        for (int i = 0 ; i < indent ; i++)
            printf("    ");
        printf("@%s=%s\n", iter->first.c_str(), iter->second.c_str());
    }
    if (!children) {
        for (int i = 0 ; i < indent ; i++)
            printf("    ");
        printf("Text: %s\n", text().c_str());
    }
    for (int j = 0 ; j < children ; j++) {
        _child[j]->print(indent + 1);
    }
    for (int i = 0 ; i < indent ; i++)
        printf("    ");
    printf("End: %s\n", name().c_str());
}

extern "C" void FE_XML_begin_element(void *context, const char *elem,
                                     const char **attrs) {
    FE_XMLParser *parser = (FE_XMLParser *)context;

    assert(elem);
    FE_XMLNode *top = parser->top();
    if (top) {
        FE_XMLNode &tmp = top->add_child(elem);
        parser->push(tmp);
    } else {
        FE_XMLNode *root = new FE_XMLNode(elem);
        parser->set_root(root);
    }

    FE_XMLNode *newNode = parser->top();
    for (int i = 0 ; attrs[i] ; i += 2) {
        newNode->add_attribute(attrs[i], attrs[i + 1]);
    }
}

extern "C" void FE_XML_end_element(void *context, const char *elem) {
    FE_XMLParser *parser = (FE_XMLParser *)context;

    FE_XMLNode *top = parser->top();
    if (top)
        parser->pop();
}

extern "C" void FE_XML_handle_text(void *context, const XML_Char *s, int len) {
    FE_XMLParser *parser = (FE_XMLParser *)context;
    assert(s && (len > 0));

    FE_XMLNode *top = parser->top();
    assert(top);

    char *tmps = strndup(s, len); //Since s is not null terminated.
    top->append_text(tmps);
}

FE_XMLParser::FE_XMLParser() : root(NULL) {};
FE_XMLParser::~FE_XMLParser() { /*don't delete root!!*/ };

FE_ParsedNode *FE_XMLParser::parse(const string &document) {
    XML_Parser p = XML_ParserCreate(NULL);
    assert(p);

    XML_SetElementHandler(p, FE_XML_begin_element, FE_XML_end_element);
    XML_SetCharacterDataHandler(p, FE_XML_handle_text);
    XML_SetUserData(p, (void *)this);

    assert(XML_Parse(p, document.c_str(), document.length(), 1));
    return root;
}

void FE_XMLParser::push(FE_XMLNode &node) { _stack.push(&node); }

void FE_XMLParser::pop() { _stack.pop(); }

FE_XMLNode *FE_XMLParser::top() const { return (root) ? _stack.top() : NULL ; }

void FE_XMLParser::set_root(FE_XMLNode *_rp) {
    assert(!root);
    root = _rp;
    _stack.push(root);
}

FE_ParsedNode *parseXML(const string &xml) {
    FE_XMLParser parser;

    return parser.parse(xml); //Remember to free up!!
}

