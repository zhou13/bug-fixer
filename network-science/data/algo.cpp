// For Network Science 2013
// Algorithm Demo
//
// Feng Qiwei
// 2013.06.24
//
#include <iostream>
#include <cstring>
#include <cstdio>
#include <vector>
#include <cmath>
using namespace std;

const int MAXN=3000010;
const int MAXQ=3000010;

const double c_eta=1000.;
const double c_mu=0.2;

bool b_deg, b_con, b_act;
bool b_vio, b_info;
int n;
int cur_time;

int edge_x[MAXQ], edge_y[MAXQ], edge_t[MAXQ], edge_n; double edge_w[MAXQ];
vector<int> et[MAXN];
vector<double> ez[MAXN], ed[MAXN];
int ufs_fa[MAXN], ufs_sz[MAXN], ufs_t[MAXN];

void add_edge(int x, int y, double w) {
	++edge_n;
	edge_x[edge_n]=x;
	edge_y[edge_n]=y;
	edge_w[edge_n]=w;
	edge_t[edge_n]=cur_time;

	et[x].push_back(cur_time);
	et[y].push_back(cur_time);
	// about degree
	if(b_deg) {
		ed[x].push_back(ed[x].back()+w);
		ed[y].push_back(ed[y].back()+w);
	}
	// about activity
	if(b_act) {
		ez[x].push_back(min(0., ez[x].back()+cur_time-et[x][ez[x].size()-1])-c_eta*w);
		ez[y].push_back(min(0., ez[y].back()+cur_time-et[y][ez[y].size()-1])-c_eta*w);
	}
	// about connecting
	if(b_con) {
		while(ufs_fa[x]>0) x=ufs_fa[x];
		while(ufs_fa[y]>0) y=ufs_fa[y];
		if(x!=y) {
			if(ufs_sz[x]<ufs_sz[y]) swap(x, y);
			ufs_fa[y]=x;
			ufs_sz[x]+=ufs_sz[y];
			ufs_t[y]=cur_time;
		}
	}
}

double deg_query(int t, int x) {
	if(!b_deg) return 0.;
	int i=(int)(lower_bound(et[x].begin(), et[x].end(), t+1) - et[x].begin());
	return ed[x][i-1];
}

double vio_deg_query(int t, int x) {
	if(!b_deg) return 0.;
	double ans=0;
	for(int i=1; i<=edge_n && edge_t[i]<=t; ++i) {
		if(edge_x[i]==x) ans+=edge_w[i];
		if(edge_y[i]==x) ans+=edge_w[i];
	}
	return ans;
}

double act_query(int t, int x) {
	if(!b_act) return 0.;
	int i=(int)(lower_bound(et[x].begin(), et[x].end(), t+1) - et[x].begin());
	double z=min(0., ez[x][i-1]+t-et[x][i-1]);
	return pow(c_mu, 1./z);
}

double vio_act_query(int t, int x) {
	if(!b_act) return 0.;
	double z=0; int e=0;
	for(int i=1; i<=edge_n && edge_t[i]<=t; ++i) {
		z=min(0., z+edge_t[i]-e);
		e=edge_t[i];
		if(edge_x[i]==x) z-=c_eta*edge_w[i];
		if(edge_y[i]==x) z-=c_eta*edge_w[i];
	}
	z=min(0., z+t-e);
	return pow(c_mu, 1./z);
}

bool con_query(int t, int x, int y) {
	if(!b_con) return false;
	while(ufs_fa[x]>0 && ufs_t[x]<=t) x=ufs_fa[x];
	while(ufs_fa[y]>0 && ufs_t[y]<=t) y=ufs_fa[y];
	return x==y;
}

int uf[MAXN]; int get_uf(int x) {
	return uf[x]==0 ? x : (uf[x]=get_uf(uf[x]));
}
bool vio_con_query(int t, int x, int y) {
	if(!b_con) return false;
	for(int i=1; i<=n; ++i) uf[i]=0;
	for(int i=1; i<=edge_n && edge_t[i]<=t; ++i) {
		int x=get_uf(edge_x[i]);
		int y=get_uf(edge_y[i]);
		if(x!=y) uf[x]=y;
	}
	return get_uf(x)==get_uf(y);
}

void print_usage(char *s) {
	printf("Usage: %s -i INPUT [-o OUTPUT] [OPTIONS]\n", s);
	printf("\n");
	printf(" -con\tenable maintaining of connectivity\n");
	printf(" -deg\tenable maintaining of degree\n");
	printf(" -act\tenable maintaining of activity\n");
	printf(" -vio\trun vio program and check\n");
}

int main(int argc, char *argv[]) {
	b_con=b_deg=b_vio=b_act=b_info=false;
	char *s_inf=NULL;
	char *s_ouf=NULL;
	for(int i=1; i<argc; ++i) {
		if(strcmp(argv[i], "-con")==0)
			b_con=true;
		if(strcmp(argv[i], "-deg")==0)
			b_deg=true;
		if(strcmp(argv[i], "-act")==0)
			b_act=true;
		if(strcmp(argv[i], "-vio")==0)
			b_vio=true;
		if(strcmp(argv[i], "-info")==0)
			b_info=true;
		if(strcmp(argv[i], "-i")==0)
			s_inf=argv[++i];
		if(strcmp(argv[i], "-o")==0)
			s_ouf=argv[++i];
	}
	if(s_inf==NULL) {
		print_usage(argv[0]);
		return 0;
	}
	FILE *inf=fopen(s_inf, "r");
	FILE *ouf=(s_ouf==NULL ? stdout : fopen(s_ouf, "w"));
	fscanf(inf, "%d", &n);
	cur_time=0;
	edge_n=0;
	for(int i=1; i<=n; ++i) {
		et[i].clear(); et[i].push_back(0);
		ed[i].clear(); ed[i].push_back(0.);
		ez[i].clear(); ez[i].push_back(0.);
		ufs_fa[i]=0;
		ufs_sz[i]=1;
	}
	int n_yes=0, n_tot=0;
	while(1) {
		char op[99], sop[99];
		if(fscanf(inf, "%s", op)==EOF) break;
		int x, y, t; double w;
		if(op[0]=='a') {
			fscanf(inf, "%d%d%lf", &x,&y,&w);
			++cur_time;
			add_edge(x, y, w);
			if(b_info)
				fprintf(ouf, "%d: a %d %d %.3lf\n", cur_time, x, y, w);
		}
		if(op[0]=='q') {
			fscanf(inf, "%s", sop);
			if(sop[1]=='c') {
				fscanf(inf, "%d%d%d", &t,&x,&y);
				bool ans=con_query(t, x, y);
				if(b_info)
					fprintf(ouf, "%d: qc %d %d %d -> %d", cur_time, t,x,y, ans);
				if(b_vio) {
					bool std=vio_con_query(t, x, y);
					if(b_info)
						fprintf(ouf, " / %d [%s]", std, (std==ans)?"yes":"error");
					if(std==ans) ++n_yes; ++n_tot;
				}
				if(b_info)
					fprintf(ouf, "\n");
			}
			if(sop[1]=='d') {
				fscanf(inf, "%d%d", &t,&x);
				double ans=deg_query(t, x);
				if(b_info)
					fprintf(ouf, "%d: qd %d %d -> %.3lf", cur_time, t,x, ans);
				if(b_vio) {
					double std=vio_deg_query(t, x);
					bool ok=(fabs(std-ans)<0.001);
					if(b_info)
						fprintf(ouf, " / %.3lf [%s]", std, ok?"yes":"error");
					if(ok) ++n_yes; ++n_tot;
				}
				if(b_info)
					fprintf(ouf, "\n");
			}
			if(sop[1]=='a') {
				fscanf(inf, "%d%d", &t,&x);
				double ans=act_query(t, x);
				if(b_info)
					fprintf(ouf, "%d: qa %d %d -> %.6lf", cur_time, t,x, ans);
				if(b_vio) {
					double std=vio_act_query(t, x);
					bool ok=(fabs(std-ans)<1E-6);
					if(b_info)
						fprintf(ouf, " / %.6lf [%s]", std, ok?"yes":"error");
					if(ok) ++n_yes; ++n_tot;
				}
				if(b_info)
					fprintf(ouf, "\n");
			}
		}
	}
	if(b_info)
		fprintf(ouf, "Finish.\n");
	if(b_vio)
		fprintf(ouf, " correct %d/%d (%.5lf)\n", n_yes, n_tot, (double)n_yes/(double)n_tot);
	
	return 0;
}
