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
#include "fire_parser.h"

using namespace std;

FireEagleException::FireEagleException(const string &_msg, int _code, const string &_response)
    : msg(_msg), code(_code), response(_response) {}

string FireEagleException::to_string() const {
    ostringstream os;

    os << "FireEagleException (code: " << code << "): " << msg << endl;
    if (response.length())
        os << "Response is: " << response << endl;
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

void OAuthTokenPair::init_from_string(const char *str) {
    char *pos = strchr(str, ' '); //Search for the separator whitespace.
    if (!pos)
        throw new FireEagleException("Invalid format", FE_INTERNAL_ERROR);

    *pos = 0; pos++;
    if (!strlen(str) || !strlen(pos))
        throw new FireEagleException("Invalid format", FE_INTERNAL_ERROR);

    token = string(str);
    secret = string(pos);
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

FireEagleConfig::FireEagleConfig(const string &conf_file)
    : app_token("",""), general_token("","") {
    common_init();

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
        if (strcmp(buffer, "app_token_file") == 0) {
            app_token = OAuthTokenPair(c);
        } else if (strcmp(buffer, "app_token_data") == 0) {
            app_token.init_from_string(c);
        } else if (strcmp(buffer, "general_token_file") == 0) {
            general_token = OAuthTokenPair(c);
        } else if (strcmp(buffer, "general_token_data") == 0) {
            general_token.init_from_string(c);
        } else if (strcmp(buffer, "root_url") == 0) {
            FE_ROOT = string(c);
        } else if (strcmp(buffer, "api_base_url") == 0) {
            FE_API_ROOT = string(c);
        } else {
            //No match. Ignore. We do not need to throw exceptions.
            continue;
        }
    }
    fclose(fp);

    if (!app_token.is_valid()) {
        string msg("FireEagleConfig: Could not initialize application token from config file ");
        msg.append(conf_file);
        throw new FireEagleException(msg, FE_INTERNAL_ERROR);
    }
}

FireEagleConfig::~FireEagleConfig() {} //Nothing much to do right now.

static void write_config(FILE *fp, const string &name, const string &value) {
    fprintf(fp, "%s:%s\n", name.c_str(), value.c_str());
}

void FireEagleConfig::save(const string &file) const {
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
    fclose(fp);
}

const OAuthTokenPair *FireEagleConfig::get_consumer_key() const { return &app_token; }

const OAuthTokenPair *FireEagleConfig::get_general_token() const {
    if (general_token.is_valid())
        return &general_token;
    return NULL;
}

static FEXMLNode *parseXMLMessage(const string &msg) {
    FEXMLParser parser;

    FEXMLNode *root = parser.parse(msg);
    if (!root)
        throw new FireEagleException("Invalid XML error response from Fire Eagle",
                                     FE_INTERNAL_ERROR, msg);

    return root;
}

static bool isXMLErrorMsg(const FEXMLNode *root, const string &msg) {
    if (root->element() != "rsp") {
        delete root;
        throw new FireEagleException("Unknown XML response format from Fire Eagle",
                                     FE_INTERNAL_ERROR, msg);
    }

    if (root->attribute("stat") == "ok")
        return false;

    if (root->attribute("stat") != "fail") {
        delete root;
        throw new FireEagleException("Unknown XML response format from Fire Eagle",
                                     FE_INTERNAL_ERROR, msg);
    }

    if (root->child(0).element() != "err") {
        delete root;
        throw new FireEagleException("Unknown XML response format from Fire Eagle",
                                     FE_INTERNAL_ERROR, msg);
    }

    return true;
}

static FireEagleException *exceptionFromXML(const FEXMLNode *root) {
    //Ideally this is not the guy who is being forced to throw any exception
    //It is just a factory method.
    string message("Remote error: ");
    message.append(root->child(0).attribute("msg"));
    FireEagleException *e = new FireEagleException(message,
                    strtol(root->child(0).attribute("code").c_str(), NULL, 0));

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
        if (contentType == "application/xml") { //Si Habla XML!!
            FEXMLNode *root = parseXMLMessage(response);
            if (isXMLErrorMsg(root, response)) {
                FireEagleException *e = exceptionFromXML(root);
                delete root;
                throw e;
            } //Don't do an else part. Even if we get a valid response with a non
              //200 HTTP code, proceed.
            delete root;
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
string FireEagle::oAuthRequest(const string &url, const FE_ParamPairs &args, bool isPost) const {
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
    char *result_tmp = oauth_sign_url(url_with_params.c_str(),
                                      (isPost) ? &postargs : NULL, OA_HMAC,
                                      consumer->token.c_str(), consumer->secret.c_str(),
                                      (token)? token->token.c_str() : NULL,
                                      (token)? token->secret.c_str() : NULL);

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
    switch (format) {
    case FE_FORMAT_XML:
        url.append(".xml");
        break;
    case FE_FORMAT_JSON:
        url.append(".json");
        break;
    }
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
    if (config->get_general_token()) {
        //There exists a general token. Use that as the oauth token...
        token = new OAuthTokenPair(*(config->get_general_token()));
        if (!token)
            throw new FireEagleException("Out of memory", FE_INTERNAL_ERROR);
    }
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

OAuthTokenPair FireEagle::getRequestToken() {
    string response = oAuthRequest(requestTokenURL());
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

OAuthTokenPair FireEagle::request_token() { return getRequestToken(); }

string FireEagle::getAuthorizeURL(const OAuthTokenPair &oauth) const {
    return authorizeURL() + "?oauth_token=" + oauth.token;
}

string FireEagle::authorize(const OAuthTokenPair &oauth) const {
    return getAuthorizeURL(oauth);
}

void FireEagle::requireToken() const {
    if (!token || !token->token.length() || !token->secret.length())
        throw new FireEagleException("This function requires an OAuth token",
                                     FE_TOKEN_REQUIRED);
}

OAuthTokenPair FireEagle::getAccessToken() {
    requireToken();
    string response = oAuthRequest(accessTokenURL());
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

OAuthTokenPair FireEagle::access_token() { return getAccessToken(); }

string FireEagle::call(const string &method, const FE_ParamPairs &args,
                      bool isPost, enum FE_format format) const {
    requireToken();
    return oAuthRequest(methodURL(method, format), args, isPost);
}

string FireEagle::user(enum FE_format format) const {
    return call("user", empty_params, false, format);
}

extern FE_user userFactory(const FEXMLNode *root);

FE_user FireEagle::user_object(const string &response, enum FE_format format) const {
    FEXMLNode *root = parseXMLMessage(response);
    if (isXMLErrorMsg(root, response)) {
        FireEagleException *e = exceptionFromXML(root);
        delete root;
        throw e;
    }

    try {
        FE_user user = userFactory(&(root->child(0)));
        delete root;
        return user;
    } catch(FireEagleException *fex) {
        delete root;
        throw fex;
    }
}

string FireEagle::update(const FE_ParamPairs &args, enum FE_format format) const {
    if (args.size() == 0)
        throw new FireEagleException("FireEagle::update() needs a location",
                                     FE_LOCATION_REQUIRED);
    return call("update", args, true, format);
}

string FireEagle::lookup(const FE_ParamPairs &args, enum FE_format format) const {
    if (args.size() == 0)
        throw new FireEagleException("FireEagle::lookup() needs a location",
                                     FE_LOCATION_REQUIRED);
    return call("lookup", args, false, format);
}

extern list<FE_location> lookupFactory(const FEXMLNode *root);

list<FE_location> FireEagle::lookup_objects(const string &response,
                                            enum FE_format format) const {
    FEXMLNode *root = parseXMLMessage(response);
    if (isXMLErrorMsg(root, response)) {
        FireEagleException *e = exceptionFromXML(root);
        delete root;
        throw e;
    }

    bool found = false;
    for (int i = 0 ; i < root->child_count() ; i++) {
        if (root->child(i).element() != "locations")
            continue;
        found = true;
        try {
            list<FE_location> locations = lookupFactory(&(root->child(i)));

            return locations;
        } catch(FireEagleException *fex) {
            delete root;
            throw fex;
        }
    }

    delete root;
    if (!found)
        throw new FireEagleException("Unknown XML response format for lookup API: No locations element present",
                                     FE_INTERNAL_ERROR, response);
}

string FireEagle::within(const FE_ParamPairs &args, enum FE_format format) const {
    if (args.size() == 0)
        throw new FireEagleException("FireEagle::within() needs a location",
                                     FE_LOCATION_REQUIRED);
    return call("within", args, false, format);
}



string FireEagle::recent(const FE_ParamPairs &args, enum FE_format format) const {
    //You can call without any args...
    return call("recent", args, false, format);
}

