#ifndef H_MODEL
#define H_MODEL

#include "basic.h"
#include <windows.h>
#include <vector>
using namespace std;

enum ItemType {IT_FACE, IT_SPHERE};

struct Light {
    Point p;
    Colors c;
    Light(Point p = Point(), Colors c = Colors()) : p(p), c(c) { }
};

class Texture {
    HBITMAP hbm;
    BITMAP bitmap;
    int *buffer;
    int width, height;
public:
    
    Texture(const char *filename);
    Colors GetPixel(double x, double y);
};

struct IntersectionDetails {
    double alpha, beta;
    bool faced;
};

struct Material {
    Colors diffuse;
    Colors specular;
    double specular_coef;
    double transparent;
    Texture *map_diffuse;
    Material() {
        diffuse = Colors(1);
        specular = Colors(0);
        specular_coef = 0.0;
        transparent = 0.0;
        map_diffuse = NULL;
    }
};

class Item {
public:
    int last_visited;
    Material *material;
    Item() : last_visited(-1) {}    
    virtual pair<double, double> GetMinMaxDimension(int k) {}
    virtual Cuboid GetCuboid() {}
    virtual ItemType GetType() {}
    virtual bool CheckIntersect(const Point &viewpoint, const Point &direction) {}
    virtual bool GetIntersection(const Point &viewpoint, const Point &direction, Point *point, double *t, IntersectionDetails *details) {}
    virtual Point GetNormal(const Point &pnt, const IntersectionDetails &details) {}
    virtual Colors GetDiffuse(const Point &pnt, const IntersectionDetails &details) {}
};

class FaceItem: public Item{
public:
    Point p[3], vn[3], vt[3];
    bool use_texture;
    FaceItem(const Point &p0, const Point &p1, const Point &p2, const Point &vn0, const Point &vn1, const Point &vn2, Material *mat) {
        p[0] = p0, p[1] = p1, p[2] = p2;
        vn[0] = vn0, vn[1] = vn1, vn[2] = vn2;
        material = mat;
        use_texture = false;
    }    
    FaceItem(const Point &p0, const Point &p1, const Point &p2, const Point &vn0, const Point &vn1, const Point &vn2, const Point &vt0, const Point &vt1, const Point &vt2, Material *mat) {
        *this = FaceItem(p0, p1, p2, vn0, vn1, vn2, mat);
        vt[0] = vt0, vt[1] = vt1, vt[2] = vt2;
        use_texture = true;
    }    
    pair<double, double> GetMinMaxDimension(int k);
    Cuboid GetCuboid();
    ItemType GetType();
    bool CheckIntersect(const Point &viewpoint, const Point &direction);
    bool GetIntersection(const Point &viewpoint, const Point &direction, Point *point, double *t, IntersectionDetails *details);
    Point GetNormal(const Point &pnt, const IntersectionDetails &details);
    Colors GetDiffuse(const Point &pnt, const IntersectionDetails &details);
};

class SphereItem: public Item{
public:
    Point o;
    double r;
    SphereItem(const Point &o, const double &r, Material *mat) : o(o), r(r) {
        material = mat;
    }   
    pair<double, double> GetMinMaxDimension(int k);
    Cuboid GetCuboid();
    ItemType GetType();
    bool CheckIntersect(const Point &viewpoint, const Point &direction);
    bool GetIntersection(const Point &viewpoint, const Point &direction, Point *point, double *t, IntersectionDetails *details);
    Point GetNormal(const Point &pnt, const IntersectionDetails &details);
    Colors GetDiffuse(const Point &pnt, const IntersectionDetails &details);
};

struct Intersection {
    Item *item;
    Point point;
    double t;
    IntersectionDetails details;
};

class TreeNode {
public:
    vector<Item*> items;
    TreeNode *child[2];
    int dm;
    double cut;
    Cuboid cuboid;
    
    TreeNode();
    TreeNode(const vector<Item*> items, const Cuboid &space);
    bool CheckIntersect(const Point &viewpoint, const Point &direction, int current_moment);
    void GetIntersection(const Point &viewpoint, const Point &direction, int current_moment, Intersection *isc);
};

class Model {   
    TreeNode *root;
    int current_moment;
    vector<Item*> items;
    vector<Light> lights;
    Colors environment;
public:    
    void Clear();
    void SetEnvironmentLight(const Colors &ce);
    void AddLight(const Light &light);
    void AddFaceItem(const Point &p0, const Point &p1, const Point &p2, const Point &vn0, const Point &vn1, const Point &vn2, Material *material);
    void AddFaceItem(const Point &p0, const Point &p1, const Point &p2, const Point &vn0, const Point &vn1, const Point &vn2, const Point &vt0, const Point &vt1, const Point &vt2, Material *material);
    void AddSphereItem(const Point &p, const double &r, Material *material);
    void Rebuild();
    Colors GetBaseColors(const Point &point, const Point &normal);
    bool CheckIntersect(const Point &viewpoint, const Point &direction);
    Intersection GetIntersection(const Point &viewpoint, const Point &direction);
};

#endif
