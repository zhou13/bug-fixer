#ifndef H_KOALA
#define H_KOALA

#include "basic.h"
#include "model.h"

class Koala {
    Point man_viewpoint, man_direction;
    double man_viewangle;
    Colors background;
    Model model;
    int width, height;
public:    
    void Initalize(int _width, int _height, const char *szInputFile);
    Colors RayTracing(int depth, double factor, const Point &viewpoint, const Point &direction);
    int GetPixel(int x, int y);
};

#endif
