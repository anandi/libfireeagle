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
#include <curl/curl.h>

extern "C" {
#include "oauth.h"
}

#include "fireeagle.h"
#include "fire_objects.h"
#include "fire_parser.h"

using namespace std;

FireEagleException::FireEagleException(const string &_msg, int _code, const string &_response)
    : msg(_msg), code(_code), response(_response) {
    if (FireEagle::FE_DEBUG) {
        cerr << "FireEagleException (code: " << _code << "): " << msg << endl;
        if (response.length())
            cerr << "Response is: " << response << endl;
    }
}

FireEagleException::~FireEagleException() throw() {}

ostream &FireEagleException::operator<<(ostream &os) {
    os << "Fire Eagle Exception: " << msg << " (code = " << code << endl;
    if (response.length())
        os << "Response: " << response << endl;
    return os;
}

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
    pos = strchr(buffer, ' '); //Search for the separator whitespace.
    if (!pos) {
        ostringstream os;
        os << "Token file: " << file << " . Invalid format.";
        throw new FireEagleException(os.str(), FE_INTERNAL_ERROR);
    }

    *pos = 0; pos++;
    if (!strlen(buffer) || !strlen(pos)) {
        ostringstream os;
        os << "Token file: " << file << " . Invalid format.";
        throw new FireEagleException(os.str(), FE_INTERNAL_ERROR);
    }

    token.append(buffer);
    secret.append(pos);
}

void OAuthTokenPair::save(const string &file) const {
    if (!token.length() || !secret.length()) {
        throw new FireEagleException("Cannot save an empty OAuthTokenPair.",
                                     FE_INTERNAL_ERROR);
    }

    FILE *fp = fopen(file.c_str(), "w");
    if (!fp) {
        ostringstream os;
        os << "Could not open token file (" << file << ") for saving.";
        throw new FireEagleException(os.str(), FE_INTERNAL_ERROR);
    }

    fprintf(fp, "%s %s\n", token.c_str(), secret.c_str());
    fclose(fp);
}

const FE_ParamPairs empty_params;

extern "C" size_t
curl_response_chunk_handler(void *ptr, size_t size, size_t nmemb, void *data) {
    size_t realsize = size * nmemb;
    string *sp = (string *)data;

    sp->append((char *)ptr);

    return realsize;
}

string FireEagle::FE_ROOT("http://fireeagle.yahoo.net");
string FireEagle::FE_API_ROOT("https://fireeagle.yahooapis.com");
bool FireEagle::FE_DEBUG = false;
bool FireEagle::FE_DUMP_REQUESTS = false;

// Make an HTTP request, throwing an exception if we get anything other than a 200 response
string FireEagle::http(const string &url, const string postData) const {
    if (FireEagle::FE_DEBUG)
        cerr << "[FE HTTP request: url: " << url << ", post data: " << postData << endl;
    if (FireEagle::FE_DUMP_REQUESTS) {
        ostringstream os;
        os << "[FE HTTP request: url: " << url << ", post data: " << postData;
        dump(os.str());
    }

    CURL *curl = curl_easy_init();
    if (!curl)
        throw new FireEagleException("Failed to initialize curl", FE_CONNECT_FAILED);

    char *CA_path = getenv("CURL_CA_BUNDLE_PATH");
    if (CA_path)
        curl_easy_setopt(curl, CURLOPT_CAINFO, CA_path);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30);
    string response;
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_response_chunk_handler);
    if (postData.length()) {
        curl_easy_setopt(curl, CURLOPT_POST, 1);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
    }

    curl_easy_perform(curl);
        
    long responseCode;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
    double contentLength;
    curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &contentLength);

    char *cType; //Don't free.
    curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &cType);
    string s((cType) ? cType : "<Unknown>");
    size_t pos = s.find(';');
    string contentType = s.substr(0, pos);

    curl_easy_cleanup(curl);

    if (!responseCode) {
        ostringstream os;
        os << "Connection to " << url << " failed";
        throw new FireEagleException(os.str(), FE_CONNECT_FAILED);
    }
    if (responseCode != 200) {
        ostringstream os;
        os << "Request to " << url << " failed: HTTP error " << responseCode;
        throw new FireEagleException(os.str(), FE_REQUEST_FAILED, response);
    }

    if (FireEagle::FE_DUMP_REQUESTS) {
        ostringstream os;
        os << "HTTP/1.0 " << responseCode << " OK" << endl;
        os << "Content-Type: " << contentType << endl;
        os << "Content-Length: " << contentLength << endl;
        os << endl << response << endl << endl;
        dump(os.str());
    }

    if (FireEagle::FE_DEBUG)
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
            char *name = oauth_url_escape(iter->first.c_str());
            char *value = oauth_url_escape(iter->second.c_str());
            if (!(name && value))
                throw new FireEagleException("Out of memory", FE_INTERNAL_ERROR);
            if (!first_time)
                url_with_params.append("&");
            else
                first_time = false;
            url_with_params.append(name);
            url_with_params.append("=");
            url_with_params.append(value);
            free(name);
            free(value);
        }
    }

    char *postargs = NULL;
    char *result_tmp = oauth_sign_url(url_with_params.c_str(),
                                      (isPost) ? &postargs : NULL, OA_HMAC,
                                      consumer->token.c_str(), consumer->secret.c_str(),
                                      (token)? token->token.c_str() : NULL,
                                      (token)? token->secret.c_str() : NULL);

    if (FireEagle::FE_DUMP_REQUESTS) {
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
    return string(FireEagle::FE_API_ROOT).append("/oauth/request_token");
}

string FireEagle::authorizeURL() const {
    return string(FireEagle::FE_ROOT).append("/oauth/authorize");
}

string FireEagle::accessTokenURL() const {
    return string(FireEagle::FE_API_ROOT).append("/oauth/access_token");
}

// API URLs
string FireEagle::methodURL(const string &method, enum FE_format format) const {
    string url(FireEagle::FE_API_ROOT);
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

FireEagle::FireEagle(const string &_consumerKey, const string &_consumerSecret,
                     const string &_oAuthToken, const string &_oAuthTokenSecret) {
    consumer = new OAuthTokenPair(_consumerKey, _consumerSecret);
    if (!consumer)
        throw new FireEagleException("Out of memory", FE_INTERNAL_ERROR);
    token = NULL;
    if (_oAuthToken.length() && _oAuthTokenSecret.length()) {
        token = new OAuthTokenPair(_oAuthToken, _oAuthTokenSecret);
        if (!token)
            throw new FireEagleException("Out of memory", FE_INTERNAL_ERROR);
    }
}

FireEagle::~FireEagle() {
    if (consumer)
        delete consumer;
    if (token)
        delete token;
}

FireEagle::FireEagle(const FireEagle &other) {
    consumer = new OAuthTokenPair(*(other.consumer));
    if (!consumer)
        throw new FireEagleException("Out of memory", FE_INTERNAL_ERROR);
    if (other.token) {
        token = new OAuthTokenPair(*(other.token));
        if (!token)
            throw new FireEagleException("Out of memory", FE_INTERNAL_ERROR);
    } else
        token = NULL;
}

FireEagle &FireEagle::operator=(const FireEagle &other) {
    if (consumer)
        delete consumer;
    if (token)
        delete token;

    consumer = new OAuthTokenPair(*(other.consumer));
    if (!consumer)
        throw new FireEagleException("Out of memory", FE_INTERNAL_ERROR);
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

    if (FireEagle::FE_DUMP_REQUESTS) {
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

    FEXMLParser parser;

    FEXMLNode *root = parser.parse(response);
    if (!root)
        throw new FireEagleException("Invalid XML response for user API", FE_INTERNAL_ERROR,
                                     response);

    if (root->element() != "rsp")
        throw new FireEagleException("Unknown XML response format for user API",
                                     FE_INTERNAL_ERROR, response);
    if (root->attribute("stat") != "ok") {
        if (root->child(0).element() != "err")
            throw new FireEagleException("Unknown XML error format for user API",
                                         FE_INTERNAL_ERROR, response);
        string message("Remote error: ");
        message.append(root->child(0).attribute("msg"));
        throw new FireEagleException(message,
                                  strtol(root->child(0).attribute("code").c_str(), NULL, 0));
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

    FEXMLParser parser;

    FEXMLNode *root = parser.parse(response);
    if (!root)
        throw new FireEagleException("Invalid XML response for lookup API", FE_INTERNAL_ERROR,
                                     response);

    if (root->element() != "rsp")
        throw new FireEagleException("Unknown XML response format for lookup API",
                                     FE_INTERNAL_ERROR, response);
    if (root->attribute("stat") != "ok") {
        if (root->child(0).element() != "err")
            throw new FireEagleException("Unknown XML error format for lookup API",
                                         FE_INTERNAL_ERROR, response);
        string message("Remote error: ");
        message.append(root->child(0).attribute("msg"));
        throw new FireEagleException(message,
                                  strtol(root->child(0).attribute("code").c_str(), NULL, 0));
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

