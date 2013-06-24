// For Network Science 2013
// Algorithm Demo
//
// Feng Qiwei
// 2013.06.24
//
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <vector>
#include <cmath>
using namespace std;

const int MAXN=6010;
const int MAXM=30010;
const int INTERVAL=20;
const int num=20;

const double c_eta=1000.;
const double c_mu=2;
const double c_mup=12;
const double c_alpha=0.8;
const double c_beta=1.;

int n;
int cur_time;

vector<int> edge[MAXN];
double tmp_z[MAXN];
double tmp_d[MAXN];

int tmp_t[MAXN];

inline void update_z(int x) {
	tmp_z[x]=min(0., tmp_z[x]+cur_time-tmp_t[x]);
	tmp_t[x]=cur_time;
}

void add_edge(int x, int y, double w) {
	update_z(x);
	update_z(y);
	tmp_z[x]-=c_eta*w; tmp_d[x]+=w;
	tmp_z[y]-=c_eta*w; tmp_d[y]+=w;
	edge[x].push_back(y);
	edge[y].push_back(x);
}

double x[MAXN], oldx[MAXN], a[MAXN];
void cal_centrality() {
	//printf("\ncal_centrality n=%d\n", n);
	for(int i=1; i<=n; ++i) {
		update_z(i);
		if(fabs(tmp_z[i])<1E-5) a[i]=0;
			else a[i]=pow(c_mu, c_mup/tmp_z[i]);
	}
	for(int cnt=1; ; ++cnt) {
		memmove(oldx, x, sizeof(x));
		for(int i=1; i<=n; ++i) {
			x[i]=c_beta;
			for(vector<int>::iterator it=edge[i].begin(); it!=edge[i].end(); ++it)
				x[i]+=c_alpha*a[*it]*oldx[*it]/tmp_d[*it];
		}
		double delta=0;
		for(int i=1; i<=n; ++i) delta=max(delta, fabs(x[i]-oldx[i]));
		//if(cnt%1000==0) printf("  iterating.. cnt=%d delta=%.5lf\n", cnt, delta);
		if(delta<1E-4) {
			//printf("  finish at cnt=%d\n", cnt);
			break;
		}
	}
}

double px[MAXN], oldpx[MAXN];
void cal_pagerank() {
	//printf("cal_pagerank n=%d\n", n);
	for(int cnt=1; ; ++cnt) {
		memmove(oldpx, px, sizeof(px));
		for(int i=1; i<=n; ++i) {
			px[i]=c_beta;
			for(vector<int>::iterator it=edge[i].begin(); it!=edge[i].end(); ++it)
				px[i]+=c_alpha*oldpx[*it]/tmp_d[*it];
		}
		double delta=0;
		for(int i=1; i<=n; ++i) delta=max(delta, fabs(px[i]-oldpx[i]));
		//if(cnt%100==0) printf("  iterating.. cnt=%d delta=%.5lf\n", cnt, delta);
		if(delta<1E-5) {
			//printf("  finish at cnt=%d\n", cnt);
			break;
		}
	}
}

int id[num];

int main(int argc, char *argv[]) {
	FILE *inf=fopen("CA-GrQc-output.txt", "r");
	FILE *ouf1[num], *ouf2[num];
	printf("$1\n");
	for(int i=0; i<num; ++i) {
		char s[999];
		sprintf(s, "exp-g%d-1.tex", i); ouf1[i]=fopen(s, "w");
		sprintf(s, "exp-g%d-2.tex", i); ouf2[i]=fopen(s, "w");
	}
	printf("$2\n");
	int _m; fscanf(inf, "%d%d", &n,&_m);
	for(int i=0; i<num; ++i) id[i]=i; //rand()%n+1;
	cur_time=0;
	for(int i=1; i<=n; ++i) {
		tmp_z[i]=0.; tmp_t[i]=0;
		edge[i].clear();
		x[i]=0.; px[i]=0.;
	}
	printf("$3\n");
	for(int next_time=0; ; ) {
		char cmd[9]; if(fscanf(inf, "%s", cmd)==EOF) break;
		if(cmd[0]=='a') {
			int x, y; double w;
			fscanf(inf, "%d%d%lf",&x,&y,&w);
			add_edge(x, y, w);
		} if(cmd[0]=='d') {
			int x; fscanf(inf, "%d", &x);
			cur_time+=x;
		}
		if(cur_time>next_time) {
			int k=1;
			cal_centrality();
			cal_pagerank();
			for(int i=0; i<num; ++i) {
				fprintf(ouf1[i], "%.3lf %.7lf\n", cur_time/1000., x[id[i]]);
				fprintf(ouf2[i], "%.3lf %.7lf\n", cur_time/1000., px[id[i]]);
			}
			printf("t=%d\t%.5lf\t%.5lf\n",cur_time,x[k],px[k]);
			//for(int i=1; i<=10; ++i) printf("%.4lf ", x[i]); printf("\n");
			//for(int i=1; i<=10; ++i) printf("%.4lf ", px[i]); printf("\n");
			next_time+=INTERVAL;
		}
	}
	fclose(inf);
	for(int i=0; i<num; ++i) {
		fclose(ouf1[i]);
		fclose(ouf2[i]);
	}

	system("copy exp.tex.head exp.tex");
	FILE *f=fopen("exp.tex", "a");
	for(int i=0; i<num; ++i) {
fprintf(f, "\\begin{center} \\begin{tikzpicture}[y=.4cm, x=.5cm, font=\\sffamily]\n");
fprintf(f, "\\draw (0,0) -- coordinate (x axis mid) (21, 0);\n");
fprintf(f, "\\draw (0,0) -- coordinate (y axis mid) (0, 11);\n");
fprintf(f, "\\node[right=1cm] at (0, 11) {%d};\n", id[i]);
fprintf(f, "\\foreach \\x in {1,2,...,20}\n");
fprintf(f, "\\draw (\\x,1pt) -- (\\x,-3pt) node[anchor=north] {\\scriptsize \\x};\n");
fprintf(f, "\\foreach \\y in {1,2,...,11}\n");
fprintf(f, "\\draw (1pt,\\y) -- (-3pt,\\y) node[anchor=east] {\\scriptsize \\y};\n");
fprintf(f, "\\draw[color=red!70!black] plot file {exp-g%d-1};\n", i);
fprintf(f, "\\draw[color=blue!70!black] plot file {exp-g%d-2};\n", i);
fprintf(f, "\\end{tikzpicture} \\end{center}\n");
	}
fprintf(f, "\\end{document}\n");
	fclose(f);

	system("pdflatex exp");
        
	system("del exp-g*.tex");
	
	return 0;
}
