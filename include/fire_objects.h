#ifndef FIRE_OBJECTS_H
#define FIRE_OBJECTS_H

#include <string>
#include <list>

using namespace std;

//Structure for location.
enum FE_geometry_type {FEGeo_INVALID = 0, FEGeo_POINT, FEGeo_BOX, FEGeo_POLYGON };

class FE_geometry {
  public:
    enum FE_geometry_type type;
    double latitude; //Center of the bounding box in case of an area.
    double longitude; //Center of the bounding box in case of an area.

    FE_geometry(FE_geometry_type type = FEGeo_INVALID);
    virtual void print(ostream &os, unsigned int indent = 0) const;
};

class FEGeo_Point : public FE_geometry {
  public:
    FEGeo_Point();
    virtual void print(ostream &os, unsigned int indent = 0) const;
};

class FEGeo_Box : public FE_geometry {
  public:
    double min_lat;
    double min_lon;
    double max_lat;
    double max_lon;

    FEGeo_Box();
    virtual void print(ostream &os, unsigned int indent = 0) const;
};

class FE_location {
  public:
    FE_geometry geometry;
    bool best_guess;
    string label;
    unsigned int level; //UINT_MAX indicates undefined.
    string level_name;
    string timestamp;
    string full_location;
    string place_name;
    string place_id;
    bool is_place_id_exact;
    unsigned int woeid; //UINT_MAX indicates undefined.
    bool is_woeid_exact;

    FE_location();
    virtual void print(ostream &os, unsigned int indent = 0) const;
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

    FE_user();
    virtual void print(ostream &os, unsigned int indent = 0) const;
};

#endif //FIRE_OBJECTS_H
