#include "model.h"
#include "objreader.h"
//#include <fstream>
#include <sstream>
using namespace std;

void ObjReader::LoadMtlFile(const char *filename) {
    Material *mtl = NULL;
    
    FILE *inf = fopen(filename, "r");
    if (inf == NULL)
        throw string("ObjReader::LoadMtlFile(): Cannot open input file!");
    string path(filename);
    while (path.size() > 0 && path[path.size()-1] != '\\' && path[path.size()-1] != '/')
        path.erase(path.size()-1);
    while (!feof(inf)) {
        char buf[999];
        fgets(buf, sizeof(buf), inf);
        istringstream is(buf);
        string type;
        if (!(is >> type))
            continue;
        if (type == "newmtl") {
            string mtlname;
            is >> mtlname;
            mtl = new Material();
            mtl_hash[mtlname] = mtl;
        }
        if (type == "Ns") {
            if (mtl != NULL) {
                is >> mtl->specular_coef;
                mtl->specular_coef /= 1000.0;
            }
        }
        if (type == "Kd") {
            if (mtl != NULL)
                is >> mtl->diffuse.r >> mtl->diffuse.g >> mtl->diffuse.b;
        }
        if (type == "Ks") {
            if (mtl != NULL)
                is >> mtl->specular.r >> mtl->specular.g >> mtl->specular.b;
        }
        if (type == "d" || type == "Tr") {
            if (mtl != NULL) {
                is >> mtl->transparent;
                mtl->transparent = 1.0-mtl->transparent;
            }
        }
        if (type == "map_Kd") {
            if (mtl != NULL) {
                string imagefile;
                is >> imagefile;
                mtl->map_diffuse = new Texture((path+imagefile).c_str());
            }
        }
    }
    fclose(inf);
}

void ObjReader::TranslatePoint(Point &p) {
    swap(p.y, p.z);
}

void ObjReader::LoadModel(Model *model, const char *filename, const Point &center, const double &factor, const double &angle) {
//printf("ObjReader::LoadModel filename=%s\n",filename);fflush(stdout);
    vector<Point> v, v_normal, vt, vn;
    Material *cur_mtl = new Material();
    //cur_mtl->diffuse = Colors(0,1,0);
    mtl_hash.clear();

    string path(filename);
    while (path.size() > 0 && path[path.size()-1] != '\\' && path[path.size()-1] != '/')
        path.erase(path.size()-1);
    
    FILE *inf = fopen(filename, "r");
    if (inf == NULL) //{ printf("ObjReader::LoadModel(): Cannot open input file!\n");fflush(stdout);
        throw string("ObjReader::LoadModel(): Cannot open input file!"); //}
    
    Point lower(1E50, 1E50, 1E50), upper = -lower;
    while (!feof(inf)) {
        char buf[999];
        if (fgets(buf, sizeof(buf), inf) == NULL)
            break;
        istringstream is(buf);
        string type;
        if (!(is >> type))
            continue;
        if (type == "v") {
            Point p;
            is >> p.x >> p.y >> p.z;
            TranslatePoint(p);
            lower.x = min(lower.x, p.x);
            lower.y = min(lower.y, p.y);
            lower.z = min(lower.z, p.z);
            upper.x = max(upper.x, p.x);
            upper.y = max(upper.y, p.y);
            upper.z = max(upper.z, p.z);
            v.push_back(p);
            v_normal.push_back(Point(0.0, 0.0, 0.0));
        }
        if (type == "f") {
            vector<int> param[3];
            string s;
            while (is >> s) {
                int ind, i, tmp;
                for (i = 0, ind = 0; ind < 3; ++ ind) {
                    if (i < s.size() && s[i] != '/') {
                        for (tmp = 0; i < s.size() && s[i] != '/'; ++ i)
                            if (s[i] >= '0' && s[i] <= '9')
                                tmp = tmp*10 + (s[i]-'0');
                        param[ind].push_back(tmp-1);
                    }
                    ++ i;
                }
            }
            for (int i = 2; i < param[0].size(); ++ i) {
                int v0, v1, v2;
                v0 = param[0][0], v1 = param[0][i-1], v2 = param[0][i];
                Point normal = Cross(v[v2]-v[v0], v[v1]-v[v0]);
                normal.MakeToUnit();
                v_normal[v0] += normal;
                v_normal[v1] += normal;
                v_normal[v2] += normal;
            }
        }
    }
    fclose(inf);
    
    Point deltaP = -(lower+upper)*0.5;
    for (int i = 0; i < v.size(); ++ i) {
        v[i] = (v[i] + deltaP) * factor;
        rotate_2D(&v[i].x, &v[i].y, angle);
        v[i] += center;
        rotate_2D(&v_normal[i].x, &v_normal[i].y, angle);
    }
        
   // printf("finish scanning\n");
    
    inf = fopen(filename, "r");
    while (!feof(inf)) {
        char buf[999];
        if (fgets(buf, sizeof(buf), inf) == NULL)
            break;
        istringstream is(buf);
        string type;
        if (!(is >> type))
            continue;
      //                  printf("processing s=%s\n",is.str().c_str());fflush(stdout);
        if (type == "mtllib") {
            string libname;
            is >> libname;
            LoadMtlFile((path + libname).c_str());
        }
        if (type == "usemtl") {
            string mtlname;
            is >> mtlname;
           // if (mtlname=="tierra_maceta"){cur_mtl=new Material();cur_mtl->diffuse = Colors(1,0,0);continue;}
            if (mtl_hash.find(mtlname) != mtl_hash.end())// {
                cur_mtl = mtl_hash[mtlname]; 
            //    printf("use_mtl %s\n",mtlname.c_str());}
        }
        if (type == "vn") {
            Point p;
            is >> p.x >> p.y >> p.z;
            TranslatePoint(p);
            rotate_2D(&p.x, &p.y, angle);
            vn.push_back(p);
        }
        if (type == "vt") {
            Point p;
            is >> p.x >> p.y;
            vt.push_back(p);
        }
        if (type == "sphere") {
            int c;
            double r;
            is >> c >> r;
                   //     printf("  - found sphere c=%d r=%.3lf   cur_mtl.d.r=%.3lf\n",c,r,cur_mtl->diffuse.r);fflush(stdout);
            model->AddSphereItem(v[c-1], r * factor, cur_mtl);
        }
        if (type == "f") {
            vector<int> param[3];
            string s;
            while (is >> s) {
                int ind, i, tmp;
                for (i = 0, ind = 0; ind < 3; ++ ind) {
                    if (i < s.size() && s[i] != '/') {
                        for (tmp = 0; i < s.size() && s[i] != '/'; ++ i)
                            if (s[i] >= '0' && s[i] <= '9')
                                tmp = tmp*10 + (s[i]-'0');
                        param[ind].push_back(tmp-1);
                       // printf("<tmp=%d>",tmp);fflush(stdout);
                    }
                    ++ i;
                }
            }
            for (int i = 2; i < param[0].size(); ++ i) {
                int v0, v1, v2, vt0, vt1, vt2, vn0, vn1, vn2;
                bool use_texture = (param[1].size() == param[0].size());
                bool use_normal = (param[2].size() == param[0].size());
                v0 = param[0][0], v1 = param[0][i-1], v2 = param[0][i];
                if (use_texture)
                    vt0 = param[1][0], vt1 = param[1][i-1], vt2 = param[1][i];
                if (use_normal)
                    vn0 = param[2][0], vn1 = param[2][i-1], vn2 = param[2][i];
                if (use_normal)
                    if (use_texture)
                        model->AddFaceItem(v[v0], v[v1], v[v2], vn[vn0], vn[vn1], vn[vn2], vt[vt0], vt[vt1], vt[vt2], cur_mtl);
                    else
                        model->AddFaceItem(v[v0], v[v1], v[v2], vn[vn0], vn[vn1], vn[vn2], cur_mtl);
                else
                    if (use_texture)
                        model->AddFaceItem(v[v0], v[v1], v[v2], v_normal[v0], v_normal[v1], v_normal[v2], vt[vt0], vt[vt1], vt[vt2], cur_mtl);
                    else
                        model->AddFaceItem(v[v0], v[v1], v[v2], v_normal[v0], v_normal[v1], v_normal[v2], cur_mtl);
            }
        }
    }
    fclose(inf);
    
                   //     printf("ObjReader::LoadModel filename=%s   : finished\n",filename);fflush(stdout);
}
