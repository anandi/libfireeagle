/**
 * FireEagle helper class for actual HTTP/HTTPS requests.
 *
 * Copyright (C) 2009 Yahoo! Inc
 *
 */

#ifndef FIREEAGLE_HTTP_H
#define FIREEAGLE_HTTP_H

#include <curl/curl.h>

#include <string>
#include <list>

using namespace std;

/**
 * Class for throwing HTTP transaction related exceptions. Throw these in case of
 * anything not going smoothly in the derived classes of FireEagleHTTPAgent.
 */
class FireEagleHTTPException : public exception {
  public:
    /**
     * msg: English language message string containing the cause of the
     * exception.
     */
    string msg;

    FireEagleHTTPException(const string &_msg);
    virtual ~FireEagleHTTPException() throw();
};

/**
 * FireEagleHTTPAgent is an interface class to allow you to integrate any HTTP
 * agent that the authors like. See FireEagleCurl for an implementation.
 */
class FireEagleHTTPAgent {
  protected:
    /**
     * The entire URL along with the query params. The query params (if any) must
     * be already URL encoded.
     */
    string url;

    /**
     * If the HTTP request is a POST, then provide the post body in this string.
     * If this string length is zero, then this is a GET request. The string must
     * be URL encoded.
     */
    string postdata;

   /**
    * Set HTTP headers as required.
    */
   list<string> request_headers;

  public:
    /**
     * @param _url Sets the protected url member. See FireEagleHTTPAgent::url
     * @param _postdata Sets the protected postdata member. See FireEagleHTTPAgent::postdata
     */
    FireEagleHTTPAgent(const string &_url, const string &_postdata);
    virtual ~FireEagleHTTPAgent();

    /**
     * The interface which sets up the actual agent for making a call.
     */
    virtual void initialize_agent() = 0;

    /**
     * This method is called after initialize_agent to set any optional
     * settings for the HTTP agent to be used. For example, you may want
     * to optionally set proxy or turn off HTTPS certificate verifications
     * that cannot be done during initialize_agent because it provides you
     * with only a fixed set of options. Note that you do not *have to*
     * override this method if there is nothing to be done.
     */
    virtual void set_custom_request_opts();

    /**
     * Make the actual HTTP request using the agent initialized in
     * initialize_agent.
     * @return The HTTP status code (if any received), else 0.
     */
    virtual int make_call() = 0;

    /**
     * If the make_call returns 0 to indicate that the HTTP request was
     * not made, then this method is invoked to collect a agent-specific
     * error code for reporting. Always returns 0 in the default implementation.
     * @return A agent specific return code for the specific request failure
     * or 0 for no error.
     */
    virtual int agent_error();

    /**
     * Return the HTTP response body after the make_call, unless make_call
     * returned 0. It may be called even if the HTTP status code is not 200, to
     * extract any error response for reporting.
     * @return HTTP response body if any or empty string.
     */
    virtual string get_response() = 0;

    /**
     * Generic method to retrieve any HTTP header from the response to make_call.
     * Mostly to be used to retrieve the "Content-Type" and "Content-Length"
     * @param header The actual header name to be searched. Case sensitive.
     * @return Header (minus the header name) when exists. Empty string o/w.
     */
    virtual string get_header(const string &header) = 0;

    /**
     * Method to set a header for the requests.
     * @param header Name of the header.
     * @param value Header content.
     */
    void add_header(const string &header);

    /**
     * Clean up the HTTP agent created through initialize_agent. Must be able
     * to handle double destruction calls gracefully. It is advisable to call
     * this in the destructor of the derived class.
     */
    virtual void destroy_agent() = 0;

  private:
    /**
     * Sort of spoon feeding to avoid debugging errors later. We do not need
     * agent instances to be copied around, so hide them.
     */
    FireEagleHTTPAgent &operator=(const FireEagleHTTPAgent &instance); //Throws exception.
    FireEagleHTTPAgent(const FireEagleHTTPAgent &instance); //Throws exception.
};

/**
 * FireEagleCurl is a cURL implementation of FireEagleHTTPAgent. This is
 * provided by default. Implementation is not thread-safe, but that is not
 * a problem according to the invocation pattern.
 */
class FireEagleCurl : public FireEagleHTTPAgent {
  private:
    /**
     * Stores the actual response body from the HTTP request.
     */
    string response;

    /**
     * To remember the HTTP response code after the call. Initialized to 0.
     */
    long response_code;

  protected:
    /**
     * The curl instance pointer. It is protected to allow access from the
     * derived classes for set_custom_request_opts override. Initialized to
     * NULL. Set by initialize_agent.
     */
    CURL *curl;

  public:
    FireEagleCurl(const string &_url, const string &_postdata);

    /** Calls FireEagleCurl::destroy_agent */
    virtual ~FireEagleCurl();

    virtual void initialize_agent();
    virtual int make_call();
    virtual int agent_error();
    virtual string get_response();

    /**
     * Generic method to retrieve any HTTP header from the response to make_call.
     * Only supports "Content-Type" and "Content-Length".
     * See FireEagleHTTPAgent::get_header for details.
     */
    virtual string get_header(const string &header);

    /** Sets the FireEagleCurl::curl to NULL. */
    virtual void destroy_agent();
};

#endif //FIREEAGLE_HTTP_H
