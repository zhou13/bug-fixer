#include <algorithm>
#include <sstream>
#include <cstring>
#include <cstdio>
#include <vector>
#include <cmath>
using namespace std;

#define MAX_LINE_CHARS 512

inline double GetCostLimit(int meshes, double min_cost) {
	/*
	   if (min_cost <= 0) {
	   printf("min_cost = %.9lf <= 0!\n", min_cost);
	   return 1E-6;
	   }*/
	if (meshes > 100000) return 5.0 * min_cost;
	if (meshes > 10000) return 2.0 * min_cost;
	if (meshes > 5000) return 1.5 * min_cost;
	if (meshes > 1000) return 1.2 * min_cost;
	return (1.0 + 1E-6) * min_cost;
}

struct Point {
	double x, y, z;
	Point(double x = 0.0, double y = 0.0, double z = 0.0) :
		x(x), y(y), z(z) { }
	friend Point operator *(const Point &p, double a) {
		return Point(p.x*a, p.y*a, p.z*a);
	}
	friend Point operator +(const Point &p1, const Point &p2) {
		return Point(p1.x+p2.x, p1.y+p2.y, p1.z+p2.z);
	}
	friend Point operator -(const Point &p1, const Point &p2) {
		return Point(p1.x-p2.x, p1.y-p2.y, p1.z-p2.z);
	}
	friend Point Cross(const Point &p1, const Point &p2) {
		return Point(p1.y*p2.z-p1.z*p2.y, p1.z*p2.x-p1.x*p2.z, p1.x*p2.y-p1.y*p2.x);
	}
};

struct Mesh {
	int i[3];
};

struct Matrix4 {
	double a[4][4];
	void Clear() {
		for (int i = 0; i < 4; ++ i)
			for (int j = 0; j < 4; ++ j)
				a[i][j] = 0.0;
	}
	void Add(const Matrix4 &tmp) {
		for (int i = 0; i < 4; ++ i)
			for (int j = 0; j < 4; ++ j)
				a[i][j] += tmp.a[i][j];	
	}
};

struct Vector4 {
	double a[4];
	void Clear() {
		a[0] = a[1] = a[2] = a[3] = 0.0;
	}
	Vector4 MultMatrix(const Matrix4 &mat) {
		Vector4 ret;
		ret.Clear();
		for (int i = 0; i < 4; ++ i)
			for (int j = 0; j < 4; ++ j)
				ret.a[j] += a[i] * mat.a[i][j];
		return ret;
	}
	double MultVector(const Vector4 &vec) {
		return a[0]*vec.a[0] + a[1]*vec.a[1] + a[2]*vec.a[2] + a[3]*vec.a[3];
	}	
};

struct DeleteScheme {
	int p1, p2;
	Point target;
	double cost;
	DeleteScheme(int p1 = -1, int p2 = -1, const Point &target = Point(), double cost = 1E50) :
		p1(p1), p2(p2), target(target), cost(cost) { }
	friend bool operator <(const DeleteScheme &tmp1, const DeleteScheme &tmp2) {
		return tmp1.cost < tmp2.cost;
	}
};

vector<vector<int> > point_mp;
vector<int> point_into;
vector<Point> point;
vector<Mesh> mesh;
vector<bool> mesh_modified;
vector<Matrix4> mesh_K;
int point_count;
int mesh_count;

void InitalizeArraysSize() {
	point.resize(point_count);
	point_mp.resize(point_count);
	point_into.resize(point_count);
	mesh.resize(mesh_count);
	mesh_K.resize(mesh_count);
	mesh_modified.resize(mesh_count);
	for (int i = 0; i < mesh_count; ++ i)
		mesh_modified[i] = true;
}

bool ReadFromFile(const char *filename) {
	FILE *inf = fopen(filename, "r");
	if (inf == NULL)
		return false;
	char tmp[9];
	fscanf(inf, "%s%d%d", tmp, &point_count, &mesh_count);
	InitalizeArraysSize();
	for (int i = 0; i < point_count; ++ i)
		fscanf(inf, "%s%lf%lf%lf", tmp, &point[i].x, &point[i].y, &point[i].z);
	for (int i = 0; i < mesh_count; ++ i) {
		fscanf(inf, "%s%d%d%d", tmp, &mesh[i].i[0], &mesh[i].i[1], &mesh[i].i[2]);
		-- mesh[i].i[0]; -- mesh[i].i[1]; -- mesh[i].i[2];
	}
	fclose(inf);
	return true;
}

bool WriteToFile(const char *filename) {
	FILE *ouf = fopen(filename, "w");
	if (ouf == NULL)
		return false;
	fprintf(ouf, "# %d %d\n", point_count, mesh_count);
	for (int i = 0; i < point_count; ++ i)
		fprintf(ouf, "v %.15lf %.15lf %.15lf\n", point[i].x, point[i].y, point[i].z);
	for (int i = 0; i < mesh_count; ++ i)
		fprintf(ouf, "f %d %d %d\n", mesh[i].i[0]+1, mesh[i].i[1]+1, mesh[i].i[2]+1);
	fclose(ouf);
	return true;
}

Matrix4 GetQ(int pi) {
	Matrix4 mat;
	mat.Clear();
	for (int ind_i = 0; ind_i < (int)point_mp[pi].size(); ++ ind_i) {
		int mesh_i = point_mp[pi][ind_i];
		mat.Add(mesh_K[mesh_i]);
		//mat = matrixK[mesh_i];
	}
	return mat;
}

inline double ComputeCost(const Matrix4 &Q, const Point &p) {
	Vector4 v, tmp;
	v.a[0] = p.x, v.a[1] = p.y, v.a[2] = p.z, v.a[3] = 1.0;
	tmp = v.MultMatrix(Q);
	double cost = v.MultVector(tmp);
	/*
	   if (cost < 0) {
	   double sum = 0.0;
	   for (int i = 0; i < 4; ++ i) for (int j = 0; j < 4; ++ j) printf("%.3lf%c", Q.a[i][j], j==3?'\n':' ');
	   for (int i = 0; i < 4; ++ i) for (int j = 0; j < 4; ++ j) sum += Q.a[i][j];
	   printf("sum=%.9lf\n", sum);
	   for (int i = 0; i < 4; ++ i) printf("%.3lf ", v.a[i]);
	   while (1) system("pause");
	   }*/
	//printf("cost=%.8lf\n", cost);
	return cost;
}

inline double Det3(double a1, double b1, double c1, double a2, double b2, double c2, double a3, double b3, double c3) {
	return a1*b2*c3 + b1*c2*a3 + c1*a2*b3 - a1*c2*b3 - b1*a2*c3 - c1*b2*a3;
}

DeleteScheme GetDeleteScheme(int p1, int p2) {
	DeleteScheme ret;
	Matrix4 Q;
	Q = GetQ(p1);
	Q.Add(GetQ(p2));
	double D = Det3(
			Q.a[0][0], Q.a[0][1], Q.a[0][2],
			Q.a[1][0], Q.a[1][1], Q.a[1][2],
			Q.a[2][0], Q.a[2][1], Q.a[2][2]);
	if (fabs(D) < 1E-10)
		//printf("fail ##\n");
		ret.target = (point[p1] + point[p2]) * 0.5;
	else {
		double Dx = Det3(
				-Q.a[0][3], Q.a[0][1], Q.a[0][2],
				-Q.a[1][3], Q.a[1][1], Q.a[1][2],
				-Q.a[2][3], Q.a[2][1], Q.a[2][2]);
		double Dy = Det3(
				Q.a[0][0], -Q.a[0][3], Q.a[0][2],
				Q.a[1][0], -Q.a[1][3], Q.a[1][2],
				Q.a[2][0], -Q.a[2][3], Q.a[2][2]);
		double Dz = Det3(
				Q.a[0][0], Q.a[0][1], -Q.a[0][3],
				Q.a[1][0], Q.a[1][1], -Q.a[1][3],
				Q.a[2][0], Q.a[2][1], -Q.a[2][3]);
		ret.target.x = Dx / D;
		ret.target.y = Dy / D;
		ret.target.z = Dz / D;
	}
	//	ret.target = (point[p1] + point[p2]) * 0.5;
	ret.p1 = p1, ret.p2 = p2;
	ret.cost = ComputeCost(Q, ret.target);
	return ret;
}

void PrecomputeK() {
	for (int i = 0; i < mesh_count; ++ i)
		if (mesh_modified[i]) {
			mesh_modified[i] = false;
			Point p0 = point[mesh[i].i[0]];
			Point p1 = point[mesh[i].i[1]];
			Point p2 = point[mesh[i].i[2]];
			Point normal = Cross(p1-p0, p2-p0);
			double a = normal.x, b = normal.y, c = normal.z, d;
			double ssum = a*a + b*b + c*c;
			ssum = sqrt(ssum);
			a /= ssum, b /= ssum, c /= ssum, d = -(a*p0.x + b*p0.y + c*p0.z);
			mesh_K[i].a[0][0] = a*a;	mesh_K[i].a[0][1] = a*b;	mesh_K[i].a[0][2] = a*c;	mesh_K[i].a[0][3] = a*d;
			mesh_K[i].a[1][0] = b*a;	mesh_K[i].a[1][1] = b*b;	mesh_K[i].a[1][2] = b*c;	mesh_K[i].a[1][3] = b*d;
			mesh_K[i].a[2][0] = c*a;	mesh_K[i].a[2][1] = c*b;	mesh_K[i].a[2][2] = c*c;	mesh_K[i].a[2][3] = c*d;
			mesh_K[i].a[3][0] = d*a;	mesh_K[i].a[3][1] = d*b;	mesh_K[i].a[3][2] = d*c;	mesh_K[i].a[3][3] = d*d;
			//printf("precompute_K - i=%d  #5\n",i);
		}
	//mesh_modified_list.clear();
}


bool Simplify(int target_meshes) {
	double min_cost = 0.0;
	while (mesh_count > target_meshes) {
		printf("mesh_count=%d\n", mesh_count);
		PrecomputeK();
		for (int i = 0; i < point_count; ++ i) {
			point_mp[i].clear();
			point_into[i] = i;
		}
		for (int i = 0; i < mesh_count; ++ i)
			for (int j = 0; j < 3; ++ j)
				point_mp[mesh[i].i[j]].push_back(i);

		vector<DeleteScheme> schemes;
		for (int mesh_index = 0; mesh_index < mesh_count; ++ mesh_index)
			for (int vex_index = 0; vex_index < 3; ++ vex_index) {
				int p1 = mesh[mesh_index].i[vex_index];
				int p2 = mesh[mesh_index].i[(vex_index+1)%3];
				DeleteScheme tmp = GetDeleteScheme(p1, p2);
				schemes.push_back(tmp);
			}
		sort(schemes.begin(), schemes.end());

		double cost_limit = GetCostLimit(mesh_count, min_cost = max(min_cost, schemes[0].cost));
		int tmpi = schemes.size()/2 - 1000;
		if (tmpi > 0) cost_limit = max(cost_limit, schemes[tmpi].cost);

		int rest = mesh_count - target_meshes;
		//printf("cost_limit=%.30lf\n", cost_limit);
		for (vector<DeleteScheme>::iterator it = schemes.begin(); rest > 0 && it != schemes.end() && it->cost <= cost_limit; ++ it) {
			//	printf("here!\n", it->cost);
			bool flag = true;
			int vp[2] = {it->p1, it->p2};
			for (int i = 0; flag && i < 2; ++ i)
				for (int j = 0; j < (int)point_mp[vp[i]].size(); ++ j)
					if (mesh_modified[point_mp[vp[i]][j]]) {
						flag = false;
						break;
					}
			if (flag) {
				//printf("apply.  cost=%.20lf  cost_limit=%.20lf\n", it->cost, cost_limit);
				rest -= 2;
				for (int i = 0; flag && i < 2; ++ i)
					for (int j = 0; j < (int)point_mp[vp[i]].size(); ++ j)
						mesh_modified[point_mp[vp[i]][j]] = true;	
				point_into[it->p2] = it->p1;
				point[it->p1] = it->target;
			}// else
			//printf("fail to cal!.  cost=%.9lf  limit=%.9lf\n", it->cost, cost_limit);
		}
		for (int i = 0; i < mesh_count; )
			if (mesh_modified[i]) {
				for (int j = 0; j < 3; ++ j)
					mesh[i].i[j] = point_into[mesh[i].i[j]];
				if (mesh[i].i[0] == mesh[i].i[1] || mesh[i].i[0] == mesh[i].i[2] || mesh[i].i[1] == mesh[i].i[2]) {
					mesh[i] = mesh[mesh_count-1];
					mesh_modified[i] = mesh_modified[mesh_count-1];
					mesh_K[i] = mesh_K[mesh_count-1];
					-- mesh_count;
				} else
					++ i;
			} else
				++ i;
	}

	vector<int> index(point_count, -1);
	for (int i = 0; i < mesh_count; ++ i)
		for (int j = 0; j < 3; ++ j)
			index[mesh[i].i[j]] = 1;
	int tmp_points = point_count;
	point_count = 0;
	for (int i = 0; i < tmp_points; ++ i)
		if (index[i] > 0) {
			index[i] = point_count ++;
			point[index[i]] = point[i];
		}
	for (int i = 0; i < mesh_count; ++ i)
		for (int j = 0; j < 3; ++ j)
			mesh[i].i[j] = index[mesh[i].i[j]];
	point.resize(point_count);
	mesh.resize(mesh_count);
	return true;
}

int main(int argc, char *argv[]) {
	if (argc != 4) {
		printf("Usage: %s in_file out_file target_mesh_num\n", argv[0]);
		return -1;
	}
	if (! ReadFromFile(argv[1])) {
		printf("ERROR: Cannot open file!\n");
		return 1;
	}
	int target_meshes = atoi(argv[3]);
	if (! Simplify(target_meshes)) {
		printf("ERROR: Fail to simplify!\n");
		return 3;
	}
	if (! WriteToFile(argv[2])) {
		printf("ERROR: Fail to write file!\n");
		return 2;
	}
	return 0;
}

