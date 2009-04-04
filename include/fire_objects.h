/**
 * FireEagle OAuth+API C++ bindings
 *
 * Copyright (C) 2009 Yahoo! Inc
 *
 */

#ifndef FIRE_OBJECTS_H
#define FIRE_OBJECTS_H

#include <string>
#include <list>

#include "fireeagle.h" //For FireEagleConfig

using namespace std;

/**
 * enum for supported geometries for areas in location responses.
 */
enum FE_geometry_type {
    /** Not a valid geometry */
    FEGeo_INVALID = 0,
    /** Exact point */
    FEGeo_POINT,
    /** A bounding box */
    FEGeo_BOX,
    /** A polygon defined by a sequence of lines. Not implemented */
    FEGeo_POLYGON
};

/**
 * Base class for all geometries. Basically a parsed structure for
 * response received.
 */
class FE_geometry {
  public:
    /** Type of the derived class */
    enum FE_geometry_type type;
    /** Center of the bounding box in case of an area or polygon.
     * Initialized to 0.
     */
    double latitude;
    /** Center of the bounding box in case of an area or polygon.
     * Initialized to 0.
     */
    double longitude;

    /**
     * @param type Each derived class is expected to supply the correct
     * type.
     */
    FE_geometry(FE_geometry_type type = FEGeo_INVALID);

    /** Method for debugging. Override for your own style.
     * @param os Any output stream.
     * @param indent Indentation level. Used for pretty printing.
     */
    virtual void print(ostream &os, unsigned int indent = 0) const;
};

/** Implementation of a point location. Basically the same as the base
 * class.
 */
class FEGeo_Point : public FE_geometry {
  public:
    FEGeo_Point();
    virtual void print(ostream &os, unsigned int indent = 0) const;
};

/**Implementation of a bounding box location. */
class FEGeo_Box : public FE_geometry {
  public:
    /** Minimum latitude of the box. Initialized to 0 */
    double min_lat;
    /** Minimum longitude of the box. Initialized to 0 */
    double min_lon;
    /** Maximum latitude of the box. Initialized to 0 */
    double max_lat;
    /** Maximum longitude of the box. Initialized to 0 */
    double max_lon;

    FEGeo_Box();
    virtual void print(ostream &os, unsigned int indent = 0) const;
};

/** A parsed structure for a location. Note that all member variables
 * may not be available in all responses. Check default values. All strings are
 * initialized to empty strings unless specified otherwise.
 */
class FE_location {
  public:
    /** The actual geometry for the location. Initialized to an invalid geometry */
    FE_geometry geometry;
    /** See documentation on Fire Eagle's 'user' method. Initialized to false */
    bool best_guess;
    /** Not quite clear what this means. Initialized to empty string. */
    string label;
    /** Location level. Ranges from 0 (exact) to 8 (continent). Initialized to UINT_MAX */
    unsigned int level;
    /** Name of the level, in English. */
    string level_name;
    /** Timestamp of update of location at the current level as recorded by Fire Eagle.
     * The reason for the legalese is that your country level can still not get updated
     * if you are going from one city to another city in the same country.
     * Not sure if this includes the TZ.
     * Would have preferred a UTC */
    string timestamp;
    /** The complete location string including less granular levels. */
    string full_location;
    /** Name of the location object at current level. For example, if the level is a 'city',
     * then only the name of the city. */
    string place_name;
    /** A 'Flickr' (http://www.flickr.com) defined ID for the location. */
    string place_id;
    /** To denote whether the specified place_id is an exact or an approximate match
     * for the location. Initialized to false. */
    bool is_place_id_exact;
    /** A 'Where On Earth' ID (http://developer.yahoo.com/geo/) for the location.
     * Initialized to UINT_MAX to indicate undefined. */
    unsigned int woeid;
    /** To denote whether the specified woeid is an exact or an approximate match
     * for the location */
    bool is_woeid_exact;

    FE_location();

    /** Method for debugging. Override for your own style.
     * @param os Any output stream.
     * @param indent Indentation level. Used for pretty printing.
     */
    virtual void print(ostream &os, unsigned int indent = 0) const;

    /** Factory method to parse API responses according to content type. Throws
     * exception if the content_type is not handled in config.
     * @param resp The actual response body to be parsed.
     * @param content_type The content type of the response.
     * @param config Pointer to the FireEagleConfig with which the parsers are
     * registered.
     * @return list of valid instances.
     */
    static list<FE_location> from_response(const string &resp,
                                           enum FE_format format, 
                                           FireEagleConfig *config);
};

/** Class representing a parsed response from the 'user' APi of Fire Eagle.
 * Remember that most of the additional info is retrieved to represent the
 * current permissions given by the user (things can change behind the
 * application's back) to the application calling the API.
 */
class FE_user {
  public:
    /** Whether the application has permission to read this user's location currently */
    bool can_read;
    /** Whether the application has permission to update this user's location currently */
    bool can_write;
    /** OAuth token identifying the user (in case you fired a bunch of calls in different
     * threads) */
    string token;
    /** Timestamp of update of any location level, as recorded by Fire Eagle.
     * Would have preferred a UTC */
    string last_update_timestamp;
    /** Timezone for all timestamps in the response. */
    string timezone;
    /** A '|' delimited list of WOE IDs (http://developer.yahoo.com/geo/) from the
     * coarsest location level to the finest (left to right) */
    string woeid_hierarchy;
    /** List representing the location hierarchy of the user's last known location.
     * Note that, even if Fire Eagle may be knowing of the actual location down to the
     * exact level, privacy settings by the user may prevent the API caller to retrieve
     * the more detailed levels in the location hierarchy. */
    list<FE_location> location;    

    FE_user();

    /** Method for debugging. Override for your own style.
     * @param os Any output stream.
     * @param indent Indentation level. Used for pretty printing.
     */
    virtual void print(ostream &os, unsigned int indent = 0) const;

    /** Factory method to parse API responses according to format. Throws
     * exception if the config does not have a parser for the format.
     * @param resp The actual response body to be parsed.
     * @param format The response format.
     * @param config Pointer to the FireEagleConfig with which the parsers are
     * registered.
     * @return Valid instance.
     */
    static FE_user from_response(const string &resp, enum FE_format format, 
                                 FireEagleConfig *config);
};

#endif //FIRE_OBJECTS_H
