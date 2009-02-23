/**
 * FireEagle OAuth+API C++ bindings
 *
 * Copyright (C) 2009 Yahoo! Inc
 *
 */

#ifndef FIREEAGLE_H
#define FIREEAGLE_H

#include <string>
#include <exception>
#include <map>

#include "fire_objects.h"

using namespace std;

//Error codes for different exceptions
#define FE_TOKEN_REQUIRED 1 // call missing an oauth request/access token
#define FE_LOCATION_REQUIRED 2 // call to update() without a location
#define FE_REMOTE_ERROR 3 // FE sent an error
#define FE_REQUEST_FAILED 4 // empty or malformed response from FE
#define FE_CONNECT_FAILED 5 // totally failed to make an HTTP request
#define FE_INTERNAL_ERROR 6 // totally failed to make an HTTP request
#define FE_CONFIG_READ_ERROR 7 // can't find or parse fireeaglerc

#define FE_REMOTE_SUCCESS 0 // Request succeeded.
#define FE_REMOTE_UPDATE_PROHIBITED 1 // Update not permitted for that user.
#define FE_REMOTE_UPDATE_ONLY 2 // Update successful, but read access prohibited
#define FE_REMOTE_QUERY_PROHIBITED 3 // Query not permitted for that user.
#define FE_REMOTE_SUSPENDED 4 // User account is suspended.
#define FE_REMOTE_PLACE_NOT_FOUND 6 // Place can't be identified.
#define FE_REMOTE_USER_NOT_FOUND 7 // Authentication token can't be matched
                                   // to a user.
#define FE_REMOTE_INVALID_QUERY 8 // Invalid location query.
#define FE_REMOTE_IS_FROB 10 // Token provided is a request token,
                             // not an auth token.
#define FE_REMOTE_NOT_VALIDATED 11 // Request token has not been validated.
#define FE_REMOTE_REQUEST_TOKEN_REQUIRED 12 // Token provided must be
                                            // an access token.
#define FE_REMOTE_EXPIRED 13 // Token has expired.
#define FE_REMOTE_GENERAL_TOKEN_REQUIRED 14 // Token provided must be an
                                            // general purpose token.
#define FE_REMOTE_UNKNOWN_CONSUMER 15 // Unknown consumer key.
#define FE_REMOTE_UNKNOWN_TOKEN 16 // Token not found.
#define FE_REMOTE_BAD_IP_ADDRESS 17 // Request made from non-blessed ip address.
#define FE_REMOTE_OAUTH_CONSUMER_KEY_REQUIRED 20 // oauth_consumer_key parameter required.
#define FE_REMOTE_OAUTH_TOKEN_REQUIRED 21 // oauth_token parameter required.
#define FE_REMOTE_BAD_SIGNATURE_METHOD 22 // Unsupported signature method.
#define FE_REMOTE_INVALID_SIGNATURE 23 // Invalid OAuth signature.
#define FE_REMOTE_REPEATED_NONCE 24 // Provided nonce has been seen before.
#define FE_REMOTE_YAHOOAPIS_REQUIRED 30 // All api methods should use fireeagle.yahooapis.com.
#define FE_REMOTE_SSL_REQUIRED 31 // SSL / https is required.
#define FE_REMOTE_RATE_LIMITING 32 // Rate limit/IP Block due to excessive requests.
#define FE_REMOTE_INTERNAL_ERROR 50 // Internal error occurred; try again later.

class FireEagleException : public exception {
  public:
    string msg;
    int code;
    string response; // for REMOTE_ERROR codes, this is the response from FireEagle (useful: $response->code and $response->message)

    FireEagleException(const string &_msg, int _code, const string &_response = "");
    virtual ~FireEagleException() throw();

    ostream &operator<<(ostream &os);
};

class OAuthTokenPair {
  public:
    string token;
    string secret;

    OAuthTokenPair(const string &_token, const string &_secret);
    OAuthTokenPair(const OAuthTokenPair &other);
    OAuthTokenPair(const string &file);

    void save(const string &file) const;
};

//extern const OAuthTokenPair empty_token;

enum FE_format { FE_FORMAT_XML = 1, FE_FORMAT_JSON };

//To cut down on long names for data types...
typedef map<string,string> FE_ParamPairs;

extern const FE_ParamPairs empty_params;

/**
 * FireEagle API access helper class.
 */
class FireEagle {
  private:
    OAuthTokenPair *consumer;
    OAuthTokenPair *token;

    // Make an HTTP request, throwing an exception if we get anything other than a 200 response
    string http(const string &url, const string postData = "") const;

    //Check that a proper token is set for the operation.
    void requireToken() const;
    string call(const string &method, const FE_ParamPairs &args = empty_params,
                bool isPost = false, enum FE_format format = FE_FORMAT_XML) const;
  protected:
    //Override to suit your own debug style.
    virtual void dump(const string &str) const;
  
    // Parse a URL-encoded OAuth response. Override as responses become more fancy with
    // version.
    virtual OAuthTokenPair oAuthParseResponse(const string &response) const;

    // Format and sign an OAuth / API request. Protected for testing.
    string oAuthRequest(const string &url, const FE_ParamPairs &args = empty_params,
                        bool isPost = false) const;

  public:
    /* It should be possible for the applications to override these if needed. */
    static string FE_ROOT;
    static string FE_API_ROOT;
    static bool FE_DEBUG; // set to true to print out debugging info
    static bool FE_DUMP_REQUESTS; // set to a pathname to dump out http requests to a log
    //Dangerous thing to do... Do not use this as false in production!!
    static bool FE_VERIFY_PEER;

    // OAuth URLs
    string requestTokenURL() const;
    string authorizeURL() const;
    string accessTokenURL() const;

    // API URLs
    string methodURL(const string &method, enum FE_format format = FE_FORMAT_XML) const;

    //Useful methods...
    OAuthTokenPair getRequestToken(); //Updates self internally with token received.
    OAuthTokenPair request_token(); //Aliased to getRequestToken().
    OAuthTokenPair getAccessToken(); //Exchange a request token for an access token.
    OAuthTokenPair access_token(); //Aliased to getAccessToken.

    //Actual API calls:
    string user(enum FE_format format = FE_FORMAT_XML) const;
    FE_user user_object(const string &response, enum FE_format format = FE_FORMAT_XML) const;
    string update(const FE_ParamPairs &args, enum FE_format format = FE_FORMAT_XML) const;
    string lookup(const FE_ParamPairs &args, enum FE_format format = FE_FORMAT_XML) const;
    list<FE_location> lookup_objects(const string &response,
                                     enum FE_format format = FE_FORMAT_XML) const;

    //Use the following API calls only in case of a Web application.
    string within(const FE_ParamPairs &args, enum FE_format format = FE_FORMAT_XML) const;
    string recent(const FE_ParamPairs &args, enum FE_format format = FE_FORMAT_XML) const;

    //Less useful methods...
    string getAuthorizeURL(const OAuthTokenPair &oauth) const;
    string authorize(const OAuthTokenPair &oauth) const; //Aliased to getAuthorizeURL

    //Constructors, destructors and paraphanelia
    FireEagle(const string &_consumerKey, const string &_consumerSecret,
              const string &_oAuthToken = "", const string &_oAuthTokenSecret = "");
    ~FireEagle();
    FireEagle(const FireEagle &other);
    FireEagle &operator=(const FireEagle &other);
};

#endif /* FIREEAGLE_H */
