#include "model.h"
#include "basic.h"

#include <vector>
using namespace std;

void Model::Clear() {
    lights.clear();
    items.clear();
    environment = Colors(0.0);
} 
void Model::SetEnvironmentLight(const Colors &ce) {
    environment = ce;
}

void Model::AddLight(const Light &light) {
    lights.push_back(light);
}

void Model::AddFaceItem(const Point &p0, const Point &p1, const Point &p2, const Point &vn0, const Point &vn1, const Point &vn2, Material *material) {
  //  printf("add_face_item\n"); fflush(stdout);
    items.push_back(new FaceItem(p0, p1, p2, vn0, vn1, vn2, material));
}

void Model::AddFaceItem(const Point &p0, const Point &p1, const Point &p2, const Point &vn0, const Point &vn1, const Point &vn2, const Point &vt0, const Point &vt1, const Point &vt2, Material *material) {
  //  printf("add_face_item_2\n"); fflush(stdout);
    items.push_back(new FaceItem(p0, p1, p2, vn0, vn1, vn2, vt0, vt1, vt2, material));
}    

void Model::AddSphereItem(const Point &p, const double &r, Material *material) {
   // printf("add_sphere_item\n"); fflush(stdout);
    items.push_back(new SphereItem(p, r, material));
}

void Model::Rebuild() {
    current_moment = 0;
 //  printf("begin build. items.size()=%d\n", items.size()); fflush(stdout);
    root = new TreeNode(items, Cuboid(-1E50, +1E50, -1E50, +1E50, -1E50, +1E50));
  //  printf("finish build. items.size()=%d\n", items.size()); fflush(stdout);
}

Colors Model::GetBaseColors(const Point &point, const Point &normal) {
	Colors colors = Colors(1.0) - environment;
	for (vector<Light>::iterator light = lights.begin(); light != lights.end(); ++ light) {
		double cosA = Dot(normal, light->p - point) / normal.Abs() / (light->p - point).Abs();
		if (cosA < 0)
			continue;
		if (CheckIntersect(point, light->p-point)) 
			continue;
		colors *= Colors(1.0) - (light->c * cosA);
	}
//	printf("GetBaseColors: ret=(%.3lf,%.3lf,%.3lf)\n",(Colors(1.0) - colors).r,(Colors(1.0) - colors).g,(Colors(1.0) - colors).b);
	return Colors(1.0) - colors;
}

bool Model::CheckIntersect(const Point &viewpoint, const Point &direction) {
    return root->CheckIntersect(viewpoint, direction, ++ current_moment);
}

Intersection Model::GetIntersection(const Point &viewpoint, const Point &direction) {
    Intersection isc;
    isc.item = NULL;
    root->GetIntersection(viewpoint, direction, ++ current_moment, &isc);
    return isc;
}
