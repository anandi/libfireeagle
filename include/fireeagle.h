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

#include "fireeagle_http.h"
#include "parser_iface.h"

using namespace std;

//Error codes for different exceptions
#define FE_TOKEN_REQUIRED 1 // call missing an oauth request/access token
#define FE_LOCATION_REQUIRED 2 // call to update() without a location
#define FE_REMOTE_ERROR 3 // FE sent an error
#define FE_REQUEST_FAILED 4 // empty or malformed response from FE
#define FE_CONNECT_FAILED 5 // totally failed to make an HTTP request
#define FE_INTERNAL_ERROR 6 // totally failed to make an HTTP request
#define FE_CONFIG_READ_ERROR 7 // can't find or parse fireeaglerc
#define FE_OAUTH_VERSION_MISMATCH 8 // server is not talking the same version.

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

/**
 * Used for all kinds of exceptions resulting from Fire Eagle transaction logic.
 */
class FireEagleException : public exception {
  public:
    /**
     * English language message string containing the cause of the exception.
     */
    string msg;
    /**
     * The local or remote error code.
     */
    int code;
    /**
     * The response (if any) string received in the content body
     * from the server.
     */
    string response;

    /**
     * Standard constructor.
     * @param _msg Mandatory const string.
     * @param _code Mandatory error code.
     * @param _response Optional response body from server. Empty string by default.
     */
    FireEagleException(const string &_msg, int _code, const string &_response = "");

    /**
     * Helper method to stream an exception
     */
    string to_string() const;

    /**
     * Standard virtual destructor.
     */
    virtual ~FireEagleException() throw();
};

/** enum to differentiate between tokens in their proper applications */
enum FE_oauth_token {
    FE_TOKEN_NONE = 0,
    FE_TOKEN_CONSUMER,
    FE_TOKEN_GENERAL,
    FE_TOKEN_REQUEST,
    FE_TOKEN_ACCESS
};

/**
 * Class representing the OAuth token and secret. For more details on OAuth,
 * visit http://oauth.net/ . Tokens can be of type:
 * - Consumer Key - Equivalent to an application's registration ID with Fire Eagle
 * - General Token - Given only to applications that qualify as a web app (i.e.
 * has a callback URL registered with Fire Eagle
 * - Request Token - Used as a transient token for authorizing individual users
 * on Fire Eagle
 * - Access Token - Actual per-user token for accessing any user's resources from
 * Fire Eagle.
 */
class OAuthTokenPair {
  public:
    /**
     * The OAuth token string.
     */
    string token;
    /**
     * The OAuth secret string.
     */
    string secret;

    /**
     * Standard constructor. Can be used for any kind of OAuth token used for
     * Fire Eagle transactions. Tokens can be of type:
     * @param _token The OAuth token
     * @param _secret The secret associated with _token.
     */
    OAuthTokenPair(const string &_token, const string &_secret);

    /**
     * Copy constructor.
     */
    OAuthTokenPair(const OAuthTokenPair &other);

    /**
     * Constructor as deserializer.
     * @param file File name to read token and secret. See 'save' method for details
     * of file format.
     */
    OAuthTokenPair(const string &file);

    /**
     * Deserializer from string.
     * @param str Contains the same data format as that in the file passed to the
     * constructor from file.
     */
    void init_from_string(const char *str);

    /**
     * Serializer
     * @param file File name to write the token and secret to. File contains a single
     * line of the form -- '&lt;token&gt; &lt;secret&gt;'
     */
    void save(const string &file) const;

    /**
     * Serialize to string
     * @return The same string format as is saved to the file using OAuthTokenPair::save
     */
    string to_string() const;

    /**
     * Method to check whether a token pair is validly constructed. Currently
     * checks only whether the token is an empty string
     */
    bool is_valid() const;
};

/**
 * enum for supported formats for API response from Fire Eagle.
 */
enum FE_format { FE_FORMAT_XML = 0, FE_FORMAT_JSON, FE_FORMAT_HTML };

/**
 * Helper data structure for storing relevant info for each response format
 */
typedef struct s_FE_format_info {
    /** The URL extension for specifying the format */
    char *extension; /* 'xml' for FE_FORMAT_XML, 'json' for FE_FORMAT_JSON, ... */
} FE_format_info_t;

extern const FE_format_info_t *FE_format_info; /* Pointer to array */
extern const int FE_n_formats; /* Size of array */

/**
 * A class to be used to store a particular parser. I *AM* making things fancy here!
 */
class ParserData {
  public:
    /** Destructor of abstract classes are virtual. */
    virtual ~ParserData() {}

    /**
     * Retrieve the supported format for the parser.
     * @return One of the supported response type.
     */
    virtual enum FE_format lang() const = 0;

    /**
     * Get an instance of the actual parser object.
     * @return A pointer to a derived class for a FE_Parser
     */
    virtual FE_Parser *parser_instance() const = 0;
};

enum FE_oauth_version { OAUTH_10 = 0, OAUTH_10A };

/**
 * A class for storing the FireEagle common stuff that applies across the
 * application. Normally we would expect this to be a singleton, but I am
 * refraining from making it so, just in case someone has more than one
 * set of app tokens for the app!
 */
class FireEagleConfig {
  private:
    /** The token pair representing an app id */
    OAuthTokenPair app_token;

    /** For applications registered as a web application, the config has an
     * additional token pair for general queries */
    OAuthTokenPair general_token;

    /** Common initialization function */
    void common_init();

    /** Common initialization function with key-value pairs */
    void common_init_with_config(const map<string,string> &config);

    /** Map for parsers of different response content types. */
    map<string,ParserData *> parsers;

  public:
    /** Contains the root URL for Fire Eagle installation. Should be possible to
     * override and point to some other test install by internal QA.
     */
    string FE_ROOT;

    /** Contains the base URL for the API methods. Should be possible to override
     * and point to some other test install by internal QA.
     */
    string FE_API_ROOT;

    /** Set to true to turn on debug message dumps through the 'dump' method.
     */
    bool FE_DEBUG;

    /** Set to true to turn on dumping of requests and responses to cerr */
    bool FE_DUMP_REQUESTS;

    /** Set the correct enum for OAuth version to be used */
    enum FE_oauth_version FE_OAUTH_VERSION;

    /** Use 'Authorization: Oauth' header for passing oauth_* params in GET
     * request. Set to false by default.
     */
    bool FE_USE_OAUTH_HEADER;

    /** Constructs an instance without a general_token */
    FireEagleConfig(const OAuthTokenPair &_app_token);

    /** Constructs an instance with both app & general token */
    FireEagleConfig(const OAuthTokenPair &_app_token,
                    const OAuthTokenPair &_general_token);

    /** Construct an instance from a map of key-value pairs. Not to be used
     * directly */
    FireEagleConfig(const map<string,string> &config);

    /** Construct an instance from the configuration file. The format of the
     * file is as generated by the save method. */ 
    FireEagleConfig(const string &conf_file);

    virtual ~FireEagleConfig();

    /** Save an instance data to a file. */
    void save(const string &file) const;

    /** Save an instance data to a file along with additional key-values. */
    void savex(const string &file, const map<string,string> &extra) const;

    /** Getter for the FireEagleConfig::app_token */
    const OAuthTokenPair *get_consumer_key() const;

    /** Getter for the FireEagleConfig::general_token
     * @return Returns NULL if the general token is invalid.
     */
    const OAuthTokenPair *get_general_token() const;

    /** Add new parsers based on response content types.
     * @param content_type The content type which will be parsed using this parser.
     * @param parser Pointer to an instance of a derived class of ParserData.
     * @return A pointer to any previous instance registered with the same content_type,
     * or NULL o/w.
     */
    ParserData *register_parser(const string &content_type, ParserData *parser);

    /** Find a parser (if registered) for the given content type
     * @param content_type The content type which needs to be handled.
     * @return A registered instance (see FireEagleConfig::register_parser) of
     * ParserData for desired content_type or NULL o/w
     */
    ParserData *get_parser(const string &content_type);

    /** Find a parser (if registered) for a given response format.
     * @param format The choice of format.
     * @return A registered instance (see FireEagleConfig::register_parser) of
     * ParserData for the desired format. If multiple parsers (for different
     * content types) are registered for the same format, it returns a random
     * one of the matches.
     */
    ParserData *get_parser(enum FE_format format);

    /** Parse config file. Contains the logic for reading a config file
     * @param file Path to the configuration file to be parsed.
     * @return A map of key-value pairs, both strings
     */
    static map<string,string> parse_config(const string &file);
};

/**
 * Typedef to cut down on long names for data types...
 */
typedef map<string,string> FE_ParamPairs;

extern const FE_ParamPairs empty_params;

/**
 * FireEagle API access helper class. See http://fireeagle.yahoo.net/ for details.
 * Almost all methods tend to raise FireEagleException on any kind of internal or
 * external failure.
 */
class FireEagle {
  private:
    FireEagleConfig *config;
    OAuthTokenPair *token;
    bool use_oauth_header; //If the config says FE_USE_OAUTH_HEADER = true, then
                           //set this when oauth credentials have to be passed
                           //through headers.

    /** Make an HTTP request, throwing an exception if we get anything
     * other than a 200 response. Request type (GET or POST) is decided by the
     * the length of the 'postData' param.
     * @param url Complete URL with any GET query params. Params should be URL encoded.
     * @param postData Empty string by default, unless request is a POST. 
     * @return Response string on success.
     */
    string http(const string &url, const string postData = "") const;

    /** Generic interface for API calls.
     * @param method Name of the method being called.
     * @param args list of key-value pairs to be passed as arguments to the API call.
     * @param isPost False by default. Set to true to make a POST request.
     * @param format Choose either XML (default) or JSON.
     * @return Response string on success.
     */
    string call(const string &method, enum FE_oauth_token token_type,
                const FE_ParamPairs &args = empty_params, bool isPost = false,
                enum FE_format format = FE_FORMAT_XML) const;

    /** This function extracts all the OAuth-specific parameters from the URL
     * encoded parameters and puts them in an 'Authorization:' header.
     * @param url URL string with URL encoded params. The url is modified after
     * extraction of oauth params.
     * @param realm A security realm. Empty string by default. See
     * http://oauth.net/core/1.0/#auth_header
     * @return Header string or empty string.
     */
    string make_oauth_header(string &url, const string &realm = "") const;

  protected:
    /** This function is called with debug messages when FE_DEBUG is turned on.
     * Override to suit your own debug style. Default implementation outputs strings
     * to cout.
     * @param str Debug message string.
     */
    virtual void dump(const string &str) const;
  
    /** Parse a URL-encoded OAuth response. This is not the same as a response from a
     * Fire Eagle API response. It is used for getting OAuth tokens, especially request
     * and access tokens. Override to do any extra processing or dumping.
     * @param response URL encoded response string containing OAuth token and secret.
     * @return Parsed object for token and secret.
     */
    virtual FE_ParamPairs oAuthParseResponse(const string &response) const;

    /** Format and sign an OAuth / API request. Protected for testing.
     * @param url Complete URL with any GET query params. Params should be URL encoded.
     * @param token_type Type of optional token required for making request. Consumer
     * token requirement is never explicitly mentioned.
     * @param args list of key-value pairs to be passed as arguments to the API call.
     * @param isPost False by default. Set to true to make a POST request.
     * @return HTTP response body.
     */
    virtual string oAuthRequest(const string &url,
                                enum FE_oauth_token token_type,
                                const FE_ParamPairs &args = empty_params,
                                bool isPost = false) const;

    /** Get an abstracted HTTP agent class to use. Can be extended to support
     * any extensible functionality in the agents. Arguments are passed directly to the
     * constructor of the actual agent.
     * @param url Complete URL with any GET query params. Params should be URL encoded.
     * @param postdata Empty string by default, unless request is a POST. 
     * @return A FireEagleHTTPAgent pointer (FireEagleCurl agent by default).
     */
    virtual FireEagleHTTPAgent *HTTPAgent(const string &url,
                                          const string &postdata = "") const;

  public:
    /** Generates the URL for the HTTP GET request for getting a OAuth Request Token.
     * @return URL string.
     */
    string requestTokenURL() const;

    /** Generates the base URL for the HTTP GET request with which to redirect the user
     * to Fire Eagle site for authorization. See also: FireEagle::getAuthorizeURL.
     * @return URL string.
     */
    string authorizeURL() const;

    /** Generates the URL for the HTTP GET request for retrieving a OAuth Access Token
     * for an user once authorization has been performed with the request token.
     * @return URL string.
     */
    string accessTokenURL() const;

    /** Utility method for generating a URL calling a specific API. Available APIs are:
     * - user (Looks up a given user's location. Needs an access token)
     * - lookup (Geocodes a generic location to a lat/lon pair and a WOEID. Needs a
     * access or general token)
     * - update (Updates a user's location in Fire Eagle database. Needs an access token)
     * - within (Gets the list of all user tokens accessible by the consumer key,
     * who are currently within the geographic area denoted by a WOEID sent as argument)
     * - recent (Get the list of all user tokens who have updated their location
     * recently)
     * @param method The name of the API being called.
     * @param format Enum to specify the response format (XML by default).
     * @return URL string with query params, without the API specific arguments.
     */
    string methodURL(const string &method, enum FE_format format = FE_FORMAT_XML) const;

    /** Query Fire Eagle to extract a request token. Updates 'this' internally
     * the request token received.
     * @param oauth_callback Required with version 1.0 Rev A. If the
     * FireEagleConfig used has FE_OAUTH_VERSION not set to default OAUTH_10,
     * then this string will be the actual OAuth callback to be called on
     * authorization. If nothing is passed, the default is 'oob'
     * @return OAuthTokenPair representing the request token received. In case 
     * error, returns a instance with zero-length string for token.
     */
    OAuthTokenPair getRequestToken(string oauth_callback = "oob");

    /** Aliased to getRequestToken */
    OAuthTokenPair request_token(string oauth_callback = "oob");

    /** Query Fire Eagle to extract a access token based on the Request token which
     * is currently set in 'this'. Updates 'this' internally to use the access token
     * received.
     * @return OAuthTokenPair representing the access token received. In case of
     * @param oauth_verifier Needed if Rev A of oauth 1.0 is being followed.
     * error, returns a instance with zero-length string for token.
     */
    OAuthTokenPair getAccessToken(string oauth_verifier = "");

    /** Aliased to getAccessToken. */
    OAuthTokenPair access_token(string oauth_verifier = "");

    /** The 'user' API call. Uses the token in 'this' as the access token.
     * See http://fireeagle.yahoo.net/developer/explorer for more details on
     * individual APIs and parameters.
     * @param format Enum to specify the response format (XML by default).
     * @return HTTP response in the format requested.
     */
    string user(enum FE_format format = FE_FORMAT_XML) const;

    /** The 'update' API call. Uses the token in 'this' as the access token.
     * See http://fireeagle.yahoo.net/developer/explorer for more details on
     * individual APIs and parameters.
     * @param args Actual name-value pairs as arguments for API.
     * @param format Enum to specify the response format (XML by default).
     * @return HTTP response in the format requested.
     */
    string update(const FE_ParamPairs &args, enum FE_format format = FE_FORMAT_XML) const;

    /** The 'lookup' API call. Uses the token in 'this' as the access token.
     * See http://fireeagle.yahoo.net/developer/explorer for more details on
     * individual APIs and parameters.
     * @param args Actual name-value pairs as arguments for API.
     * @param format Enum to specify the response format (XML by default).
     * @return HTTP response in the format requested.
     */
    string lookup(const FE_ParamPairs &args, enum FE_format format = FE_FORMAT_XML) const;

    /** The 'within' API call. Uses the token in 'this' as the general token.
     * Note that this API is available to only consumer keys that have a
     * corresponding general token, i.e. consumer keys given to applications that
     * have registered as a web application.
     * See http://fireeagle.yahoo.net/developer/explorer for more details on
     * individual APIs and parameters.
     * @param args Actual name-value pairs as arguments for API.
     * @param format Enum to specify the response format (XML by default).
     * @return HTTP response in the format requested.
     */
    string within(const FE_ParamPairs &args, enum FE_format format = FE_FORMAT_XML) const;

    /** The 'recent' API call. Uses the token in 'this' as the general token.
     * Note that this API is available to only consumer keys that have a
     * corresponding general token, i.e. consumer keys given to applications that
     * have registered as a web application.
     * See http://fireeagle.yahoo.net/developer/explorer for more details on
     * individual APIs and parameters.
     * @param args Actual name-value pairs as arguments for API.
     * @param format Enum to specify the response format (XML by default).
     * @return HTTP response in the format requested.
     */
    string recent(const FE_ParamPairs &args, enum FE_format format = FE_FORMAT_XML) const;

    /** Generate an actual URL with which to redirect the user to Fire Eagle site
     * along with a request token, so that the user can authorize the application
     * to access the location.
     * @param oauth OAuthTokenPair representing the request token from Fire Eagle.
     * see FireEagle::getRequestToken for getting the request token.
     * @param callback Pass a callback URL along with the authorize URL, so 
     * that Fire Eagle will call back the app. Deprecated with rev A of OAuth
     * 1.0
     * @return URL to which the user should be redirected (complete with GET parameters).
     */
    string getAuthorizeURL(const OAuthTokenPair &oauth,
                           const string &callback = "") const;

    /** Aliased to getAuthorizeURL. */
    string authorize(const OAuthTokenPair &oauth,
                     const string &callback = "") const;

    //Constructors, destructors and paraphanelia
    /**
     * Use this constructor for operations that involve a request or access token.
     * @param _config The Fire Eagle configuration settings to be shared among
     * instances.
     * @param _token Reference to a token to use for operations. Tokens can
     * be either request token or access token.
     */
    FireEagle(FireEagleConfig *_config, const OAuthTokenPair &_token);

    /**
     * Use this constructor when operations require only a app token (consumer
     * token) or a general token.
     * @param _config The Fire Eagle configuration settings to be shared among
     * instances.
     */
    FireEagle(FireEagleConfig *_config);

    /**
     * Does not delete the FireEagleConfig pointer (FireEagle::config)
     */
    ~FireEagle();

    /**
     * Uses the same FireEagle::config pointer
     */
    FireEagle(const FireEagle &other);

    /**
     * Uses the same FireEagle::config pointer
     */
    FireEagle &operator=(const FireEagle &other);
};

#endif /* FIREEAGLE_H */
