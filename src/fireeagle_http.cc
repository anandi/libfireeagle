/**
 * FireEagle helper class for actual HTTP/HTTPS requests.
 *
 * Copyright (C) 2009 Yahoo! Inc
 *
 */

#include <string>
#include <exception>
#include <sstream>

#include <stdlib.h>
#include <curl/curl.h>

#include "fireeagle_http.h"

using namespace std;

FireEagleHTTPException::FireEagleHTTPException(const string &_msg)
    : msg(_msg) {}

FireEagleHTTPException::~FireEagleHTTPException() throw() {}

FireEagleHTTPAgent::FireEagleHTTPAgent(const string &_url,
                                       const string &_postdata) {
    url = _url;
    postdata = _postdata;
}

FireEagleHTTPAgent::~FireEagleHTTPAgent() {}

FireEagleHTTPAgent::FireEagleHTTPAgent(const FireEagleHTTPAgent &instance) {
    throw new FireEagleHTTPException("Copy constructor invocation for FireEagleHTTPAgent not allowed.");
}

FireEagleHTTPAgent &FireEagleHTTPAgent::operator=(const FireEagleHTTPAgent &instance) {
    throw new FireEagleHTTPException("Assignment operator invocation for FireEagleHTTPAgent not allowed.");
}

void FireEagleHTTPAgent::set_custom_request_opts() {}

int FireEagleHTTPAgent::agent_error() { return 0; } //All agents may not implement errors.

FireEagleCurl::FireEagleCurl(const string &_url,
                             const string &_postdata)
    : FireEagleHTTPAgent(_url, _postdata) {
    response_code = 0;
    curl = curl_easy_init();
    if (!curl)
        throw new FireEagleHTTPException("Failed to initialize curl");
}

FireEagleCurl::~FireEagleCurl() {
    destroy_agent();
}

extern "C" size_t
curl_response_chunk_handler(void *ptr, size_t size, size_t nmemb, void *data) {
    size_t realsize = size * nmemb;
    string *sp = (string *)data;

    sp->append((char *)ptr);

    return realsize;
}

void FireEagleCurl::initialize_agent() {
    char *CA_path = getenv("CURL_CA_BUNDLE_PATH");
    if (CA_path)
        curl_easy_setopt(curl, CURLOPT_CAINFO, CA_path);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&(this->response));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_response_chunk_handler);

    if (postdata.length()) {
        curl_easy_setopt(curl, CURLOPT_POST, 1);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postdata.c_str());
    }
}

int FireEagleCurl::make_call() {
    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &(this->response_code));
    return (response_code > 99) ? response_code : 0;
}

int FireEagleCurl::agent_error() { return response_code; }

string FireEagleCurl::get_response() { return response; }

string FireEagleCurl::get_header(const string &header) {
    if (header == "Content-Type") {
        char *cType; //Don't free.
        curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &cType);
        string s((cType) ? cType : "<Unknown>");

        return s;
    } else if (header == "Content-Length") {
        double contentLength = 0;
        curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &contentLength);

        ostringstream os;
        os << contentLength;
        return os.str();
    } else {
        return "";
    }
}

void FireEagleCurl::destroy_agent() {
    if (curl) {
        curl_easy_cleanup(curl);
        curl = NULL;
    }
}

