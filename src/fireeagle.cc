/**
 * FireEagle OAuth+API C++ bindings
 *
 * Copyright (C) 2009 Yahoo! Inc
 *
 */
#include <string>
#include <exception>
#include <map>
#include <sstream>
#include <iostream>

#include <string.h>
#include <stdlib.h> //for malloc.

extern "C" {
#include "oauth.h"
}

#include "fireeagle.h"
#include "fire_objects.h"
//#include "fire_parser.h"

using namespace std;

FireEagleException::FireEagleException(const string &_msg, int _code, const string &_response)
    : msg(_msg), code(_code), response(_response) {}

string FireEagleException::to_string() const {
    ostringstream os;

    os << "FireEagleException (code: " << code << "): " << msg << endl;
    if (response.length())
        os << "Response is: " << response << endl;

    return os.str();
}

FireEagleException::~FireEagleException() throw() {}

OAuthTokenPair::OAuthTokenPair(const string &_token, const string &_secret)
    : token(_token), secret(_secret) {}

OAuthTokenPair::OAuthTokenPair(const OAuthTokenPair &other) {
    token = other.token;
    secret = other.secret;
}

OAuthTokenPair::OAuthTokenPair(const string &file) {
    FILE *fp; //Someone please patch my code with ifstreams :( ... I am so lazy to
              //break the C mould.
    char buffer[1024];

    fp = fopen(file.c_str(), "r");

    if (!fp) {
        ostringstream os;
        os << "Could not open token file (" << file << ")";
        throw new FireEagleException(os.str(), FE_INTERNAL_ERROR);
    }
    fgets(buffer, 1023, fp);
    fclose(fp);

    char *pos = &(buffer[strlen(buffer) - 1]); //Last char may be '\n'
    if (*pos == '\n')
        *pos = 0;
    try {
        init_from_string(&(buffer[0]));
    } catch (FireEagleException *fe) {
        delete fe;
        ostringstream os;
        os << "Token file: " << file << " . Invalid format.";
        throw new FireEagleException(os.str(), FE_INTERNAL_ERROR);
    }
}

void OAuthTokenPair::init_from_string(const char *stro) {
    char *str = strdup(stro);
    char *pos = strchr(str, ' '); //Search for the separator whitespace.
    if (!pos) {
        free(str);
        throw new FireEagleException("Invalid format", FE_INTERNAL_ERROR);
    }

    *pos = 0; pos++;
    if (!strlen(str) || !strlen(pos)) {
        free(str);
        throw new FireEagleException("Invalid format", FE_INTERNAL_ERROR);
    }

    token = string(str);
    secret = string(pos);
    free(str);
}

string OAuthTokenPair::to_string() const {
    string data(token);
    data.append(" ");
    data.append(secret);

    return data;
}

void OAuthTokenPair::save(const string &file) const {
    if (!is_valid()) {
        throw new FireEagleException("Cannot save an empty OAuthTokenPair.",
                                     FE_INTERNAL_ERROR);
    }

    FILE *fp = fopen(file.c_str(), "w");
    if (!fp) {
        ostringstream os;
        os << "Could not open token file (" << file << ") for saving.";
        throw new FireEagleException(os.str(), FE_INTERNAL_ERROR);
    }

    string str = to_string();
    fprintf(fp, "%s\n", str.c_str());
    fclose(fp);
}

bool OAuthTokenPair::is_valid() const {
    if (!token.length() || !secret.length())
        return false;

    return true;
}

const FE_ParamPairs empty_params;

void FireEagleConfig::common_init() {
    this->FE_ROOT = "http://fireeagle.yahoo.net";
    this->FE_API_ROOT = "https://fireeagle.yahooapis.com";
    this->FE_DEBUG = false;
    this->FE_DUMP_REQUESTS = false;
    this->FE_OAUTH_VERSION = OAUTH_10;
}

FireEagleConfig::FireEagleConfig(const OAuthTokenPair &_app_token)
    : app_token(_app_token), general_token("","") {
    common_init();
}

FireEagleConfig::FireEagleConfig(const OAuthTokenPair &_app_token,
                                 const OAuthTokenPair &_general_token)
    : app_token(_app_token), general_token(_general_token) {
    common_init();
}

map<string,string> FireEagleConfig::parse_config(const string &conf_file) {
    map<string,string> pairs;

    FILE *fp = fopen(conf_file.c_str(), "r");
    if (!fp) {
        string msg("FireEagleConfig: Failed to open configuration file ");
        msg.append(conf_file);
        msg.append(" for loading");
        throw new FireEagleException(msg, FE_INTERNAL_ERROR);
    }

    char buffer[1024]; //I wonder whether we shall need anything more.
    int line_no = 0;
    while (!feof(fp)) {
        if (!fgets(buffer, 1023, fp))
            break;
        //Read a line.
        line_no++;
        char *c = &(buffer[strlen(buffer) - 1]);
        if (*c == '\n')
            *c = 0;
        else {
            ostringstream os;
            os << "FireEagleConfig: Failure to read complete line from config file ";
            os << conf_file << ":" << line_no;
            fclose(fp);
            throw new FireEagleException(os.str(), FE_INTERNAL_ERROR);
        }
        if (buffer[0] == '#')
            continue;
        c = strchr(buffer, ':');
        if (!c) {
            ostringstream os;
            os << "FireEagleConfig: Invalid config statement '" << buffer << "'";
            os << " in config file " << conf_file << " at line " << line_no;
            throw new FireEagleException(os.str(), FE_INTERNAL_ERROR);
        }

        *c = 0; c++; //Split into name and value
        pairs[string(buffer)] = string(c);
    }

    fclose(fp);

    return pairs;
}

void FireEagleConfig::common_init_with_config(const map<string,string> &config) {
    map<string,string>::const_iterator iter;

    iter = config.find("app_token_file");
    if (iter != config.end()) {
        app_token = OAuthTokenPair(iter->second);
    } else {
        //File for token is not specified.
        iter = config.find("app_token_data");
        if (iter != config.end()) {
            app_token.init_from_string(iter->second.c_str());
        }
    }

    iter = config.find("general_token_file");
    if (iter != config.end()) {
        general_token = OAuthTokenPair(iter->second);
    } else {
        //File for token is not specified.
        iter = config.find("general_token_data");
        if (iter != config.end()) {
            general_token.init_from_string(iter->second.c_str());
        }
    }

    iter = config.find("root_url");
    if (iter != config.end())
        FE_ROOT = iter->second;

    iter = config.find("api_base_url");
    if (iter != config.end())
        FE_API_ROOT = iter->second;
}

FireEagleConfig::FireEagleConfig(const map<string,string> &config)
    : app_token("",""), general_token("","") {
    common_init();
    common_init_with_config(config);

    if (!app_token.is_valid()) {
        string msg("FireEagleConfig: Could not initialize application token from config");
        throw new FireEagleException(msg, FE_INTERNAL_ERROR);
    }
}

FireEagleConfig::FireEagleConfig(const string &conf_file)
    : app_token("",""), general_token("","") {
    common_init();

    map<string,string> pairs = FireEagleConfig::parse_config(conf_file);
    common_init_with_config(pairs);

    if (!app_token.is_valid()) {
        string msg("FireEagleConfig: Could not initialize application token from config file ");
        msg.append(conf_file);
        throw new FireEagleException(msg, FE_INTERNAL_ERROR);
    }
}

FireEagleConfig::~FireEagleConfig() {
    for (map<string,ParserData *>::iterator iter = parsers.begin() ;
         iter != parsers.end() ; iter++) {
        if (iter->second)
            delete iter->second;
    }
}

static void write_config(FILE *fp, const string &name, const string &value) {
    fprintf(fp, "%s:%s\n", name.c_str(), value.c_str());
}

void FireEagleConfig::save(const string &file) const {
    map<string,string> dummy;
    dummy.clear();
    savex(file, dummy);
}

void FireEagleConfig::savex(const string &file,
                            const map<string,string> &extra) const {
    if (!app_token.is_valid()) {
        string msg("FireEagleConfig: Cannot save an invalid config.");
        throw new FireEagleException(msg, FE_INTERNAL_ERROR);
    }

    FILE *fp = fopen(file.c_str(), "w");
    if (!fp) {
        ostringstream os;
        os << "FireEagleConfig: Could not open config file (" << file << ") for saving.";
        throw new FireEagleException(os.str(), FE_INTERNAL_ERROR);
    }

    write_config(fp, "app_token_data", app_token.to_string());
    write_config(fp, "root_url", FE_ROOT);
    write_config(fp, "api_base_url", FE_API_ROOT);
    if (general_token.is_valid())
        write_config(fp, "general_token_data", general_token.to_string());

    map<string,string>::const_iterator iter;
    for (iter = extra.begin() ; iter != extra.end() ; iter++) {
        if ((iter->first == "app_token_data")
            || (iter->first == "root_url")
            || (iter->first == "api_base_url")
            || ((iter->first == "general_token_data")
                && general_token.is_valid()))
            continue;
        write_config(fp, iter->first, iter->second);
    }
    fclose(fp);
}

ParserData *FireEagleConfig::register_parser(const string &content_type,
                                             ParserData *parser) {
    ParserData *old = NULL;

    map<string,ParserData *>::iterator iter = parsers.find(content_type);
    if (iter != parsers.end())
        old = iter->second;

    parsers[content_type] = parser;

    return old;
}

ParserData *FireEagleConfig::get_parser(const string &content_type) {
    map<string,ParserData *>::iterator iter = parsers.find(content_type);

    if (iter != parsers.end())
        return iter->second;

    return NULL;
}

ParserData *FireEagleConfig::get_parser(enum FE_format format) {
    for (map<string,ParserData *>::iterator iter = parsers.begin() ;
         iter != parsers.end() ; iter++) {
        if (iter->second->lang() == format)
            return iter->second;
    }

    return NULL;
}

const OAuthTokenPair *FireEagleConfig::get_consumer_key() const { return &app_token; }

const OAuthTokenPair *FireEagleConfig::get_general_token() const {
    if (general_token.is_valid())
        return &general_token;
    return NULL;
}

static FE_ParsedNode *parseResponse(const string &resp, FE_Parser *parser) {
    FE_ParsedNode *root = parser->parse(resp);
    if (!root) {
        delete parser; //This is a bad way of doing things!!
        throw new FireEagleException("Invalid XML error response from Fire Eagle",
                                     FE_INTERNAL_ERROR, resp);
    }

    return root;
}

bool FE_isJSONErrorMsg(const FE_ParsedNode *root, const string &msg) {
    throw new FireEagleException("Method FE_isJSONErrorMsg is not implemented yet",
                                 FE_INTERNAL_ERROR, msg);
}

bool FE_isXMLErrorMsg(const FE_ParsedNode *root, const string &msg) {
    if (root->name() != "rsp") {
        delete root;
        throw new FireEagleException("Unknown XML response format from Fire Eagle",
                                     FE_INTERNAL_ERROR, msg);
    }

    if (!(root->has_property("stat"))) {
        delete root;
        throw new FireEagleException("Unknown XML response format from Fire Eagle",
                                     FE_INTERNAL_ERROR, msg);
    }

    if (root->get_string_property("stat") == "ok")
        return false;

    list<const FE_ParsedNode *> children = root->get_children("err");
    if (children.size() != 1) {
        delete root;
        throw new FireEagleException("Unknown XML response format from Fire Eagle",
                                     FE_INTERNAL_ERROR, msg);
    }

    return true;
}

FireEagleException *FE_exceptionFromJSON(const FE_ParsedNode *root) {
    throw new FireEagleException("Method FE_exceptionFromJSON is not implemented yet",
                                 FE_INTERNAL_ERROR);
}

FireEagleException *FE_exceptionFromXML(const FE_ParsedNode *root) {
    //Ideally this is not the guy who is being forced to throw any exception
    //It is just a factory method.
    string message("Remote error: ");
    list<const FE_ParsedNode *> children = root->get_children("err");
    const FE_ParsedNode *err = *(children.begin());
    message.append(err->get_string_property("msg"));
    long code = err->get_long_property("code");
    FireEagleException *e = new FireEagleException(message, code);

    return e;
}

FireEagleHTTPAgent *FireEagle::HTTPAgent(const string &url,
                                         const string &postdata) const {
    return new FireEagleCurl(url, postdata);
}

// Make an HTTP request, throwing an exception if we get anything other than a 200 response
string FireEagle::http(const string &url, const string postData) const {
    if (config->FE_DEBUG)
        cerr << "[FE HTTP request: url: " << url << ", post data: " << postData << endl;
    if (config->FE_DUMP_REQUESTS) {
        ostringstream os;
        os << "[FE HTTP request: url: " << url << ", post data: " << postData;
        dump(os.str());
    }

    string response;
    string contentType;
    long contentLength;
    long responseCode;

    try {
        FireEagleHTTPAgent *agent = HTTPAgent(url, postData);
        agent->initialize_agent();
        agent->set_custom_request_opts();
        responseCode = agent->make_call();
        if (!responseCode) {
            responseCode = agent->agent_error();
            ostringstream os;
            os << "Connection to " << url << " failed with agent error "<< responseCode;
            throw new FireEagleException(os.str(), FE_CONNECT_FAILED);
        }

        response = agent->get_response();

        string content_length = agent->get_header("Content-Length");
        contentLength = strtol(content_length.c_str(), NULL, 0);

        string content_type = agent->get_header("Content-Type");
        size_t pos = content_type.find(';');
        contentType = content_type.substr(0, pos);

        delete agent;
    } catch (FireEagleHTTPException *e) {
        FireEagleException *fex = new FireEagleException(e->msg, FE_INTERNAL_ERROR);
        delete e;
        throw fex;
    }

    if (responseCode != 200) {
        ParserData *parser_data = config->get_parser(contentType);
        if (parser_data) {
            FE_Parser *parser = parser_data->parser_instance();
//        if (contentType == "application/xml") { //Si Habla XML!!
            FE_ParsedNode *root = parseResponse(response, parser);
            if ((parser_data->lang() == FE_FORMAT_XML)
                && (FE_isXMLErrorMsg(root, response))) {
                FireEagleException *e = FE_exceptionFromXML(root);
                delete root;
                throw e;
            } else if ((parser_data->lang() == FE_FORMAT_JSON)
                && (FE_isJSONErrorMsg(root, response))) {
                FireEagleException *e = FE_exceptionFromJSON(root);
                delete root;
                throw e;
            } //Don't do an else part. Even if we get a valid response with a non
              //200 HTTP code, proceed.
            delete root;
            delete parser;
        } else {
            ostringstream os;
            os << "Request to " << url << " failed: HTTP error " << responseCode;
            os << " Content Type: " << contentType;
            throw new FireEagleException(os.str(), FE_REQUEST_FAILED, response);
        }
    }

    if (config->FE_DUMP_REQUESTS) {
        ostringstream os;
        os << "HTTP/1.0 " << responseCode << " OK" << endl;
        os << "Content-Type: " << contentType << endl;
        os << "Content-Length: " << contentLength << endl;
        os << endl << response << endl << endl;
        dump(os.str());
    }

    if (config->FE_DEBUG)
        cerr << "HTTP response: " << response << endl;

    return response;
}

// Format and sign an OAuth / API request
string FireEagle::oAuthRequest(const string &url, enum FE_oauth_token token_type,
                               const FE_ParamPairs &args, bool isPost) const {
    string url_with_params(url);

    if (args.empty())
        isPost = false;
    else {
        url_with_params.append("?");
        bool first_time = true;
        for (FE_ParamPairs::const_iterator iter = args.begin() ;
             iter != args.end() ; iter++) {
            if (!first_time)
                url_with_params.append("&");
            else
                first_time = false;
            url_with_params.append(iter->first + "=" + iter->second);
        }
    }

    char *postargs = NULL;
    const OAuthTokenPair *consumer = config->get_consumer_key();

    OAuthTokenPair *token2 = NULL;
    switch (token_type) {
    case FE_TOKEN_GENERAL:
        if (config->get_general_token()) {
            token2 = new OAuthTokenPair(*(config->get_general_token()));
            if (!token2)
                throw new FireEagleException("Out of memory", FE_INTERNAL_ERROR);
        } else {
            ostringstream os;
            os << "Call to " << url << " requires a general token";
            throw new FireEagleException(url, FE_INTERNAL_ERROR);
        }
        break;
    case FE_TOKEN_REQUEST:
    case FE_TOKEN_ACCESS:
        token2 = token;
        if (!token2) {
            ostringstream os;
            os << "Call to " << url << " requires a ";
            if (token_type == FE_TOKEN_ACCESS)
                os << "access token";
            else
                os << "request token";
            throw new FireEagleException(url, FE_INTERNAL_ERROR);
        }
    default:
        break;
    }

    char *result_tmp = oauth_sign_url(url_with_params.c_str(),
                                      (isPost) ? &postargs : NULL, OA_HMAC,
                                      consumer->token.c_str(), consumer->secret.c_str(),
                                      (token2)? token2->token.c_str() : NULL,
                                      (token2)? token2->secret.c_str() : NULL);
    if (token_type == FE_TOKEN_GENERAL)
        delete token2;

    if (config->FE_DUMP_REQUESTS) {
        ostringstream os;
        os << "FireEagle ";
        if (isPost)
            os << "POST";
        else
            os << "GET";
        os << " Request: " << result_tmp;
        dump(os.str());
    }

    if (!result_tmp)
        throw new FireEagleException("OAuth signing failed", FE_INTERNAL_ERROR);
    string result(result_tmp);
    free(result_tmp);

    string response = (isPost) ? http(url, postargs) : http(result);
    if (postargs)
        free(postargs);
    return response;
}

// OAuth URLs
string FireEagle::requestTokenURL() const {
    return string(config->FE_API_ROOT).append("/oauth/request_token");
}

string FireEagle::authorizeURL() const {
    return string(config->FE_ROOT).append("/oauth/authorize");
}

string FireEagle::accessTokenURL() const {
    return string(config->FE_API_ROOT).append("/oauth/access_token");
}

// API URLs
string FireEagle::methodURL(const string &method, enum FE_format format) const {
    string url(config->FE_API_ROOT);
    url.append("/api/0.1/").append(method);
//    url.append(".").append((FE_format_info[format]).extension);
    return url;
}

FireEagle::FireEagle(FireEagleConfig *_config, const OAuthTokenPair &_token) {
    if (!_config)
        throw new FireEagleException("NULL pointer for FireEagleConfig", FE_INTERNAL_ERROR);
    if (!_token.is_valid())
        throw new FireEagleException("Invalid OAuthTokenPair in constructor",
                                     FE_INTERNAL_ERROR);
    config = _config;
    token = new OAuthTokenPair(_token);
    if (!token)
        throw new FireEagleException("Out of memory", FE_INTERNAL_ERROR);
}

FireEagle::FireEagle(FireEagleConfig *_config) {
    if (!_config)
        throw new FireEagleException("NULL pointer for FireEagleConfig", FE_INTERNAL_ERROR);
    config = _config;
    token = NULL;
}

FireEagle::~FireEagle() {
    if (token)
        delete token;
}

FireEagle::FireEagle(const FireEagle &other) {
    config = other.config;
    if (other.token) {
        token = new OAuthTokenPair(*(other.token));
        if (!token)
            throw new FireEagleException("Out of memory", FE_INTERNAL_ERROR);
    } else
        token = NULL;
}

FireEagle &FireEagle::operator=(const FireEagle &other) {
    config = other.config;
    if (token)
        delete token;

    if (other.token) {
        token = new OAuthTokenPair(*(other.token));
        if (!token)
            throw new FireEagleException("Out of memory", FE_INTERNAL_ERROR);
    } else
        token = NULL;
}

void FireEagle::dump(const string &str) const {
    cout << str << endl;
}

// Parse a URL-encoded OAuth response
OAuthTokenPair FireEagle::oAuthParseResponse(const string &response) const {
    size_t begin = (size_t)0;
    OAuthTokenPair oauth("", "");

    bool done = false;
    while (!done) {
        size_t end = response.find('&', begin);
        if (end == string::npos) {
            end = response.length();
            done = true; //Do not loop more.
        }
        string pair = response.substr(begin, end - begin);
        begin = end + 1;

        size_t mid = pair.find('=');
        if (mid == string::npos)
            continue;

        string name = pair.substr(0, mid);
        string value = pair.substr(mid + 1);

        if (name == "oauth_token") {
            oauth.token = value;
        } else if (name == "oauth_token_secret") {
            oauth.secret = value;
        }
    }

    return oauth;
}

OAuthTokenPair FireEagle::getRequestToken(string oauth_callback) {
    FE_ParamPairs args;

    if (config->FE_OAUTH_VERSION == OAUTH_10A) {
        args["oauth_callback"] = oauth_callback;
    }

    string response = oAuthRequest(requestTokenURL(), FE_TOKEN_NONE, args);
    OAuthTokenPair oauth = oAuthParseResponse(response);

    if (oauth.token.length() && oauth.secret.length()) {
        if (token)
            delete token;
        token = new OAuthTokenPair(oauth);
        if (!token)
            throw new FireEagleException("Out of memory", FE_INTERNAL_ERROR);
    }

    if (config->FE_DUMP_REQUESTS) {
        ostringstream os;
        os << "Now the user is redirected to " << getAuthorizeURL(oauth) << endl;
        os << "Once the user returns, via the callback URL for web authentication or manually for desktop authentication, we can get their access token and secret by calling /oauth/access_token." << endl;
        dump(os.str());
    }

    return oauth;
}

OAuthTokenPair FireEagle::request_token(string oauth_callback) {
    return getRequestToken(oauth_callback);
}

string FireEagle::getAuthorizeURL(const OAuthTokenPair &oauth,
                                  const string &callback) const {
    string url(authorizeURL() + "?oauth_token=" + oauth.token);
    if ((callback.length() > 0) && (config->FE_OAUTH_VERSION == OAUTH_10)) {
        char *tmp = oauth_url_escape(callback.c_str());
        url.append("&oauth_callback=").append(tmp);
        free(tmp);
    }

    return url;
}

string FireEagle::authorize(const OAuthTokenPair &oauth,
                            const string &callback) const {
    return getAuthorizeURL(oauth, callback);
}

void FireEagle::requireToken() const {
    if (!token || !token->token.length() || !token->secret.length())
        throw new FireEagleException("This function requires an OAuth token",
                                     FE_TOKEN_REQUIRED);
}

OAuthTokenPair FireEagle::getAccessToken(string oauth_verifier) {
    requireToken();
    FE_ParamPairs args;
    if (config->FE_OAUTH_VERSION == OAUTH_10A) {
        args["oauth_verifier"] = oauth_verifier;
    }
    string response = oAuthRequest(accessTokenURL(), FE_TOKEN_REQUEST, args);
    OAuthTokenPair oauth = oAuthParseResponse(response);

    if (oauth.token.length() && oauth.secret.length()) {
        if (token)
            delete token;
        token = new OAuthTokenPair(oauth);
        if (!token)
            throw new FireEagleException("Out of memory", FE_INTERNAL_ERROR);
    }

    return oauth;
}

OAuthTokenPair FireEagle::access_token(string oauth_verifier) { return getAccessToken(oauth_verifier); }

string FireEagle::call(const string &method, enum FE_oauth_token token_type,
                       const FE_ParamPairs &args, bool isPost,
                       enum FE_format format) const {
    requireToken();
    return oAuthRequest(methodURL(method, format), token_type, args, isPost);
}

string FireEagle::user(enum FE_format format) const {
    return call("user", FE_TOKEN_ACCESS, empty_params, false, format);
}

string FireEagle::update(const FE_ParamPairs &args, enum FE_format format) const {
    if (args.size() == 0)
        throw new FireEagleException("FireEagle::update() needs a location",
                                     FE_LOCATION_REQUIRED);
    return call("update", FE_TOKEN_ACCESS, args, true, format);
}

string FireEagle::lookup(const FE_ParamPairs &args, enum FE_format format) const {
    if (args.size() == 0)
        throw new FireEagleException("FireEagle::lookup() needs a location",
                                     FE_LOCATION_REQUIRED);
    return call("lookup", FE_TOKEN_ACCESS, args, false, format);
}

string FireEagle::within(const FE_ParamPairs &args, enum FE_format format) const {
    if (args.size() == 0)
        throw new FireEagleException("FireEagle::within() needs a location",
                                     FE_LOCATION_REQUIRED);
    return call("within", FE_TOKEN_GENERAL, args, false, format);
}

string FireEagle::recent(const FE_ParamPairs &args, enum FE_format format) const {
    //You can call without any args...
    return call("recent", FE_TOKEN_GENERAL, args, false, format);
}

static FE_format_info_t format_info[] = {
    { "xml" },
    { "json" },
    { "html" }
};

const FE_format_info_t *FE_format_info = &(format_info[0]);
const int FE_n_formats = sizeof(format_info)/sizeof(FE_format_info_t);

