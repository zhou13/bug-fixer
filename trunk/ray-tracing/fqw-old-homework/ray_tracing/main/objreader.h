#ifndef H_OBJREADER
#define H_OBJREADER

#include "basic.h"
#include "model.h"
#include <string>
#include <map>
using namespace std;

class ObjReader {
    map<string, Material*> mtl_hash;
    void LoadMtlFile(const char *filename);
    void TranslatePoint(Point &p);
public:
    void LoadModel(Model *model, const char *filename, const Point &center, const double &factor, const double &angle);
};

#endif
