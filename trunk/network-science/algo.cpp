// For Network Science 2013
// Algorithm Demo
//
// Feng Qiwei
// 2013.06.21
//
#include <iostream>
#include <cstring>
#include <cstdio>
#include <vector>
using namespace std;

const int MAXN=1000010;
const int MAXQ=1000010;

bool b_deg, b_con, b_vio;
int n, query_cnt;
int cur_time;

int edge_x[MAXQ], edge_y[MAXQ], edge_t[MAXQ], edge_n;
vector<int> ex[MAXN];
int ufs_fa[MAXN], ufs_sz[MAXN], ufs_t[MAXN];

void add_edge(int x, int y) {
	++edge_n;
	edge_x[edge_n]=x;
	edge_y[edge_n]=y;
	edge_t[edge_n]=t;
	// about degree
	if(b_deg) {
		ex[x].push_back(cur_time);
		ex[y].push_back(cur_time);
	}
	// about connecting
	if(b_con) {
		while(ufs_fa[x]>0) x=ufs_fa[x];
		while(ufs_fa[y]>0) y=ufs_fa[y];
		if(ufs_sz[x]<ufs_sz[y]) swap(x, y);
		ufs_fa[y]=x;
		ufs_sz[x]+=ufs_sz[y];
		ufs_t[y]=cur_time;
	}
}

int deg_query(int t, int x) {
	if(!b_deg) return 0;
	if(ex[x].size()==0) return 0;
	return (int)(lower_bound(ex[x].begin(), ex[x].end(), t) - e[x].begin());
}

int vio_deg_query(int t, int x) {
	int ans=0;
	for(int i=1; i<=edge_n && edge_t[i]<=t; ++i)
		if(edge_x[i]==x || edge_y[i]==x)
			++ans;
	return ans;
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
	printf(" -vio\trun vio program and check\n");
}

int main(int argc, char *argv[]) {
	b_con=b_deg=b_vio=true;
	char *s_inf=NULL;
	char *s_ouf=NULL;
	for(int i=1; i<argc; ++i) {
		if(strcmp(argv[i], "-con")==0)
			b_con=true;
		if(strcmp(argv[i], "-deg")==0)
			b_deg=true;
		if(strcmp(argv[i], "-vio")==0)
			b_vio=true;
		if(strcmp(argv[i], "-i")==0)
			s_inf=argv[++i];
		if(strcmp(argv[i], "-o")==0)
			s_ouf=argv[++i];
	}
	if(s_inf==NULL) {
		print_usage(argv[0]);
		return 0;
	}
	cur_time=0;
	edge_n=0;
	for(int i=0; i<MAXN; ++i) {
		ex[i].clear();
		ufs_fa[i]=0;
		ufs_sz[i]=1;
	}
	FILE *inf=fopen(s_inf, "r");
	FILE *ouf=(s_ouf==NULL ? stdout : fopen(s_out, "w"));
	fscanf(inf, "%d%d", &n, &query_cnt);
	for(int cur_q=1; cur_q<=query_cnt; ++cur_q) {
		char op[99], sop[99]; fscanf(inf, "%s", op);
		int x, y, t;
		if(op[0]=='a') {
			fscanf(inf, "%d%d", &x,&y);
			++cur_time;
			add_edge(x, y);
		}
		if(op[0]=='q') {
			fscanf(inf, "%s", sop);
			if(sop[1]=='c') {
				fscanf(inf, "%d%d%d", &t,&x,&y);
				bool ans=con_query(t, x, y);
				fprintf(ouf, "%d: qc %d %d %d -> %d", cur_time, t,x,y, ans);
				if(b_vio) {
					bool std=vio_con_query(t, x, y);
					fprintf(ouf, " / %d [%s]", std, (std==ans)?"yes":"error");
				}
				fprintf(ouf, "\n");
			}
			if(sop[1]=='d') {
				fscanf(inf, "%d%d", &t,&x);
				bool ans=deg_query(t, x);
				fprintf(ouf, "%d: qc %d %d -> %d", cur_time, t,x, ans);
				if(b_vio) {
					bool std=vio_deg_query(t, x);
					fprintf(ouf, " / %d [%s]", std, (std==ans)?"yes":"error");
				}
				fprintf(ouf, "\n");
			}
		}
	}
	
	return 0;
}
