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

#include "fire_parser.h"

#include <string.h>
#include <assert.h>
#include <expat.h>
#include <stdlib.h>

using namespace std;

string FEXMLNode::empty_value;

FEXMLNode::FEXMLNode(const string &name) : _element(name) {
    children = 0;
    _child = (FEXMLNode **) malloc(sizeof(FEXMLNode *) * 5);
    _child_size = 5;
}

FEXMLNode::~FEXMLNode() {
    for (int i = 0 ; i < children ; i++) {
        delete(_child[i]);
    }
    free(_child);
}

const string &FEXMLNode::element() const { return _element; }

const string &FEXMLNode::text() const { return _text; }

void FEXMLNode::append_text(const char *fragment) {
    if (children > 0)
        return;
    if (fragment)
        _text.append(fragment);
}

void FEXMLNode::add_attribute(const char *name, const char *value) {
    if (name && value) {
        string _name(name);
        string _value(value);
        _attribute[name] = value;
    }
}

unsigned int FEXMLNode::attribute_count() const { return _attribute.size(); }
list<string> FEXMLNode::attributes() const {
    list<string> names;

    for (map<string,string>::const_iterator iter = _attribute.begin();
         iter != _attribute.end() ; iter++) {
        names.push_back(iter->second);
    }

    return names;
}

const string &FEXMLNode::attribute(const string &name) const {
    map<string,string>::const_iterator iter = _attribute.find(name);
    if (iter != _attribute.end())
        return iter->second;
    return FEXMLNode::empty_value;
}

FEXMLNode &FEXMLNode::add_child(const char *element) {
    assert(element);
    FEXMLNode *newNode = new FEXMLNode(element);
    if (_child_size == children) {
        _child = (FEXMLNode **) realloc(_child, sizeof(FEXMLNode *) * (_child_size + 5));
        _child_size += 5;
    }
    _child[children] = newNode;
    children++;
    if (children == 1)
        _text = "";
    return *newNode;
}

unsigned int FEXMLNode::child_count() const { return children; }
const FEXMLNode &FEXMLNode::child(unsigned int i) const {
    if (i < children)
        return *(_child[i]);

    assert(!"Index past the maximum children count");
}

//Debug
void FEXMLNode::print(int indent) const {
    for (int i = 0 ; i < indent ; i++)
        printf("    ");
    printf("Element: %s (Children = %d)\n", element().c_str(), children);
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
    printf("End: %s\n", element().c_str());
}

extern "C" void FE_XML_begin_element(void *context, const char *elem,
                                     const char **attrs) {
    FEXMLParser *parser = (FEXMLParser *)context;

    assert(elem);
    FEXMLNode *top = parser->top();
    if (top) {
        FEXMLNode &tmp = top->add_child(elem);
        parser->push(tmp);
    } else {
        FEXMLNode *root = new FEXMLNode(elem);
        parser->set_root(root);
    }

    FEXMLNode *newNode = parser->top();
    for (int i = 0 ; attrs[i] ; i += 2) {
        newNode->add_attribute(attrs[i], attrs[i + 1]);
    }
}

extern "C" void FE_XML_end_element(void *context, const char *elem) {
    FEXMLParser *parser = (FEXMLParser *)context;

    FEXMLNode *top = parser->top();
    if (top)
        parser->pop();
}

extern "C" void FE_XML_handle_text(void *context, const XML_Char *s, int len) {
    FEXMLParser *parser = (FEXMLParser *)context;
    assert(s && (len > 0));

    FEXMLNode *top = parser->top();
    assert(top);

    char *tmps = strndup(s, len); //Since s is not null terminated.
    top->append_text(tmps);
}

FEXMLParser::FEXMLParser() : root(NULL) {};
FEXMLParser::~FEXMLParser() { /*don't delete root!!*/ };

FEXMLNode *FEXMLParser::parse(const string &document) {
    XML_Parser p = XML_ParserCreate(NULL);
    assert(p);

    XML_SetElementHandler(p, FE_XML_begin_element, FE_XML_end_element);
    XML_SetCharacterDataHandler(p, FE_XML_handle_text);
    XML_SetUserData(p, (void *)this);

    assert(XML_Parse(p, document.c_str(), document.length(), 1));
    return root;
}

void FEXMLParser::push(FEXMLNode &node) { _stack.push(&node); }

void FEXMLParser::pop() { _stack.pop(); }

FEXMLNode *FEXMLParser::top() const { return (root) ? _stack.top() : NULL ; }

void FEXMLParser::set_root(FEXMLNode *_rp) {
    assert(!root);
    root = _rp;
    _stack.push(root);
}

FEXMLNode *parseXML(const string &xml) {
    FEXMLParser parser;

    return parser.parse(xml); //Remember to free up!!
}

/*-------------------------Test driver-------------------------------
#include <stdio.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Give a file name.\n");
        return 0;
    }

    FILE *fp = fopen(argv[1], "r");
    if (!fp) {
        printf("Invalid file name: %s\n", argv[1]);
        return 0;
    }

    string document;
    char buf[1024];
    while (!feof(fp)) {
        if (!fgets(buf, 1024, fp))
            break;
        document.append(buf);
    }
    fclose(fp);

    FEXMLParser parser;
    FEXMLNode *root = parser.parse(document);
//    root->print(cout);
    root->print1();

    cout << "Parse successful." << endl;
    return 0;
}
*/
