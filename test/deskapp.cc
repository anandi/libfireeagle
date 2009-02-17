/**
 * FireEagle OAuth+API C++ bindings
 *
 * Copyright (C) 2009 Yahoo! Inc
 *
 */
#include <iostream>

#include <string.h>
#include "fireeagle.h"

#define CONSUMER_KEY "UkaPZ38Hehbi"
#define CONSUMER_SECRET "qAvHI2KEM4EsbkNf5rYrDxq0CNdLaCjq"

void usage() {
    cout << "Arguments:" << endl;
    cout << "\t--get_request_tok" << endl;
    cout << "\t--get_authorize_url [<request_token> <request_secret>]" << endl;
    cout << "\t--get_access_token <request_token> <request_secret>" << endl;
    cout << "\t--get_location <request_token> <request_secret> [json]" << endl;
    cout << "\t--lookup <request_token> <request_secret> <name>=<value> [<name>=<value>,[...]] [json]" << endl;
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

int main(int argc, char *argv[]) {
//    FireEagle::FE_DUMP_REQUESTS = true;
//    FireEagle::FE_DEBUG = true;

    if (argc == 1) {
        usage();
        return 0;
    }

    try {
        if (strcmp(argv[1], "--help") == 0) {
            usage();
        } else if (strcmp(argv[1], "--get_request_tok") == 0) {
            FireEagle fe(CONSUMER_KEY, CONSUMER_SECRET);
            request_token(fe);
        } else if (strcmp(argv[1], "--get_authorize_url") == 0) {
            FireEagle fe(CONSUMER_KEY, CONSUMER_SECRET);
            if (argc == 4) {
                OAuthTokenPair request(argv[2], argv[3]);
                authorize_url(fe, request);
            } else {
                cout << "Generating request .... " << endl;
                OAuthTokenPair request = request_token(fe);
                authorize_url(fe, request);
            }
        } else if (strcmp(argv[1], "--get_access_token") == 0) {
            if (argc < 4) {
                cout << "You must provide the request token and secret. Run with --get_authorize_url to generate the tokens and to access the generated URL before this step" << endl;
                return 0;
            }
            FireEagle fe(CONSUMER_KEY, CONSUMER_SECRET, argv[2], argv[3]);
            access_token(fe);
        } else if (strcmp(argv[1], "--get_location") == 0) {
            if (argc < 4) {
                cout << "You must provide the request token and secret. Run with --get_authorize_url to generate the tokens and to access the generated URL before this step" << endl;
                return 0;
            }
            FireEagle fe(CONSUMER_KEY, CONSUMER_SECRET, argv[2], argv[3]);
            location(fe, (argc > 4)? argv[4] : "");
        } else if (strcmp(argv[1], "--lookup") == 0) {
            if (argc < 4) {
                cout << "You must provide the request token and secret. Run with --get_authorize_url to generate the tokens and to access the generated URL before this step" << endl;
                return 0;
            }

            //Parse the arguments...
            FE_ParamPairs args;
            bool json = false; //Have to pick out when parsing other args.
            int argcount = 0;

            for (int i = 4 ; i < argc ; i++) {
                if (strcmp(argv[i], "json") == 0) {
                    json = true;
                    continue;
                }

                char *c = strchr(argv[i], '=');
                if (!c) {
                    cout << "Invalid argument: '" << argv[i] << "'" << endl;
                    return 0;
                }

                *c = 0;
                c++;
                if ((strlen(argv[i]) == 0) || (strlen(c) == 0)) {
                    cout << "Invalid argument: '" << argv[i] << "=" << c << "'" << endl;
                    return 0;
                }

                args[argv[i]] = c;
                argcount++;
            }

            if (argcount == 0) {
                cout << "Please provide some arguments in the <name>=<value> form" << endl;
                return 0;
            }

            FireEagle fe(CONSUMER_KEY, CONSUMER_SECRET, argv[2], argv[3]);
            lookup(fe, args, (json) ? "json" : "");
        } else if (strcmp(argv[1], "--update") == 0) {
            if (argc < 4) {
                cout << "You must provide the request token and secret. Run with --get_authorize_url to generate the tokens and to access the generated URL before this step" << endl;
                return 0;
            }

            //Parse the arguments...
            FE_ParamPairs args;
            bool json = false; //Have to pick out when parsing other args.
            int argcount = 0;

            for (int i = 4 ; i < argc ; i++) {
                if (strcmp(argv[i], "json") == 0) {
                    json = true;
                    continue;
                }

                char *c = strchr(argv[i], '=');
                if (!c) {
                    cout << "Invalid argument: '" << argv[i] << "'" << endl;
                    return 0;
                }

                *c = 0;
                c++;
                if ((strlen(argv[i]) == 0) || (strlen(c) == 0)) {
                    cout << "Invalid argument: '" << argv[i] << "=" << c << "'" << endl;
                    return 0;
                }

                args[argv[i]] = c;
                argcount++;
            }

            if (argcount == 0) {
                cout << "Please provide some arguments in the <name>=<value> form" << endl;
                return 0;
            }

            FireEagle fe(CONSUMER_KEY, CONSUMER_SECRET, argv[2], argv[3]);
            update(fe, args, (json) ? "json" : "");
        } else {
            usage();
        }
    } catch (FireEagleException *e) {
        cerr << "Caught a FEException" << endl;
    }

    return 0;
}
