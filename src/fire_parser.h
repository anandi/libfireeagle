/**
 * FireEagle OAuth+API C++ bindings
 *
 * Copyright (C) 2009 Yahoo! Inc
 *
 */
#include <string>
#include <map>
#include <vector>
#include <list>
#include <stack>

using namespace std;

class FEXMLNode {
  private:
    string _element;
    map<string,string> _attribute;
    FEXMLNode **_child;
    int children;
    int _child_size;
    string _text;
    static string empty_value;

  public:
    FEXMLNode(const string &name);
    ~FEXMLNode();

    const string &element() const;

    const string &text() const;

    void append_text(const char *fragment);

    void add_attribute(const char *name, const char *value);

    unsigned int attribute_count() const;
    list<string> attributes() const;

    const string &attribute(const string &name) const;

    FEXMLNode &add_child(const char *element);

    unsigned int child_count() const;
    const FEXMLNode &child(unsigned int i) const;

    //Debug
    void print(int indent = 0) const;
};

class FEXMLParser {
  private:
    FEXMLNode *root;
    stack<FEXMLNode *> _stack;

  public:
    FEXMLParser();
    ~FEXMLParser();

    FEXMLNode *parse(const string &document);

    void push(FEXMLNode &node);

    void pop();

    FEXMLNode *top() const;

    void set_root(FEXMLNode *_rp);
};

