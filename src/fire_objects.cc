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
#include "fire_parser.h"
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

static FE_geometry geometryFactory(const FEXMLNode *root) { //Do not free up root!
                                                      //Expect the root to be a georss:<something>
    if (root->element() == "georss:point") {
        FEGeo_Point fpoint;

        list<double> items = parseGeoStr(root->text());
        if (items.size() != 2) {
            string message = "Invalid text for georss:point : ";
            message.append(root->text());
            throw new FireEagleException(message, FE_INTERNAL_ERROR);
        }

        fpoint.type = FEGeo_POINT;
        list<double>::iterator iter = items.begin();
        fpoint.latitude = *(iter);
        iter++;
        fpoint.longitude = *(iter);

        return fpoint;
    } else if (root->element() == "georss:box") {
        FEGeo_Box fbox;

        list<double> items = parseGeoStr(root->text());
        if (items.size() != 4) {
            string message = "Invalid text for georss:box : ";
            message.append(root->text());
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
        message.append(root->element());
        throw new FireEagleException(message, FE_INTERNAL_ERROR);
    }
}

static FE_location locationFactory(const FEXMLNode *root) {//Do not free root!
    if (root->element() != "location") {
        //Not handling georss:polygon right now!
        string message("Expected element = location. Got: ");
        message.append(root->element());
        throw new FireEagleException(message, FE_INTERNAL_ERROR);
    }

    FE_location location;
    int nchildren = root->child_count();
    for (int i = 0 ; i < nchildren ; i++) {
        const FEXMLNode &child = root->child(i);

        if (child.element() == "label")
            location.label = child.text();
        else if (child.element() == "level")
            location.level = strtoul(child.text().c_str(), NULL, 10);
        else if (child.element() == "level-name")
            location.level_name = child.text();
        else if (child.element().substr(0, 7) == "georss:")
            location.geometry = geometryFactory(&child);
        else if (child.element() == "located-at")
            location.timestamp = child.text();
        else if (child.element() == "name")
            location.full_location = child.text();
        else if (child.element() == "normal-name")
            location.place_name = child.text();
        else if (child.element() == "place-id") {
            location.place_id = child.text();
            if (child.attribute("exact-match") == "true")
                location.is_place_id_exact = true;
        } else if (child.element() == "woeid") {
            location.woeid = strtoul(child.text().c_str(), NULL, 10);
            if (child.attribute("exact-match") == "true")
                location.is_woeid_exact = true;
        }
    }

    if (root->attribute("best-guess") == "true")
        location.best_guess = true;

    return location;
}

FE_user userFactory(const FEXMLNode *root) {//Do not free root!
    if (root->element() != "user") {
        //Not handling georss:polygon right now!
        string message("Expected element = user. Got: ");
        message.append(root->element());
        throw new FireEagleException(message, FE_INTERNAL_ERROR);
    }

    FE_user user;
    int nchildren = root->child_count();
    if (root->attribute("located-at").length() > 0)
        user.last_update_timestamp = root->attribute("located-at");
    if (root->attribute("readable") == "true")
        user.can_read = true;
    if (root->attribute("writable") == "true")
        user.can_write = true;
    if (root->attribute("token").length() > 0)
        user.token = root->attribute("token");
    
    for (int i = 0 ; i < nchildren ; i++) {
        const FEXMLNode &child = root->child(i);

        if (child.element() != "location-hierarchy")
            continue;

        user.woeid_hierarchy = child.attribute("string");
        user.timezone = child.attribute("timezone");

        int grandchildren = child.child_count();
        for (int j = 0 ; j < grandchildren ; j++) {
            const FEXMLNode &gchild = child.child(j);
            if (gchild.element() == "location")
                user.location.push_back(locationFactory(&gchild));
        }
    }

    return user;
}

list<FE_location> lookupFactory(const FEXMLNode *root) {//Do not free root!
    list<FE_location> locations;

    if (root->element() != "locations") {
        string message("Expected element = locations. Got: ");
        message.append(root->element());
        throw new FireEagleException(message, FE_INTERNAL_ERROR);
    }

    int nchildren = root->child_count();
    for (int i = 0 ; i < nchildren ; i++)
        locations.push_back(locationFactory(&(root->child(i))));

    return locations;
}


















































