/**
 * FireEagle OAuth+API C++ bindings
 *
 * Copyright (C) 2009 Yahoo! Inc
 *
 */
#include <iostream>
#include <string.h>

#include <stdlib.h>
#include <assert.h>
#include "fireeagle.h"
#include "fire_objects.h"
#include "expat_parser.h"

#include <curl/curl.h>

//Global to avoid having to pass this around.
FireEagleConfig *fe_config = NULL;

//Extend the default Curl HTTP Agent to handle HTTPS_noverify.
class MyFireEagleCurlAgent : public FireEagleCurl {
  public:
    static bool https_noverify;

    MyFireEagleCurlAgent(const string &url, const string &post)
        : FireEagleCurl(url, post) {}

  protected:
    void set_custom_request_opts() {
        //Dangerous stuff to do! Please do not do such things in production.
        if (https_noverify) {
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
        }
    }
};

bool MyFireEagleCurlAgent::https_noverify = false;

/*
 * Ummm... why exactly did we need this class? Oh yes... we wanted to print the
 * actual HTTP(S) request to be made, without actually making it. Why not use
 * --debug instead?? Because, it does not stop the request from being made. Once
 *  the request is made, the nonce is registered at the server side, so the
 *  output cannot be used to fire again. Hence this new class.
 */
class HTTPRequestPrinter : public FireEagleHTTPAgent {
  public:
    HTTPRequestPrinter(const string&_url, const string &_postdata)
        : FireEagleHTTPAgent(_url, _postdata) {}
    ~HTTPRequestPrinter() {}

    void initialize_agent() {}
    int make_call() {
        list<string>::iterator iter;
        for (iter = request_headers.begin() ; iter != request_headers.end() ;
             iter++) {
            cout << "[Header]" << *iter << endl;
        }
        cout << "Request URL: " << url << endl;
        if (postdata.length() > 0)
            cout << "Post Body: " << postdata << endl;
        return 0; /* Internal agent error ;) */
    }
    string get_response() { return ""; }
    string get_header(const string &header) { return ""; }
    void destroy_agent() {}
};

//Extended to use a different HTTP agent (actuall different agent settings)
class MyFireEagle : public FireEagle {
  public:
    bool dummy_request; /* Set this to true so that no actual request happens */

  protected:
    FireEagleHTTPAgent *HTTPAgent(const string &url,
                                  const string &postdata) const {
        FireEagleHTTPAgent *pAgent = NULL;
        pAgent = (dummy_request) ? (FireEagleHTTPAgent *) new HTTPRequestPrinter(url, postdata)
                                 : (FireEagleHTTPAgent *) new MyFireEagleCurlAgent(url, postdata);
        return pAgent;
    }

  public:
    MyFireEagle(FireEagleConfig *_config, const OAuthTokenPair &_token)
        : FireEagle(_config, _token) { dummy_request = false; }
    MyFireEagle(FireEagleConfig *_config) : FireEagle(_config) { dummy_request = false; }
};

void usage() {
    cout << "Arguments (common):" << endl;
    cout << "\t--help (Shows this help)" << endl;
    cout << "\t--app-token-file <file> Mandatory arg. (You can set the FE_APPTOKEN environment instead)" << endl;
    cout << "\t--general-token-file <file> General token for within and recent queries" << endl;
    cout << "\t--token-file <file> Request or Access token as needed for operation." << endl;
    cout << "\t--token <token> <secret> Request or Access token as needed for operation. Overrides --token-file" << endl;
    cout << "\t--out-token <file> Optional. Use with commands that generate request or access tokens to save to file." << endl;
    cout << "\t--format [json|xml|html] Default xml. Use html only with --fake-request." << endl;
    cout << "\t--fe-root <base URL> Point to the Fire Eagle installation [" << endl;     cout << "\t--debug Enable verbose output" << endl;
    cout << "\t--https-noverify Disable HTTPS peer verification" << endl;
    cout << "\t--save-fe-config <file> Save the Fire Eagle config to a file" << endl;
    cout << "\t--fe-config <file> Load Fire Eagle config from a file. This avoids --app-token-file, --general-token, --fe-root" << endl;
    cout << "\t--fake-request Do not make actual request. Dump the possible request to stout." << endl;
    cout << "\t--oauth-version [1.0|1.0a] Defaults to 1.0a." << endl;
    cout << "\t--oauth-header Send OAuth params through header when possible." << endl;
    cout << "\nOAuth Commands: (use --token or --token-file where tokens are needed)" << endl;
    cout << "\t--get_request_tok [oauth_callback=<value>] oauth_callback is used only with --oauth-version 1.0a. It has no effect in original 1.0 protocol. If not specified, the default oob value is taken for 1.0a" << endl;
    cout << "\t--get_authorize_url [oauth_callback=<value>] You can optionally specify a request token. Based on the --oauth-version used, the oauth_callback may be sent with the authorize URL (version 1.0) or sent when retrieving a request token when not specified (version 1.0a)" << endl;
    cout << "\t--get_access_token [oauth_verifier=<value>] Needs a request token. Needs a verifier if --oauth-version is 1.0a." << endl;
    cout << "\nAPI commands: Always mention the access token throug --token or --token-file." << endl;
    cout << "\t--get_location Needs an access token" << endl;
    cout << "\t--lookup Needs an access token <name>=<value> [<name>=<value>,[...]]" << endl;
    cout << "\t--update Needs an access token <name>=<value> [<name>=<value>,[...]]" << endl;
    cout << "\t--within Needs a general token <name>=<value> [<name>=<value>,[...]]" << endl;
    cout << "\t--recent Needs a general token <name>=<value> [<name>=<value>,[...]]" << endl;
}

OAuthTokenPair request_token(FireEagle &fe, const FE_ParamPairs &args) {
    FE_ParamPairs::const_iterator iter = args.find("oauth_callback");
    OAuthTokenPair reqTok = (iter == args.end()) ? fe.request_token()
                                               : fe.request_token(iter->second);

    cout << "Request Token: " << reqTok.token << endl;
    cout << "Request Secret: " << reqTok.secret << endl;

    return reqTok;
}

string authorize_url(FireEagle &fe, OAuthTokenPair &request,
                     const FE_ParamPairs &args) {
    FE_ParamPairs::const_iterator iter = args.find("oauth_callback");
    string url = (iter == args.end()) ? fe.getAuthorizeURL(request)
                                    : fe.getAuthorizeURL(request, iter->second);

    cout << "Authorize URL: " << url << endl;
    return url;
}

OAuthTokenPair access_token(FireEagle &fe, string verifier) {
    OAuthTokenPair accessTok = fe.access_token(verifier);

    cout << "Access Token: " << accessTok.token << endl;
    cout << "Access Secret: " << accessTok.secret << endl;

    return accessTok;
}

string location(FireEagle &fe, enum FE_format format) {
    string response = fe.user(format);

//    if (format == "json") {
        cout << "Location Response: " << response << endl;
/*    } else {
        try {
            FE_user user = fe.user_object(response);
            user.print(cout);
        } catch (FireEagleException *fex) {
            cerr << "Fire Eagle exception (Message: " << fex->msg << ")" << endl;
        }
    }*/
    
    return response;
}

string lookup(FireEagle &fe, const FE_ParamPairs &args, enum FE_format format) {
    string response = fe.lookup(args, format);

    if (format == FE_FORMAT_JSON) {
        cout << "Lookup response: " << response << endl;
    } else {
        try {
            list<FE_location> locations = FE_location::from_response(response,
                                                                    FE_FORMAT_XML,
                                                                    fe_config);
            list<FE_location>::iterator iter;
            for(iter = locations.begin() ; iter != locations.end() ; iter++)
                iter->print(cout, 0);
        } catch (FireEagleException *fex) {
            cerr << "Fire Eagle exception (Message: " << fex->msg << ")" << endl;
        }
    }

    return response;
}

string update(FireEagle &fe, const FE_ParamPairs &args, enum FE_format format) {
    string response = fe.update(args, format);
    cout << "Update response: " << response << endl;

    return response;
}

string within(FireEagle &fe, const FE_ParamPairs &args, enum FE_format format) {
    string response = fe.within(args, format);
    cout << "Within response: " << response << endl;

    return response;
}

string recent(FireEagle &fe, const FE_ParamPairs &args, enum FE_format format) {
    string response = fe.recent(args, format);
    cout << "Recent response: " << response << endl;

    return response;
}

FE_ParamPairs get_args(int idx, int argc, char *argv[]) {
    //Parse the arguments...
    FE_ParamPairs args;

    for (int i = idx + 1 ; (i < argc) && (strncmp(argv[i], "--", 2) != 0) ; i++) {
        char *c = strchr(argv[i], '=');
        if (!c) {
            cerr << "Invalid argument: '" << argv[i] << "'" << endl;
            args.clear();
            break;
        }

        *c = 0;
        c++;
        if ((strlen(argv[i]) == 0) || (strlen(c) == 0)) {
            cerr << "Invalid argument: '" << argv[i] << "=" << c << "'" << endl;
            args.clear();
            break;
        }

        args[argv[i]] = c;
    }

    return args;
}

class XMLParserData : public ParserData {
  public:
    ~XMLParserData() {}

    virtual enum FE_format lang() const { return FE_FORMAT_XML; }

    /**
     * Get an instance of the actual parser object.
     * @return A pointer to a derived class for a FE_Parser
     */
    virtual FE_Parser *parser_instance() const { return new FE_XMLParser; }
};

int main(int argc, char *argv[]) {
    if (argc == 1) {
        usage();
        return 0;
    }

    string app_tok_file;
    string general_tok_file;
    string fe_conf;
    string save_fe_conf;
    string tok_file;
    string save_file;
    string token_str;
    string secret_str;
    enum FE_format format = FE_FORMAT_XML;
    enum FE_oauth_version oauth_version = OAUTH_10A;
    bool do_debug = false;
    string base_url;
    int i = 1;
    int idx = -1; //Command.
    bool make_request = true;
    bool oauth_header = false;
    while (i < argc) {
        if (strcmp(argv[i], "--help") == 0) {
            usage();
            return 0;
        } else if (strcmp(argv[i], "--fake-request") == 0) {
            make_request = false;
            i++;
        } else if (strcmp(argv[i], "--oauth-header") == 0) {
            oauth_header = true;
            i++;
        } else if (strcmp(argv[i], "--oauth-version") == 0) {
            if (i == (argc - 1)) {
                cerr << "Option --fe-config must be followed by a filename." << endl;
                return 0;
            }
            if (strcmp(argv[i + 1], "1.0") == 0) {
                oauth_version = OAUTH_10;
            } else if (strcmp(argv[i + 1], "1.0a") == 0) {
                oauth_version = OAUTH_10A;
            } else {
                cerr << "Invalid value " << argv[i + 1] << " for --oauth-version" << endl;
                return 0;
            }
            i += 2;
        } else if (strcmp(argv[i], "--fe-config") == 0) {
            if (i == (argc - 1)) {
                cerr << "Option --fe-config must be followed by a filename." << endl;
                return 0;
            }
            fe_conf = string(argv[i + 1]);
            i += 2;
        } else if (strcmp(argv[i], "--save-fe-config") == 0) {
            if (i == (argc - 1)) {
                cerr << "Option --save-fe-config must be followed by a filename." << endl;
                return 0;
            }
            save_fe_conf = string(argv[i + 1]);
            i += 2;
        } else if (strcmp(argv[i], "--app-token-file") == 0) {
            if (i == (argc - 1)) {
                cerr << "Option --app-token-file must be followed by a filename." << endl;
                return 0;
            }
            app_tok_file.append(argv[i + 1]);
            i += 2;
        } else if (strcmp(argv[i], "--general-token-file") == 0) {
            if (i == (argc - 1)) {
                cerr << "Option --general-token-file must be followed by a filename." << endl;
                return 0;
            }
            general_tok_file.append(argv[i + 1]);
            i += 2;
        } else if (strcmp(argv[i], "--token-file") == 0) {
            if (i == (argc - 1)) {
                cerr << "Option --token-file must be followed by a filename." << endl;
                return 0;
            }
            tok_file.append(argv[i + 1]);
            i += 2;
        } else if (strcmp(argv[i], "--token") == 0) {
            if (i == (argc - 2)) {
                cerr << "Option --token must be followed by a token and a secret" << endl;
                return 0;
            }
            token_str = string(argv[i + 1]);
            secret_str = string(argv[i + 2]);
            i += 3;
        } else if (strcmp(argv[i], "--out-token") == 0) {
            if (i == (argc - 1)) {
                cerr << "Option --out-token must be followed by a filename." << endl;
                return 0;
            }
            save_file.append(argv[i + 1]);
            i += 2;
        } else if (strcmp(argv[i], "--fe-root") == 0) {
            if (i == (argc - 1)) {
                cerr << "Option --fe-root must be followed by a filename." << endl;
                return 0;
            }
            base_url = string(argv[i + 1]);
            i += 2;
        } else if (strcmp(argv[i], "--format") == 0) {
            if (i == (argc - 1)) {
                cerr << "Option --format must be followed by a valid format." << endl;
                return 0;
            }
            if (strcmp(argv[i + 1], "xml") == 0)
                format = FE_FORMAT_XML;
            else if (strcmp(argv[i + 1], "json") == 0)
                format = FE_FORMAT_JSON;
            else if (strcmp(argv[i + 1], "html") == 0)
                format = FE_FORMAT_HTML;
            else {
                cerr << argv[i + 1] << "is not a valid value for --format." << endl;
                return 0;
            }
            i += 2;
        } else if (strcmp(argv[i], "--debug") == 0) {
            do_debug = true;
            i++;
        } else if (strcmp(argv[i], "--https-noverify") == 0) {
            MyFireEagleCurlAgent::https_noverify = true;
            i++;
        } else if ((idx == -1) && (strncmp(argv[i], "--", 2) == 0)) {
            idx = i;
            i++;
        } else
            i++;
    }

    if (idx == -1) {
        cerr << "No command specified." << endl;
        usage();
        return 0;
    }

    if (fe_conf.length() > 0) {
        fe_config = new FireEagleConfig(fe_conf);
    } else {
        OAuthTokenPair consumer(app_tok_file);
        if (!consumer.is_valid()) {
            cerr << "You must specify the app tokens to invoke this program." << endl;
            usage();
        }
        if (general_tok_file.length() > 0) {
            OAuthTokenPair general_token(general_tok_file);
            fe_config = new FireEagleConfig(consumer, general_token);
        } else
            fe_config = new FireEagleConfig(consumer);
    }

    assert(fe_config);
    fe_config->FE_DEBUG = do_debug;
    fe_config->FE_DUMP_REQUESTS = do_debug;
    fe_config->FE_OAUTH_VERSION = oauth_version;
    fe_config->FE_USE_OAUTH_HEADER = oauth_header;
    if (base_url.length() > 0) {
        fe_config->FE_ROOT = base_url;
        fe_config->FE_API_ROOT = base_url;
    }

    if (save_fe_conf.length() > 0)
        fe_config->save(save_fe_conf);

    fe_config->register_parser("application/xml", new XMLParserData);

    OAuthTokenPair oauth_tok("", ""); //Can be request or access token.
    if ((token_str.length() > 0) && (secret_str.length() > 0))
        oauth_tok = OAuthTokenPair(token_str, secret_str);
    else if (tok_file.length() > 0)
        oauth_tok = OAuthTokenPair(tok_file);

    if (make_request && (format == FE_FORMAT_HTML)) {
        cerr << "HTML is not an actually supported API response format for actual requests" << endl;
        return 0;
    }

    //Have a common place where all the args for the command can be extracted.
    FE_ParamPairs args = get_args(idx, argc, argv);

    try {
        if (strcmp(argv[idx], "--get_request_tok") == 0) {
            MyFireEagle fe(fe_config);
            fe.dummy_request = !make_request;
            OAuthTokenPair tok = request_token(fe, args);
            if (save_file.length() > 0)
                tok.save(save_file);
        } else if (strcmp(argv[idx], "--get_authorize_url") == 0) {
            MyFireEagle fe(fe_config);
            fe.dummy_request = !make_request;
            if (oauth_tok.token.length() == 0) {
                cout << "Generating request .... " << endl;
                oauth_tok = request_token(fe, args);
                if (save_file.length() > 0)
                    oauth_tok.save(save_file);
            }
            authorize_url(fe, oauth_tok, args);
        } else if (strcmp(argv[idx], "--get_access_token") == 0) {
            if (!oauth_tok.is_valid()) {
                cout << "You must provide the request token and secret. Run with --get_authorize_url to generate the tokens and to access the generated URL before this step" << endl;
                return 0;
            }
            string verifier;
            if (oauth_version == OAUTH_10A) {
                FE_ParamPairs::iterator iter = args.find("oauth_verifier");
                if (iter == args.end()) {
                    cout << "oauth_verifier is needed for oauth version = 1.0a" << endl;
                    return 0;
                }
                verifier = iter->second;
            }
            MyFireEagle fe(fe_config, oauth_tok);
            fe.dummy_request = !make_request;
            OAuthTokenPair access = access_token(fe, verifier);
            if (save_file.length() > 0)
                access.save(save_file);
        } else if (strcmp(argv[idx], "--get_location") == 0) {
            if (!oauth_tok.is_valid()) {
                cout << "You must provide the access token and secret. Run with --get_access_token to generate the tokens before this step" << endl;
                return 0;
            }
            MyFireEagle fe(fe_config, oauth_tok);
            fe.dummy_request = !make_request;
            location(fe, format);
        } else if (strcmp(argv[idx], "--lookup") == 0) {
            if (!oauth_tok.is_valid()) {
                cout << "You must provide the access token and secret. Run with --get_access_token to generate the tokens before this step" << endl;
                return 0;
            }
            MyFireEagle fe(fe_config, oauth_tok);
            fe.dummy_request = !make_request;

            if (args.size() == 0) {
                cerr << "Please provide some arguments in the <name>=<value> form" << endl;
                return 0;
            }
            lookup(fe, args, format);
        } else if (strcmp(argv[idx], "--update") == 0) {
            if (!oauth_tok.is_valid()) {
                cout << "You must provide the access token and secret. Run with --get_access_token to generate the tokens before this step" << endl;
                return 0;
            }
            MyFireEagle fe(fe_config, oauth_tok);
            fe.dummy_request = !make_request;

            if (args.size() == 0) {
                cerr << "Please provide some arguments in the <name>=<value> form" << endl;
                return 0;
            }
            update(fe, args, format);
        } else if (strcmp(argv[idx], "--within") == 0) {
            if (!fe_config->get_general_token()) {
                cout << "You must provide the general token and secret." << endl;
                return 0;
            }
            MyFireEagle fe(fe_config);
            fe.dummy_request = !make_request;

            if (args.size() == 0) {
                cerr << "Please provide some arguments in the <name>=<value> form" << endl;
                return 0;
            }
            within(fe, args, format);
        } else if (strcmp(argv[idx], "--recent") == 0) {
            if (!fe_config->get_general_token()) {
                cout << "You must provide the general token and secret." << endl;
                return 0;
            }
            MyFireEagle fe(fe_config);
            fe.dummy_request = !make_request;

            recent(fe, args, format);
        } else {
            usage();
        }
    } catch (FireEagleException *e) {
        cerr << "Caught a FEException" << endl;
        cerr << "Message: '" << e->msg << "' , Code: " << e->code << endl;
        if (e->response.length() > 0)
            cerr << "Response body: " << e->response << endl;
    }

    return 0;
}
