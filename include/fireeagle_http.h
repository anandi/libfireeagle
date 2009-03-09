/**
 * FireEagle helper class for actual HTTP/HTTPS requests.
 *
 * Copyright (C) 2009 Yahoo! Inc
 *
 */

#ifndef FIREEAGLE_HTTP_H
#define FIREEAGLE_HTTP_H

#include <string>
#include <curl/curl.h>

using namespace std;

/**
 * FireEagleException: The only exception class that is used for all kinds of
 * exceptions.
 */
class FireEagleHTTPException : public exception {
  public:
    /**
     * msg: English language message string containing the cause of the
     * exception.
     */
    string msg;

    /**
     * FireEagleException: Standard constructor.
     * @_msg: Mandatory const string.
     */
    FireEagleHTTPException(const string &_msg);

    /**
     * ~FireEagleException: Standard virtual destructor.
     */
    virtual ~FireEagleHTTPException() throw();
};

/**
 * FireEagleHTTPAgent is an interface class to allow you to integrate any HTTP
 * agent that the authors like.
 */
class FireEagleHTTPAgent {
  protected:
    string url;
    string postdata;

  public:
    FireEagleHTTPAgent(const string &_url, const string &_postdata); //If postdata
                                 //is empty, treat this as a GET request.
    virtual ~FireEagleHTTPAgent();

    virtual void initialize_agent() = 0; //Must implement.
    virtual void set_custom_request_opts(); //Optional, for proxy or https CA,
                                 //should extend the implemented class.
    virtual int make_call() = 0; //Must implement. Return value is the HTTP
                                 //status code (if any received), else is 0.
    virtual int agent_error(); //Return 0 for no error.
    virtual string get_response() = 0; //Get HTTP response body if any. Else
                                       //empty string.
    virtual string get_header(const string &header) = 0; //Generic method
                                    //to retrieve any HTTP header from the
                                    //response to make_call. Mostly to be
                                    //used to retrieve the "Content-Type".
    virtual void destroy_agent() = 0; //Must implement. Should handle double
                                  //destruction calls gracefully.

  private:
    //Hide any chances of copying the instances.
    FireEagleHTTPAgent &operator=(const FireEagleHTTPAgent &instance); //Throws exception.
    FireEagleHTTPAgent(const FireEagleHTTPAgent &instance); //Throws exception.
};

//Implementation is not thread safe w.r.t instance.
/**
 * FireEagleCurl is a cURL implementation of FireEagleHTTPAgent. This is
 * provided by default.
 */
class FireEagleCurl : public FireEagleHTTPAgent {
  private:
    string response;
    long response_code;

  protected:
    CURL *curl;

  public:
    FireEagleCurl(const string &_url, const string &_postdata);
    virtual ~FireEagleCurl();

    virtual void initialize_agent();
    virtual int make_call();
    virtual int agent_error();
    virtual string get_response();
    virtual string get_header(const string &header); //Only 'Content-Type'
                                         //and 'Content-Length' are supported.
    virtual void destroy_agent();
};

#endif //FIREEAGLE_HTTP_H
