#ifndef H_BASIC
#define H_BASIC
#include <string>
#include <cmath>
using namespace std;

#define EPSI_DIV 1E-12
#define EPSI_CROSS 1E-8
#define EPSI_STEP 1E-5
#define EPSI_BUILD_SP 1E-8
#define PI 3.1415926535897932384626433

#define MAX_ITEMS 5000000
#define LIMIT_NODE_ITEMS 50
#define LIMIT_FACTOR 0.001
#define LIMIT_DEPTH 20

static int __cnt;

template <class T> T abs(T x) { return x < 0 ? -x : x; }
template <class T> T sqr(T x) { return x * x; }

inline int random_int(int n) {
    if (n <= 0)
        throw string("random(): n <= 0");
    return (rand()%10000 * 10000 + rand()%10000) % n;
}

inline void rotate_2D(double *x, double *y, double angle) {
	double x0 = *x * cos(angle) - *y * sin(angle);
	double y0 = *x * sin(angle) + *y * cos(angle);
	*x = x0, *y = y0;
}

inline double det3(double a1, double b1, double c1, double a2, double b2, double c2, double a3, double b3, double c3) {
    return a1*b2*c3 + b1*c2*a3 + c1*a2*b3 - a1*c2*b3 - b1*a2*c3 - c1*b2*a3;
}

inline bool solve_p2(double a, double b, double c, double *x1, double *x2) {
	double delta = b*b - 4*a*c;
	if (delta < 0)
		return false;
	delta = sqrt(delta);
	*x1 = (-b + delta) / 2.0 / a;
	*x2 = (-b - delta) / 2.0 / a;
	return true;
}

inline bool solve_e3(double a1, double b1, double c1, double d1, 
        double a2, double b2, double c2, double d2, 
        double a3, double b3, double c3, double d3,
        double *x, double *y, double *z) {
    double D = det3(a1, b1, c1, a2, b2, c2, a3, b3, c3);
    if (fabs(D) < EPSI_DIV)
        return false;
    *x = det3(d1, b1, c1, d2, b2, c2, d3, b3, c3) / D;
    *y = det3(a1, d1, c1, a2, d2, c2, a3, d3, c3) / D;
    *z = det3(a1, b1, d1, a2, b2, d2, a3, b3, d3) / D;
    return true;
}


struct Point {
    double x, y, z;
    Point(double x = 0.0, double y = 0.0, double z = 0.0) :
        x(x), y(y), z(z) { }
	double Abs() const {
		return sqrt(x*x + y*y + z*z);
	}
	double GetDimension(int k) const {
        switch (k) {
            case 0: return x;
            case 1: return y;
            case 2: return z;
        }
        return 0.0;
    }
	void MakeToUnit(double l = 1.0) {
		double len = Abs();
		if (fabs(len) < EPSI_DIV)
            return;
			//throw std::string("Point.make_to_unit(): |len| < 1e-10");
		x *= l / len, y *= l / len, z *= l / len;
	}
	Point operator -() const {
        return Point(-x, -y, -z);
    }
    void operator +=(const Point &p0) {
        *this = *this + p0;
    }
    void operator -=(const Point &p0) {
        *this = *this - p0;
    }
	friend Point operator *(const Point &p, double a) {
		return Point(p.x*a, p.y*a, p.z*a);
	}
	friend Point operator *(double a, const Point &p) {
		return Point(p.x*a, p.y*a, p.z*a);
	}
	friend Point operator +(const Point &p1, const Point &p2) {
		return Point(p1.x+p2.x, p1.y+p2.y, p1.z+p2.z);
	}
	friend Point operator -(const Point &p1, const Point &p2) {
		return Point(p1.x-p2.x, p1.y-p2.y, p1.z-p2.z);
	}
	friend double Dot(const Point &p1, const Point &p2) {
		return p1.x*p2.x + p1.y*p2.y + p1.z*p2.z;
	}
	friend Point Cross(const Point &p1, const Point &p2) {
		return Point(p1.y*p2.z-p1.z*p2.y, p1.z*p2.x-p1.x*p2.z, p1.x*p2.y-p1.y*p2.x);
	}
	friend double Dist(const Point &p1, const Point &p2) {
        return sqrt(sqr(p1.x-p2.x)+sqr(p1.y-p2.y)+sqr(p1.z-p2.z));
    }
};

struct Colors {
    double r, g, b;
    Colors() { }
    Colors(double c): r(c), g(c), b(c) { }
    Colors(double r, double g, double b): r(r), g(g), b(b) { }
	Colors& operator +=(const Colors &c0) {
		*this = *this + c0;
	}
	Colors& operator -=(const Colors &c0) {
		*this = *this - c0;
	}
	void operator *=(const Colors &c0) {
		*this = *this * c0;
	}
	void operator *=(const double &a) {
		*this = *this * a;
	}
	friend Colors operator +(const Colors &c1, const Colors &c2) {
		return Colors(c1.r+c2.r, c1.g+c2.g, c1.b+c2.b);
	}
	friend Colors operator -(const Colors &c1, const Colors &c2) {
		return Colors(c1.r-c2.r, c1.g-c2.g, c1.b-c2.b);
	}
	friend Colors operator *(const Colors &c1, const Colors &c2) {
		return Colors(c1.r*c2.r, c1.g*c2.g, c1.b*c2.b);
	}
	friend Colors operator *(const Colors &c, double a) {
		return Colors(c.r*a, c.g*a, c.b*a);
	}
	friend Colors operator *(double a, const Colors &c) {
		return Colors(c.r*a, c.g*a, c.b*a);
	}
};

struct Cuboid {
    double x1, x2, y1, y2, z1, z2;
    Cuboid() { }
    Cuboid(double x, double y, double z) :
        x1(x), x2(x), y1(y), y2(y), z1(z), z2(z) { }
    Cuboid(double x1, double x2, double y1, double y2, double z1, double z2) :
        x1(x1), x2(x2), y1(y1), y2(y2), z1(z1), z2(z2) { }
    bool CheckRay(const Point &viewpoint, const Point &direction) {
        double t1 = -1E50, t2 = 1E50;
        if (abs(direction.x) > EPSI_DIV) {
            double p1 = (x1-viewpoint.x)/direction.x;
            double p2 = (x2-viewpoint.x)/direction.x;
            if (p1 > p2) swap(p1, p2);
            t1 = max(t1, p1), t2 = min(t2, p2);
        } else
            if (viewpoint.x + EPSI_CROSS < x1 || viewpoint.x > x2 + EPSI_CROSS)
                return false;
        if (t1 > t2 + EPSI_CROSS) return false;
    
        if (abs(direction.y) > EPSI_DIV) {
            double p1 = (y1-viewpoint.y)/direction.y;
            double p2 = (y2-viewpoint.y)/direction.y;
            if (p1 > p2) swap(p1, p2);
            t1 = max(t1, p1), t2 = min(t2, p2);
        } else
            if (viewpoint.y + EPSI_CROSS < y1 || viewpoint.y > y2 + EPSI_CROSS)
                return false;
        if (t1 > t2 + EPSI_CROSS) return false;
                
        if (abs(direction.z) > EPSI_DIV) {
            double p1 = (z1-viewpoint.z)/direction.z;
            double p2 = (z2-viewpoint.z)/direction.z;
            if (p1 > p2) swap(p1, p2);
            t1 = max(t1, p1), t2 = min(t2, p2);
        } else
            if (viewpoint.z + EPSI_CROSS < z1 || viewpoint.z > z2 + EPSI_CROSS)
                return false;
                
       // printf("end check\n");fflush(stdout);
        //printf("t1=%.9lf t2=%.9lf\n", t1, t2);
        if (t1 > t2 + EPSI_CROSS) return false;
        return true;
    }
    void CutInto(int dm, double cut, Cuboid *c1, Cuboid *c2) {
        *c1 = *c2 = *this;
        switch (dm) {
            case 0:
                c1->x2 = c2->x1 = cut;
                break;
            case 1:
                c1->y2 = c2->y1 = cut;
                break;
            case 2:
                c1->z2 = c2->z1 = cut;
                break;
        }
    }
    void operator |=(const Cuboid &c0) {
        *this = *this | c0;
    }
    void operator &=(const Cuboid &c0) {
        *this = *this & c0;
    }
    friend Cuboid operator |(const Cuboid &a, const Cuboid &b) {
        return Cuboid(min(a.x1, b.x1), max(a.x2, b.x2), min(a.y1, b.y1), max(a.y2, b.y2), min(a.z1, b.z1), max(a.z2, b.z2));
    }
    friend Cuboid operator &(const Cuboid &a, const Cuboid &b) {
        return Cuboid(max(a.x1, b.x1), min(a.x2, b.x2), max(a.y1, b.y1), min(a.y2, b.y2), max(a.z1, b.z1), min(a.z2, b.z2));
    }
};

#endif
