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

#include <curl/curl.h>

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

//Extended to use a different HTTP agent (actuall different agent settings)
class MyFireEagle : public FireEagle {
  protected:
    FireEagleHTTPAgent *HTTPAgent(const string &url,
                                  const string &postdata) const {
        return new MyFireEagleCurlAgent(url, postdata);
    }

  public:
    MyFireEagle(FireEagleConfig *_config, const OAuthTokenPair &_token)
        : FireEagle(_config, _token) {}
    MyFireEagle(FireEagleConfig *_config) : FireEagle(_config) {}
};

void usage() {
    cout << "Arguments (common):" << endl;
    cout << "\t--help (Shows this help)" << endl;
    cout << "\t--app-token-file <file> Mandatory arg. (You can set the FE_APPTOKEN environment instead)" << endl;
    cout << "\t--general-token-file <file> General token for within and recent queries" << endl;
    cout << "\t--token-file <file> Request or Access token as needed for operation." << endl;
    cout << "\t--token <token> <secret> Request or Access token as needed for operation. Overrides --token-file" << endl;
    cout << "\t--out-token <file> Optional. Use with commands that generate request or access tokens to save to file." << endl;
    cout << "\t--json Optional with some commands. Returns the data for the API calls as JSON." << endl;
    cout << "\t--fe-root <base URL> Point to the Fire Eagle installation [" << endl;     cout << "\t--debug Enable verbose output" << endl;
    cout << "\t--https-noverify Disable HTTPS peer verification" << endl;
    cout << "\t--save-fe-config <file> Save the Fire Eagle config to a file" << endl;
    cout << "\t--fe-config <file> Load Fire Eagle config from a file. This avoids --app-token-file, --general-token, --fe-root" << endl;
    cout << "\nOAuth Commands: (use --token or --token-file where tokens are needed)" << endl;
    cout << "\t--get_request_tok" << endl;
    cout << "\t--get_authorize_url You can optionally specify a request token." << endl;
    cout << "\t--get_access_token Needs a request token." << endl;
    cout << "\nAPI commands: Always mention the access token throug --token or --token-file." << endl;
    cout << "\t--get_location Needs an access token" << endl;
    cout << "\t--lookup Needs an access token <name>=<value> [<name>=<value>,[...]]" << endl;
    cout << "\t--update Needs an access token <name>=<value> [<name>=<value>,[...]]" << endl;
    cout << "\t--within Needs a general token <name>=<value> [<name>=<value>,[...]]" << endl;
    cout << "\t--recent Needs a general token <name>=<value> [<name>=<value>,[...]]" << endl;
}

OAuthTokenPair request_token(FireEagle &fe) {
    OAuthTokenPair reqTok = fe.request_token();

    cout << "Request Token: " << reqTok.token << endl;
    cout << "Request Secret: " << reqTok.secret << endl;

    return reqTok;
}

string authorize_url(FireEagle &fe, OAuthTokenPair &request) {
    string url = fe.getAuthorizeURL(request);

    cout << "Authorize URL: " << url << endl;
    return url;
}

OAuthTokenPair access_token(FireEagle &fe) {
    OAuthTokenPair accessTok = fe.access_token();

    cout << "Access Token: " << accessTok.token << endl;
    cout << "Access Secret: " << accessTok.secret << endl;

    return accessTok;
}

string location(FireEagle &fe, const string &format) {
    string response;
    if (format == "json") {
        response = fe.user(FE_FORMAT_JSON);
    } else if ((format == "xml") || (format == "")) {
        response = fe.user();
    } else {
        cout << "Invalid format specified. Reverting to default XML..." << endl;
        response = fe.user();
    }

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

string lookup(FireEagle &fe, const FE_ParamPairs &args, const string &format) {
    string response;
    if (format == "json") {
        response = fe.lookup(args, FE_FORMAT_JSON);
    } else if ((format == "xml") || (format == "")) {
        response = fe.lookup(args);
    } else {
        cout << "Invalid format specified. Reverting to default XML..." << endl;
        response = fe.lookup(args);
    }


    if (format == "json") {
        cout << "Lookup response: " << response << endl;
    } else {
        try {
            list<FE_location> locations = fe.lookup_objects(response);
            list<FE_location>::iterator iter;
            for(iter = locations.begin() ; iter != locations.end() ; iter++)
                iter->print(cout, 0);
        } catch (FireEagleException *fex) {
            cerr << "Fire Eagle exception (Message: " << fex->msg << ")" << endl;
        }
    }

    return response;
}

string update(FireEagle &fe, const FE_ParamPairs &args, const string &format) {
    string response;
    if (format == "json") {
        response = fe.update(args, FE_FORMAT_JSON);
    } else if ((format == "xml") || (format == "")) {
        response = fe.update(args);
    } else {
        cout << "Invalid format specified. Reverting to default XML..." << endl;
        response = fe.update(args);
    }

    cout << "Update response: " << response << endl;

    return response;
}

string within(FireEagle &fe, const FE_ParamPairs &args, const string &format) {
    string response;
    if (format == "json") {
        response = fe.within(args, FE_FORMAT_JSON);
    } else if ((format == "xml") || (format == "")) {
        response = fe.within(args);
    } else {
        cout << "Invalid format specified. Reverting to default XML..." << endl;
        response = fe.within(args);
    }

    cout << "Within response: " << response << endl;

    return response;
}

string recent(FireEagle &fe, const FE_ParamPairs &args, const string &format) {
    string response;

    if (format == "json") {
        response = fe.recent(args, FE_FORMAT_JSON);
    } else if ((format == "xml") || (format == "")) {
        response = fe.recent(args);
    } else {
        cout << "Invalid format specified. Reverting to default XML..." << endl;
        response = fe.recent(args);
    }

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
    bool json = false;
    bool do_debug = false;
    string base_url;
    int i = 0;
    int idx = -1; //Command.
    while (i < argc) {
        if (strcmp(argv[i], "--help") == 0) {
            usage();
            return 0;
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
        } else if (strcmp(argv[i], "--json") == 0) {
            json = true;
            i++;
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

    FireEagleConfig *fe_config = NULL;
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
    if (base_url.length() > 0) {
        fe_config->FE_ROOT = base_url;
        fe_config->FE_API_ROOT = base_url;
    }

    if (save_fe_conf.length() > 0)
        fe_config->save(save_fe_conf);

    OAuthTokenPair oauth_tok("", ""); //Can be request or access token.
    if ((token_str.length() > 0) && (secret_str.length() > 0))
        oauth_tok = OAuthTokenPair(token_str, secret_str);
    else if (tok_file.length() > 0)
        oauth_tok = OAuthTokenPair(tok_file);

    try {
        if (strcmp(argv[idx], "--get_request_tok") == 0) {
            MyFireEagle fe(fe_config);
            OAuthTokenPair tok = request_token(fe);
            if (save_file.length() > 0)
                tok.save(save_file);
        } else if (strcmp(argv[idx], "--get_authorize_url") == 0) {
            MyFireEagle fe(fe_config);
            if (oauth_tok.token.length() == 0) {
                cout << "Generating request .... " << endl;
                oauth_tok = request_token(fe);
                if (save_file.length() > 0)
                    oauth_tok.save(save_file);
            }
            authorize_url(fe, oauth_tok);
        } else if (strcmp(argv[idx], "--get_access_token") == 0) {
            if (!oauth_tok.is_valid()) {
                cout << "You must provide the request token and secret. Run with --get_authorize_url to generate the tokens and to access the generated URL before this step" << endl;
                return 0;
            }
            MyFireEagle fe(fe_config, oauth_tok);
            OAuthTokenPair access = access_token(fe);
            if (save_file.length() > 0)
                access.save(save_file);
        } else if (strcmp(argv[idx], "--get_location") == 0) {
            if (!oauth_tok.is_valid()) {
                cout << "You must provide the access token and secret. Run with --get_access_token to generate the tokens before this step" << endl;
                return 0;
            }
            MyFireEagle fe(fe_config, oauth_tok);
            location(fe, (json)? "json" : "");
        } else if (strcmp(argv[idx], "--lookup") == 0) {
            if (!oauth_tok.is_valid()) {
                cout << "You must provide the access token and secret. Run with --get_access_token to generate the tokens before this step" << endl;
                return 0;
            }
            MyFireEagle fe(fe_config, oauth_tok);

            FE_ParamPairs args = get_args(idx, argc, argv);
            if (args.size() == 0) {
                cerr << "Please provide some arguments in the <name>=<value> form" << endl;
                return 0;
            }
            lookup(fe, args, (json) ? "json" : "");
        } else if (strcmp(argv[idx], "--update") == 0) {
            if (!oauth_tok.is_valid()) {
                cout << "You must provide the access token and secret. Run with --get_access_token to generate the tokens before this step" << endl;
                return 0;
            }
            MyFireEagle fe(fe_config, oauth_tok);

            FE_ParamPairs args = get_args(idx, argc, argv);
            if (args.size() == 0) {
                cerr << "Please provide some arguments in the <name>=<value> form" << endl;
                return 0;
            }
            update(fe, args, (json) ? "json" : "");
        } else if (strcmp(argv[idx], "--within") == 0) {
            if (!fe_config->get_general_token()) {
                cout << "You must provide the general token and secret." << endl;
                return 0;
            }
            MyFireEagle fe(fe_config);

            FE_ParamPairs args = get_args(idx, argc, argv);
            if (args.size() == 0) {
                cerr << "Please provide some arguments in the <name>=<value> form" << endl;
                return 0;
            }
            within(fe, args, (json) ? "json" : "");
        } else if (strcmp(argv[idx], "--recent") == 0) {
            if (!fe_config->get_general_token()) {
                cout << "You must provide the general token and secret." << endl;
                return 0;
            }
            MyFireEagle fe(fe_config);

            FE_ParamPairs args = get_args(idx, argc, argv);
            recent(fe, args, (json) ? "json" : "");
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
