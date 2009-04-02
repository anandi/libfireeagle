/**
 * FireEagle OAuth+API C++ bindings
 *
 * Copyright (C) 2009 Yahoo! Inc
 *
 */
#include <sstream>

#include <stdlib.h>
#include <limits.h>

#include "fire_objects.h"
#include "expat_parser.h"
#include "fireeagle.h"

using namespace std;

static void do_indent(ostream &os, unsigned int indent) {
    for (int i = 0 ; i < indent ; i++)
        os << "    ";
}

FE_geometry::FE_geometry(FE_geometry_type _type) {
    type = _type;
    latitude = 0;
    longitude = 0;
}

void FE_geometry::print(ostream &os, unsigned int indent) const {
    if (type == FEGeo_INVALID)
        return;
    do_indent(os, indent); os << "Latitude: " << latitude << endl;
    do_indent(os, indent); os << "Longitude: " << longitude << endl;
}

FEGeo_Point::FEGeo_Point() : FE_geometry(FEGeo_POINT) {}

void FEGeo_Point::print(ostream &os, unsigned int indent) const {
    do_indent(os, indent); os << "Object: " << "Location point" << endl;
    ((FE_geometry *)this)->print(os, indent);
}

FEGeo_Box::FEGeo_Box() : FE_geometry(FEGeo_BOX) {
    min_lat = 0;
    min_lon = 0;
    max_lat = 0;
    max_lon = 0;
}

void FEGeo_Box::print(ostream &os, unsigned int indent) const {
    do_indent(os, indent); os << "Object: " << "Location bounding box" << endl;
    ((FE_geometry *)this)->print(os, indent);
    do_indent(os, indent); os << "Latitude (min): " << min_lat << endl;
    do_indent(os, indent); os << "Longitude (min): " << min_lon << endl;
    do_indent(os, indent); os << "Latitude (max): " << max_lat << endl;
    do_indent(os, indent); os << "Longitude (max): " << max_lon << endl;
}

FE_location::FE_location() {
    best_guess = false;
    level = UINT_MAX;
    is_place_id_exact = false;
    woeid = UINT_MAX;
    is_woeid_exact = false;
}

void FE_location::print(ostream &os, unsigned int indent) const {
    do_indent(os, indent); os << "Object: " << "Location data" << endl;
    geometry.print(os, indent + 1);
    if (best_guess) {
        do_indent(os, indent); os << "Best guess: TRUE" << endl;
    } else {
        do_indent(os, indent); os << "Best guess: FALSE" << endl;
    }
    if (label.length() > 0) {
        do_indent(os, indent); os << "Label: " << label << endl;
    }
    if (level != UINT_MAX) {
        do_indent(os, indent); os << "Level: " << level << endl;
        do_indent(os, indent); os << "Level name: " << level_name << endl;
    }
    if (timestamp.length() > 0) {
        do_indent(os, indent); os << "Timestamp: " << timestamp << endl;
    }
    do_indent(os, indent); os << "Location name: " << full_location << endl;
    do_indent(os, indent); os << "Normal name: " << place_name << endl;
    if (place_name.length() > 0) {
        do_indent(os, indent); os << "Place id: " << place_id << endl;
    }
    if (is_place_id_exact) {
        do_indent(os, indent); os << "Place ID is exact: TRUE" << endl;
    } else {
        do_indent(os, indent); os << "Place ID is exact: FALSE" << endl;
    }
    if (woeid != UINT_MAX) {
        do_indent(os, indent); os << "WOEID: " << woeid << endl;
    }
    if (is_woeid_exact) {
        do_indent(os, indent); os << "WOEID is exact: TRUE" << endl;
    } else {
        do_indent(os, indent); os << "WOEID is exact: FALSE" << endl;
    }
}

FE_user::FE_user() {
    can_read = false;
    can_write = false;
}

void FE_user::print(ostream &os, unsigned int indent) const {
    do_indent(os, indent); os << "Object: " << "User" << endl;
    if (can_read) {
        do_indent(os, indent); os << "Can read: TRUE" << endl;
    } else {
        do_indent(os, indent); os << "Can read: FALSE" << endl;
    }
    if (can_write) {
        do_indent(os, indent); os << "Can write: TRUE" << endl;
    } else {
        do_indent(os, indent); os << "Can write: FALSE" << endl;
    }
    do_indent(os, indent); os << "OAuth token: " << token << endl;
    if (last_update_timestamp.length() > 0) {
        do_indent(os, indent); os << "Update timestamp: " << last_update_timestamp << endl;
    }
    if (timezone.length() > 0) {
        do_indent(os, indent); os << "Timezone: " << timezone << endl;
    }
    if (woeid_hierarchy.length() > 0) {
        do_indent(os, indent); os << "Location hierarchy: " << woeid_hierarchy << endl;
    }

    list<FE_location>::const_iterator iter;
    for (iter = location.begin() ; iter != location.end() ; iter++)
        iter->print(os, indent + 1);
}

static list<double> parseGeoStr(const string &s) {
    list<double> items;

    size_t begin = 0;
    size_t pos = s.find(' ', begin);
    while (pos != string::npos) {
        string item = s.substr(begin, begin - pos);
        begin = pos + 1;
        items.push_back(strtod(item.c_str(), NULL));
        pos = s.find(' ', begin);
    }

    if (begin < s.length()) {
        string item = s.substr(begin, s.length() - begin);
        items.push_back(strtod(item.c_str(), NULL));
    }

    return items;
}

static FE_geometry geometryFactory(const FE_ParsedNode *root) { //Do not free up root!
                                                      //Expect the root to be a georss:<something>
    if (root->name() == "georss:point") {
        FEGeo_Point fpoint;

        list<double> items = parseGeoStr(root->get_string_property("text()"));
        if (items.size() != 2) {
            string message = "Invalid text for georss:point : ";
            message.append(root->get_string_property("text()"));
            throw new FireEagleException(message, FE_INTERNAL_ERROR);
        }

        fpoint.type = FEGeo_POINT;
        list<double>::iterator iter = items.begin();
        fpoint.latitude = *(iter);
        iter++;
        fpoint.longitude = *(iter);

        return fpoint;
    } else if (root->name() == "georss:box") {
        FEGeo_Box fbox;

        list<double> items = parseGeoStr(root->get_string_property("text()"));
        if (items.size() != 4) {
            string message = "Invalid text for georss:box : ";
            message.append(root->get_string_property("text()"));
            throw new FireEagleException(message, FE_INTERNAL_ERROR);
        }

        fbox.type = FEGeo_BOX;
        list<double>::iterator iter = items.begin();
        fbox.min_lat = *(iter);
        iter++;
        fbox.min_lon = *(iter);
        iter++;
        fbox.max_lat = *(iter);
        iter++;
        fbox.max_lon = *(iter);

        fbox.latitude = (fbox.min_lat + fbox.max_lat) / 2;
        fbox.longitude = (fbox.min_lon + fbox.max_lon) / 2;

        return fbox;
    } else {
        //Not handling georss:polygon right now!
        string message("Unhandled geometry: ");
        message.append(root->name());
        throw new FireEagleException(message, FE_INTERNAL_ERROR);
    }
}

static FE_location locationFactory(const FE_ParsedNode *root) {//Do not free root!
    if (root->name() != "location") {
        //Not handling georss:polygon right now!
        string message("Expected element = location. Got: ");
        message.append(root->name());
        throw new FireEagleException(message, FE_INTERNAL_ERROR);
    }

    FE_location location;
    int nchildren = root->child_count();
    for (int i = 0 ; i < nchildren ; i++) {
        const FE_ParsedNode &child = root->child(i);

        if (child.name() == "label")
            location.label = child.get_string_property("text()");
        else if (child.name() == "level")
            location.level = (unsigned long) child.get_long_property("text()");
        else if (child.name() == "level-name")
            location.level_name = child.get_string_property("text()");
        else if (child.name().substr(0, 7) == "georss:")
            location.geometry = geometryFactory(&child);
        else if (child.name() == "located-at")
            location.timestamp = child.get_string_property("text()");
        else if (child.name() == "name")
            location.full_location = child.get_string_property("text()");
        else if (child.name() == "normal-name")
            location.place_name = child.get_string_property("text()");
        else if (child.name() == "place-id") {
            location.place_id = child.get_string_property("text()");
            location.is_place_id_exact = child.get_bool_property("exact-match");
        } else if (child.name() == "woeid") {
            location.woeid = (unsigned long) child.get_long_property("text()");
            location.is_woeid_exact = child.get_bool_property("exact-match");
        }
    }

    location.best_guess = root->get_bool_property("best-guess");

    return location;
}

FE_user userFactory(const FE_ParsedNode *root) {//Do not free root!
    if (root->name() != "user") {
        //Not handling georss:polygon right now!
        string message("Expected element = user. Got: ");
        message.append(root->name());
        throw new FireEagleException(message, FE_INTERNAL_ERROR);
    }

    FE_user user;
    int nchildren = root->child_count();
    if (root->has_property("located-at"))
        user.last_update_timestamp = root->get_string_property("located-at");
    user.can_read = root->get_bool_property("readable");
    user.can_write = root->get_bool_property("writable");
    if (root->has_property("token"))
        user.token = root->get_string_property("token");
    
    for (int i = 0 ; i < nchildren ; i++) {
        const FE_ParsedNode &child = root->child(i);

        if (child.name() != "location-hierarchy")
            continue;

        user.woeid_hierarchy = child.get_string_property("string");
        user.timezone = child.get_string_property("timezone");

        int grandchildren = child.child_count();
        for (int j = 0 ; j < grandchildren ; j++) {
            const FE_ParsedNode &gchild = child.child(j);
            if (gchild.name() == "location")
                user.location.push_back(locationFactory(&gchild));
        }
    }

    return user;
}

extern bool FE_isXMLErrorMsg(const FE_ParsedNode *root, const string &msg);
extern FireEagleException *FE_exceptionFromXML(const FE_ParsedNode *root);
extern bool FE_isJSONErrorMsg(const FE_ParsedNode *root, const string &msg);
extern FireEagleException *FE_exceptionFromJSON(const FE_ParsedNode *root);

FE_user FE_user::from_response(const string &resp, enum FE_format format,
                               FireEagleConfig *config) {
    if (format == FE_FORMAT_JSON) {
        throw new FireEagleException("FE_user::from_response is not implemented for JSON",
                                     FE_INTERNAL_ERROR, resp);
    }

    ParserData *parser_data = config->get_parser(format);
    if (!parser_data) {
        ostringstream os;

        os << "Cannot parse response to make a user object.";
        os << " No registered handler for requested format.";
        throw new FireEagleException(os.str(), FE_INTERNAL_ERROR, resp);
    }

    FE_Parser *parser = parser_data->parser_instance();
    FE_ParsedNode *root = parser->parse(resp);
    if (!root) {
        delete parser;
        throw FireEagleException("Parse failed for response", FE_INTERNAL_ERROR, resp);
    }

    //OK, we parsed. But, is this a valid response?
    if ((format == FE_FORMAT_XML) && FE_isXMLErrorMsg(root, resp)) {
        FireEagleException *e = FE_exceptionFromXML(root);
        delete parser;
        delete root;
        throw e;
    } else if ((format == FE_FORMAT_JSON) && FE_isJSONErrorMsg(root, resp)) {
        FireEagleException *e = FE_exceptionFromJSON(root);
        delete parser;
        delete root;
        throw e;
    }

    try {
        FE_user user;
        if (format == FE_FORMAT_XML) {
            user = userFactory(&(root->child(0)));
        } else if (format == FE_FORMAT_JSON) {
            delete root;
            delete parser;
            throw new FireEagleException("FE_user::from_response is not implemented for JSON",
                                         FE_INTERNAL_ERROR, resp);
        }
        delete root;
        delete parser;
        return user;
    } catch(FireEagleException *fex) {
        delete root;
        delete parser;
        throw fex;
    }    
}

list<FE_location> lookupFactory(const FE_ParsedNode *root) {//Do not free root!
    list<FE_location> locations;

    if (root->name() != "locations") {
        string message("Expected element = locations. Got: ");
        message.append(root->name());
        throw new FireEagleException(message, FE_INTERNAL_ERROR);
    }

    int nchildren = root->child_count();
    for (int i = 0 ; i < nchildren ; i++)
        locations.push_back(locationFactory(&(root->child(i))));

    return locations;
}

list<FE_location> FE_location::from_response(const string &resp,
                                             enum FE_format format, 
                                             FireEagleConfig *config) {
    if (format == FE_FORMAT_JSON) {
        throw new FireEagleException("FE_location::from_response is not implemented for JSON",
                                     FE_INTERNAL_ERROR, resp);
    }

    ParserData *parser_data = config->get_parser(format);
    if (!parser_data) {
        ostringstream os;

        os << "Cannot parse response to make location objects.";
        os << " No registered handler for requested format";
        throw new FireEagleException(os.str(), FE_INTERNAL_ERROR, resp);
    }

    FE_Parser *parser = parser_data->parser_instance();
    FE_ParsedNode *root = parser->parse(resp);
    if (!root) {
        delete parser;
        throw FireEagleException("Parse failed for response", FE_INTERNAL_ERROR, resp);
    }

    //OK, we parsed. But, is this a valid response?
    if ((format == FE_FORMAT_XML) && FE_isXMLErrorMsg(root, resp)) {
        FireEagleException *e = FE_exceptionFromXML(root);
        delete parser;
        delete root;
        throw e;
    } else if ((format == FE_FORMAT_JSON) && FE_isJSONErrorMsg(root, resp)) {
        FireEagleException *e = FE_exceptionFromJSON(root);
        delete parser;
        delete root;
        throw e;
    }


    bool found = false;
    for (int i = 0 ; i < root->child_count() ; i++) {
        if (root->child(i).name() != "locations")
            continue;
        found = true;
        try {
            if (format == FE_FORMAT_XML) {
                list<FE_location> locations = lookupFactory(&(root->child(i)));
                delete root;
                delete parser;
                return locations;
            } else if (format == FE_FORMAT_JSON) {
                delete root;
                delete parser;
                throw new FireEagleException("FE_user::from_response is not implemented for JSON",
                                             FE_INTERNAL_ERROR, resp);
            }
        } catch(FireEagleException *fex) {
            delete root;
            delete parser;
            throw fex;
        }
    }

    delete root;
    delete parser;
    if (!found)
        throw new FireEagleException("Unknown XML response format for lookup API: No locations element present",
                                     FE_INTERNAL_ERROR, resp);
}

