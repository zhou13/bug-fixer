#include "rica.h"

namespace rica
{

matrix	patches;
int patch_n;
int patch_size;
int centroid_n;
double lambda;
double epsilon;
int rank;
int size;

double sqr(double x)
{
	return x * x;
}

matrix inverse(matrix& M)
{
	matrix MT;

	MT.create(M.cols(),M.rows());
	for(int i=0;i<M.rows();i++)
		for(int j=0;j<M.cols();j++)
			MT.val(j,i)=M.val(i,j);
	
	return MT;
}

bool rica_proc(const mk_config &config, matrix &patches, matrix &centroids)
{
	lbfgs_parameter_t param;
	int ret;
	double fx;

	printf("initializing rica...\n");fflush(stdout);

	srand((unsigned)time(NULL));
//	srand(1);

	lbfgs_parameter_init(&param);

	patch_n = patches.rows();
	patch_size = patches.cols();
	centroid_n = config.centroid_num;
	lambda = config.rica_lambda;
	epsilon = config.rica_epsilon;
	rank=config.rank;
	size=config.size;

	param.max_iterations = config.rica_max_iterations;
	param.ftol=config.rica_lbfgs_ftol;
	param.epsilon=config.rica_lbfgs_epsilon;
//	param.m=100;
//	param.linesearch=LBFGS_LINESEARCH_BACKTRACKING_STRONG_WOLFE;
//	param.max_step=1;

	rica::patches=inverse(patches);
	
	for(int i=0;i<(rica::patches).cols();i++)
	{	
		double sum=0;
		for(int j=0;j<(rica::patches).rows();j++)
			sum+=sqr((rica::patches).val(j,i));
		sum=sqrt(sum+1e-8);
		for(int j=0;j<(rica::patches).rows();j++)
			(rica::patches).val(j,i)/=sum;
	}
		

	centroids.create(centroid_n, patch_size);

	printf("generating initial solution...\n");fflush(stdout);
	
	if(rank==0)
	{
		for(int i=0;i<centroid_n;i++)
		{
			double sum=0;
			for(int j=0;j<patch_size;j++)
			{
			//	centroids.val(i,j)=0.5*0.01;
			//	if(j==0)
			//		centroids.val(i,j)/=(i+1);
				centroids.val(i,j)=(double)rand()/(double)RAND_MAX*0.01;
				sum+=sqr(centroids.val(i,j));
			}
			sum=sqrt(sum);
			for(int j=0;j<patch_size;j++)
				centroids.val(i,j)/=sum;
		}
	}
	//MPI_Bcast(centroids.get_buf(),centroid_n*patch_size,MPI_DOUBLE,0,MPI_COMM_WORLD);

	printf("optimizing...\n\n");fflush(stdout);

	if(rank==0)	// only process 0 do the lbfgs, flag==2 means end of the lbfgs
	{
		ret = lbfgs(centroid_n*patch_size, centroids.get_buf(), &fx, softICACost, progress, NULL, &param);
		int flag=2;
		for(int i=1;i<size;i++)
			MPI_Send(&flag, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
	}
	else	// process 0 will require process i to calculate i's part of softicacost
	{
		int flag;
		MPI_Status status;
		double *g=new double[centroid_n*patch_size];
		double *theta=new double[centroid_n*patch_size];

		for(;;)
		{
			MPI_Recv(&flag, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
			if(flag==2)
				break;
			MPI_Recv(theta, centroid_n*patch_size, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, &status);
			softICACost(NULL, theta, g, centroid_n*patch_size, 1.);
		}

		delete g;
		delete theta;
	}

	MPI_Bcast(&ret, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(&fx, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
	MPI_Bcast(centroids.get_buf(),centroid_n*patch_size,MPI_DOUBLE,0,MPI_COMM_WORLD);

	printf("L-BFGS optimization terminated with status code = %d\n", ret);fflush(stdout);
    printf("  fx = %f\n", fx);fflush(stdout);

	rica::patches.free();
	return true;
}

void l2rowscaledg(matrix &x, matrix &y, matrix &outderv,double alpha)
{
	double normeps=1e-5;
	double *epssumsq=new double[centroid_n];
	double *l2rows=new double[centroid_n];
	double *divide=new double[centroid_n];
	matrix grad;

	grad.create(outderv.rows(),outderv.cols());

	for(int i=0;i<x.rows();i++)
	{
		epssumsq[i]=normeps;
		for(int j=0;j<x.cols();j++)
			epssumsq[i]+=sqr(x.val(i,j));
		l2rows[i]=sqrt(epssumsq[i])*alpha;
	}

	for(int i=0;i<outderv.rows();i++)
		for(int j=0;j<outderv.cols();j++)
			grad.val(i,j)=outderv.val(i,j)/l2rows[i];

	for(int i=0;i<outderv.rows();i++)
	{
		divide[i]=0;
		for(int j=0;j<outderv.cols();j++)
			divide[i]+=x.val(i,j)*outderv.val(i,j);
		divide[i]/=epssumsq[i];
		for(int j=0;j<grad.cols();j++)
			grad.val(i,j)-=y.val(i,j)*divide[i];
	}

	for(int i=0;i<outderv.rows();i++)
		for(int j=0;j<outderv.cols();j++)
			outderv.val(i,j)=grad.val(i,j);

	delete epssumsq;
	delete l2rows;
	delete divide;
	grad.free();
}

void l2rowscaled(matrix &x, double alpha)
{
	double normeps=1e-5;
	double epssumsq;
	
	for(int i=0;i<x.rows();i++)
	{
		epssumsq=normeps;
		for(int j=0;j<x.cols();j++)
			epssumsq+=sqr(x.val(i,j));
		epssumsq=sqrt(epssumsq)*alpha;

		for(int j=0;j<x.cols();j++)
			x.val(i,j)/=epssumsq;
	}
}

/*void output(FILE *fo,matrix &temp)//
{
	fprintf(fo,"%d %d\n",temp.rows(),temp.cols());
	for(int i=0;i<temp.rows();i++)
	{
		for(int j=0;j<temp.cols();j++)
			fprintf(fo,"%lf ",temp.val(i,j));
		fprintf(fo,"\n");
	}
}*/

lbfgsfloatval_t debug_softICACost(
    void *instance,
    const lbfgsfloatval_t *theta,
    lbfgsfloatval_t *g,
    const int n,
    const lbfgsfloatval_t step
    )
{
	return softICACost(instance,theta,g,n,step);
}

static lbfgsfloatval_t softICACost(
    void *instance,
    const lbfgsfloatval_t *theta,
    lbfgsfloatval_t *g,
    const int n,
    const lbfgsfloatval_t step
    )
{
    int i,j;
    lbfgsfloatval_t fx = 0.0;
	matrix K;
	matrix W,Wold;
	matrix h,r,ht;
	matrix diff,W2grad,W1grad;
	matrix Wgrad;
	double sparsity_cost=0,reconstruction_cost=0;

	if(rank==0)
	{
		int flag=1;
		for(int i=1;i<size;i++)
			MPI_Send(&flag, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
		for(int i=1;i<size;i++)
			MPI_Send((void *)theta, n, MPI_DOUBLE, i, 0, MPI_COMM_WORLD);
	}


	MPI_Barrier(MPI_COMM_WORLD);

	W.create(centroid_n, patch_size);
	memmove(W.get_buf(), theta, sizeof(double) * centroid_n * patch_size);

	Wold.copy(W);

	l2rowscaled(W, 1);

	h=W.multiply(patches);


	ht=inverse(h);


	r=inverse(W);
	r.multiply_right(h);
	K.create(h.rows(),h.cols());

	for(int i=0;i<h.rows();i++)
		for(int j=0;j<h.cols();j++)
		{
			double temp=sqrt(epsilon+sqr(h.val(i,j)));
			sparsity_cost+=lambda*temp;
			K.val(i,j)=1./temp;
		}
	

	diff.create(patches.rows(),patches.cols());
	for(int i=0;i<diff.rows();i++)
		for(int j=0;j<diff.cols();j++)
		{
			diff.val(i,j)=r.val(i,j)-patches.val(i,j);
			reconstruction_cost+=sqr(diff.val(i,j));
		}
	reconstruction_cost*=0.5;


	fx=reconstruction_cost+sparsity_cost;
	
	W2grad=diff.multiply(ht);


	diff.multiply_left(W);

	for(int i=0;i<diff.rows();i++)
		for(int j=0;j<diff.cols();j++)
			diff.val(i,j)+=lambda*K.val(i,j)*h.val(i,j);

	W1grad=inverse(patches);
	W1grad.multiply_left(diff);


	Wgrad.create(W1grad.rows(), W1grad.cols());

	for(i=0;i<Wgrad.rows();i++)
		for(j=0;j<Wgrad.cols();j++)
			Wgrad.val(i,j)=W1grad.val(i,j)+W2grad.val(j,i);


	l2rowscaledg(Wold, W, Wgrad, 1);


	double *buf=Wgrad.get_buf();

	MPI_Allreduce(buf, g, n, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

	Wgrad.free();
	W1grad.free();
	W2grad.free();
	K.free();
	diff.free();
	r.free();
	h.free();
	W.free();
	Wold.free();
	ht.free();

	double ret;
	MPI_Allreduce(&fx, &ret, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
    return ret;
}

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
    )
{
    printf("Iteration %d:\n", k);fflush(stdout);
    printf("  fx = %f\n", fx);fflush(stdout);
    printf("  xnorm = %f, gnorm = %f, step = %.10f\n", xnorm, gnorm, step);fflush(stdout);
    printf("\n");fflush(stdout);
    return 0;
}

}