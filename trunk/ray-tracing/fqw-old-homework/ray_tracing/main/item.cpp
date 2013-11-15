#include "model.h"

//// FaceItem ////

ItemType FaceItem::GetType() {
    return IT_FACE;
}

Cuboid FaceItem::GetCuboid() {
    return Cuboid(p[0].x, p[0].y, p[0].z) | Cuboid(p[1].x, p[1].y, p[1].z) | Cuboid(p[2].x, p[2].y, p[2].z);
}

pair<double, double> FaceItem::GetMinMaxDimension(int k) {
    double minv = 1E50, maxv = -1E50;
    for (int i = 0; i < 3; ++ i) {
        double cur = p[i].GetDimension(k);
        minv = min(minv, cur);
        maxv = max(maxv, cur);
    }
    return make_pair(minv, maxv);
}

bool FaceItem::CheckIntersect(const Point &viewpoint, const Point &direction) {
    double a1 = p[0].x-p[2].x, b1 = p[1].x-p[2].x, c1 = -direction.x, d1 = viewpoint.x-p[2].x;
    double a2 = p[0].y-p[2].y, b2 = p[1].y-p[2].y, c2 = -direction.y, d2 = viewpoint.y-p[2].y;
    double a3 = p[0].z-p[2].z, b3 = p[1].z-p[2].z, c3 = -direction.z, d3 = viewpoint.z-p[2].z;
    double alpha, beta, t;
    if (solve_e3(a1, b1, c1, d1, a2, b2, c2, d2, a3, b3, c3, d3, &alpha, &beta, &t))
        if (alpha > -EPSI_CROSS && beta > -EPSI_CROSS && alpha+beta < 1.0+EPSI_CROSS)
            if (t > EPSI_STEP && t < 1.0)
                return true;
    return false;
}

bool FaceItem::GetIntersection(const Point &viewpoint, const Point &direction, Point *point, double *t, IntersectionDetails *details) {
    double a1 = p[0].x-p[2].x, b1 = p[1].x-p[2].x, c1 = -direction.x, d1 = viewpoint.x-p[2].x;
    double a2 = p[0].y-p[2].y, b2 = p[1].y-p[2].y, c2 = -direction.y, d2 = viewpoint.y-p[2].y;
    double a3 = p[0].z-p[2].z, b3 = p[1].z-p[2].z, c3 = -direction.z, d3 = viewpoint.z-p[2].z;
    double alpha, beta;
    if (solve_e3(a1, b1, c1, d1, a2, b2, c2, d2, a3, b3, c3, d3, &alpha, &beta, t))
        if (alpha > -EPSI_CROSS && beta > -EPSI_CROSS && alpha+beta < 1.0+EPSI_CROSS)
            if (*t > EPSI_STEP) {
                *point = p[0] * alpha + p[1] * beta + p[2] * (1.0-alpha-beta);
                details->alpha = alpha;
                details->beta = beta;
                details->faced = (Dot(direction, Cross(p[1]-p[0], p[2]-p[0])) <= 0);
                return true;
            }
    return false;
}

Point FaceItem::GetNormal(const Point &pnt, const IntersectionDetails &details) {
    Point normal = vn[0] * details.alpha + vn[1] * details.beta + vn[2] * (1.0-details.alpha-details.beta);
    if (details.faced)
        normal = -normal;
    return normal;
}

Colors FaceItem::GetDiffuse(const Point &pnt, const IntersectionDetails &details) {
    if (use_texture && material->map_diffuse != NULL) {
        Point pt = vt[0] * details.alpha + vt[1] * details.beta + vt[2] * (1.0-details.alpha-details.beta);
        return material->map_diffuse->GetPixel(pt.x, pt.y);
    }
    return material->diffuse;
}

//// SphereItem ////

ItemType SphereItem::GetType() {
    return IT_SPHERE;
}

Cuboid SphereItem::GetCuboid() {
    return Cuboid(o.x-r, o.x+r, o.y-r, o.y+r, o.z-r, o.z+r);
}

pair<double, double> SphereItem::GetMinMaxDimension(int k) {
    double c = o.GetDimension(k);
    return make_pair(c-r, c+r);
}

bool SphereItem::CheckIntersect(const Point &viewpoint, const Point &direction) {
	double a = direction.x*direction.x + direction.y*direction.y + direction.z*direction.z;
	double b = 2.0 * (direction.x*(viewpoint.x-o.x) + direction.y*(viewpoint.y-o.y) + direction.z*(viewpoint.z-o.z));
	double c = sqr(viewpoint.x-o.x) + sqr(viewpoint.y-o.y) + sqr(viewpoint.z-o.z) - r*r;
	double x1, x2;
	if (solve_p2(a, b, c, &x1, &x2))
	   return (x1 > EPSI_CROSS && x1 + EPSI_CROSS < 1.0) || (x2 > EPSI_CROSS && x2 + EPSI_CROSS < 1.0);
    return false;
}

bool SphereItem::GetIntersection(const Point &viewpoint, const Point &direction, Point *point, double *t, IntersectionDetails *details) {
	double a = direction.x*direction.x + direction.y*direction.y + direction.z*direction.z;
	double b = 2.0 * (direction.x*(viewpoint.x-o.x) + direction.y*(viewpoint.y-o.y) + direction.z*(viewpoint.z-o.z));
	double c = sqr(viewpoint.x-o.x) + sqr(viewpoint.y-o.y) + sqr(viewpoint.z-o.z) - r*r;
	double x1, x2;
	if (solve_p2(a, b, c, &x1, &x2)) {
        *t = min(x1, x2);
        details->faced = true;
        if (*t < EPSI_CROSS) {
            *t = x1 + x2 - *t;
            if (*t < EPSI_CROSS)
                return false;
            details->faced = false;
        }
        *point = viewpoint + direction * *t;
        return true;
    }
    return false;
}

Point SphereItem::GetNormal(const Point &pnt, const IntersectionDetails &details) {
    Point normal = pnt - o;
    if (!details.faced)
        normal = -normal;
    return normal;
}

Colors SphereItem::GetDiffuse(const Point &pnt, const IntersectionDetails &details) {
    return material->diffuse;
}
