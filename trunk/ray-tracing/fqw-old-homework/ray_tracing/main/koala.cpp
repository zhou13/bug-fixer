#include "koala.h"
#include "basic.h"
#include "objreader.h"

#include <cstdio>
#include <string>
using namespace std;

void Koala::Initalize(int _width, int _height, const char *szInputFile) {
    width = _width;
    height = _height;
    
    ObjReader *objreader = new ObjReader();
    FILE *inf = fopen(szInputFile, "r");
    
    for (bool first_turn = true; !feof(inf); first_turn = false) {
        char s[999];
        fgets(s, sizeof(s), inf);
        int sl = strlen(s);
        while (sl > 0 && (s[sl-1] == '\n' || s[sl-1] == '\r' || s[sl-1] == ' '))
            -- sl;
        s[sl] = '\0';
        
      //  printf("s=[%s]\n",s);fflush(stdout);
        
        FILE *cf = fopen(s, "r");
        if (cf == NULL)
            continue;
        if (first_turn) {
            Colors c;
            Point p;
            int n;
    
            fscanf(cf, "%lf%lf%lf", &man_viewpoint.x, &man_viewpoint.y, &man_viewpoint.z);
            fscanf(cf, "%lf%lf%lf", &man_direction.x, &man_direction.y, &man_direction.z);
            fscanf(cf, "%lf", &man_viewangle); man_viewangle = man_viewangle * PI;
            
            model.Clear();
            fscanf(cf, "%lf%lf%lf", &c.r, &c.g, &c.b);
            background = c;
            fscanf(cf, "%lf%lf%lf", &c.r, &c.g, &c.b);
            model.SetEnvironmentLight(c);
            fscanf(cf, "%d", &n);
            while (n -- > 0) {
                fscanf(cf, "%lf%lf%lf", &p.x, &p.y, &p.z);
                fscanf(cf, "%lf%lf%lf", &c.r, &c.g, &c.b);
                model.AddLight(Light(p, c));
            }
        } else {
            char objfile[999];
            fgets(objfile, sizeof(objfile), cf);
            int ol = strlen(objfile);
            while (ol >= 1 && objfile[ol-1] == '\n' || objfile[ol-1] == '\r' || objfile[ol-1] == ' ')
                -- ol;
            objfile[ol] = '\0';
        
     //   printf("objfile=[%s]\n",objfile);fflush(stdout);
        
            Point center;
            double factor, angle;
            fscanf(cf, "%lf%lf%lf", &center.x, &center.y, &center.z);
            fscanf(cf, "%lf%lf", &factor, &angle);
            angle = angle / 180.0 * PI;
            objreader -> LoadModel(&model, objfile, center, factor, angle);
        }
        fclose(cf);
    }
    delete objreader;
    fclose(inf);
    
    model.Rebuild();
}

Colors Koala::RayTracing(int depth, double factor, const Point &viewpoint, const Point &direction) {
    Intersection isc = model.GetIntersection(viewpoint, direction);
    if (isc.item == NULL)
        return depth == 1 ? background : Colors(0);
   // return Colors(0,1,0);
    Point normal = isc.item->GetNormal(isc.point, isc.details);
    normal.MakeToUnit();
    Colors ret = factor * isc.item->GetDiffuse(isc.point, isc.details) * model.GetBaseColors(isc.point, normal);
    //*
	if (depth < LIMIT_DEPTH) {   
        double tmp1 = factor * isc.item->material->specular_coef;
        if (tmp1 > LIMIT_FACTOR)
            ret += isc.item->material->specular * RayTracing(depth+1, tmp1, isc.point, direction-2*Dot(direction, normal)*normal);
            
        double tmp2 = factor * isc.item->material->transparent;
        if (tmp2 > LIMIT_FACTOR)
            ret += RayTracing(depth+1, tmp2, isc.point, direction); 
    }//*/
    return ret;
}

int Koala::GetPixel(int x, int y) {
 //printf("Koala::GetPixel (%d,%d)\n",x,y);fflush(stdout);
    Point viewpoint = man_viewpoint;
    Point direction = man_direction;
    double angle_lr = ((double)x / (double)width - 0.5) * man_viewangle;
    double angle_ud = ((double)y / (double)height - 0.5) * man_viewangle * (double)height / (double)width;
	rotate_2D(&direction.x, &direction.y, -angle_lr);
	double tmp = sqrt(sqr(direction.x) + sqr(direction.y)), new_tmp = tmp;
	if (abs(tmp) < EPSI_DIV)
	   throw string("Koala::GetPixel(): |tmp| = 0");
	rotate_2D(&new_tmp, &direction.z, -angle_ud);
	direction.x *= new_tmp / tmp;
	direction.y *= new_tmp / tmp;
	
	Colors c = RayTracing(1, 1.0, viewpoint, direction);
	c = 1-(1-c)*(1-c);
	int R = min(255, max(0, (int)(c.r * 256.0)));
	int G = min(255, max(0, (int)(c.g * 256.0)));
	int B = min(255, max(0, (int)(c.b * 256.0)));	
    return (R << 16) + (G << 8) + B;
}
