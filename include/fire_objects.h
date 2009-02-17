/**
 * FireEagle OAuth+API C++ bindings
 *
 * Copyright (C) 2009 Yahoo! Inc
 *
 */

//Structure for location.
enum FE_geometry_type {FEGeo_POINT = 0, FEGeo_BOX, FEGeo_POLYGON };
class FE_geometry {
  public:
    enum FE_geometry_type type;
    double latitude; //Center of the bounding box in case of an area.
    double longitude; //Center of the bounding box in case of an area.
};

class FEGeo_Point extends FE_geometry {};

class FEGeo_Box extends FE_geometry {
  public:
    double min_lat;
    double min_lon;
    double max_lat;
    double max_lon;
};

class FE_location {
  public:
    FE_geometry geometry;
    bool best_guess;
    string label;
    unsigned int level;
    string level_name;
    string timestamp;
    string full_location;
    string place_name;
    string place_id;
    bool is_place_id_exact;
    unsigned int woeid;
    bool is_woeid_exact;
};

class FE_user {
  public:
    bool can_read; //This application can read the user.
    bool can_write; //This application can update the user's location.
    string token; //OAuth token for the user.
    string last_update_timestamp;
    string timezone;
    string woeid_hierarchy;
    list<FE_location> location;    
};

