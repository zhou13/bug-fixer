#include "..\common\lib-matrix.h"
#include "..\common\common.h"
#include <stdio.h>
#include <lbfgs.h>
#include <cmath>
#include "mpi.h"

namespace rica
{
	extern	matrix	patches;
	extern	int patch_n;
	extern	int patch_size;
	extern	int centroid_n;
	extern	double lambda;
	extern	double epsilon;

	double sqr(double x);
	matrix inverse(matrix& M);
	bool rica_proc(const mk_config &config, matrix &patches, matrix &centroids);
	void l2rowscaledg(matrix &x, matrix &y, matrix &outderv,double alpha);
	void l2rowscaled(matrix &x, double alpha);
	
	static lbfgsfloatval_t softICACost(
    void *instance,
    const lbfgsfloatval_t *theta,
    lbfgsfloatval_t *g,
    const int n,
    const lbfgsfloatval_t step
    );

	static int progress(
    void *instance,
    const lbfgsfloatval_t *x,
    const lbfgsfloatval_t *g,
    const lbfgsfloatval_t fx,
    const lbfgsfloatval_t xnorm,
    const lbfgsfloatval_t gnorm,
    const lbfgsfloatval_t step,
    int n,
    int k,
    int ls
    );

	lbfgsfloatval_t debug_softICACost(
    void *instance,
    const lbfgsfloatval_t *theta,
    lbfgsfloatval_t *g,
    const int n,
    const lbfgsfloatval_t step
    );
}