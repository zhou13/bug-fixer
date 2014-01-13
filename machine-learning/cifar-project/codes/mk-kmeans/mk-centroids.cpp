#include "mk-centroids.h"
using namespace std;


mk_centroids::mk_centroids(string learn_proc)
{
	this->learn_proc = learn_proc;
}


void mk_centroids::initialize()
{
	whitening = 0;
	inds.clear();
	cens.clear();
}


void mk_centroids::save(std::string fname) const
{
	printf("\n");
	printf("mk_centroids: saving...\n"); fflush(stdout);

	if (learn_proc == LPROC_KMEANS || learn_proc == LPROC_RICA) {
		FILE *f = fopen(fname.c_str(), "w");
		assert(f != NULL);
	
		fprintf(f, "%d\n", whitening);
		fprintf(f, "\n");
		if (whitening) {
			M.write(f);
			P.write(f);
		}
		fprintf(f, "\n");
		fprintf(f, "%d\n", inds.size());
		fprintf(f, "\n");
		for (size_t i = 0; i < inds.size(); ++i) {
			fprintf(f, "%d\n", inds[i].size());
			for (size_t j = 0; j < inds[i].size(); ++j)
				fprintf(f, "%d ", inds[i][j]);
			fprintf(f, "\n");
			cens[i].write(f);
		}
		fclose(f);
	}
	if (learn_proc == LPROC_AUTOENC) {
		FILE *f = fopen(fname.c_str(), "w");
		assert(f != NULL);
		fprintf(f, "%d\n", whitening);
		fprintf(f, "\n");
		if (whitening) {
			M.write(f);
			P.write(f);
		}
		fclose(f);
	//	printf("CANNOT Save centroids with learn_proc = %s!!!\n", LPROC_AUTOENC.c_str());
	}
}


bool mk_centroids::load(std::string fname)
{
	printf("mk_centroids: loading...\n"); fflush(stdout);

	if (learn_proc == LPROC_KMEANS || learn_proc == LPROC_RICA) {
		FILE *f = fopen(fname.c_str(), "r");
		if (f == NULL)
			return false;
		fscanf(f, "%d", &whitening);
		if (whitening) {
			M.read(f);
			P.read(f);
		}
		int n;
		fscanf(f, "%d", &n);
		inds.resize(n);
		cens.resize(n);
		for (size_t i = 0; i < inds.size(); ++i) {
			fscanf(f, "%d", &n);
			inds[i].resize(n);
			for (size_t j = 0; j < inds[i].size(); ++j)
				fscanf(f, "%d", &inds[i][j]);
			cens[i].read(f);
		}
		fclose(f);	
		return true;
	}
	if (learn_proc == LPROC_AUTOENC) {
		FILE *f = fopen(fname.c_str(), "r");
		if (f == NULL)
			return false;
		fscanf(f, "%d", &whitening);
		if (whitening) {
			M.read(f);
			P.read(f);
		}

		fscanf(f, "%d", &ae_layers);
		ae_w.resize(ae_layers);
		ae_b.resize(ae_layers);
		for (int layer = 0; layer < ae_layers; ++layer) {
			ae_w[layer].read(f);
			ae_b[layer].read(f);
		}

		fclose(f);
		return true;
	}
}



bool mk_centroids::run_whitening(matrix &patches)
{
	whitening = 1;

	int num = patches.rows();
	int size = patches.cols();
	int tot;

	// Compute Mean
	printf("whitening: computing mean...\n"); fflush(stdout);
	P.create(1, size);
	M.create(1, size);
	for (int i = 0; i < num; ++i)
		for (int j = 0; j < size; ++j)
			P.val(0, j) += patches.val(i, j);
	MPI_Allreduce(P.get_buf(), M.get_buf(), size, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
	MPI_Allreduce(&num, &tot, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
	M.mult(1. / (double)tot);
	P.free();

	// Compute Covariance
	printf("whitening: computing covariance...\n"); fflush(stdout);
	matrix cov, tmp;
	cov.create(size, num);
	tmp.create(num, size);
	for (int i = 0; i < num; ++i)
		for (int j = 0; j < size; ++j)
			cov.val(j, i) = tmp.val(i, j) = patches.val(i, j) - M.val(0, j);
	cov.multiply_right(tmp);
	tmp.free();
	tmp.copy(cov);
	MPI_Allreduce(tmp.get_buf(), cov.get_buf(), size * size, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);	
	cov.mult(1. / (double)(tot - 1));
	tmp.free();

	// Compute EigenVectors
	printf("whitening: computing eigen...\n"); fflush(stdout);
	double *w = new double[size];
	double *w_work, w_opt;
	int lda = size;
	int w_len, info;

	w_len = -1;
	dsyev("Vectors", "Upper", &size, cov.get_buf(), &lda, w, &w_opt, &w_len, &info);
	w_len = (int)w_opt;
	w_work = new double[w_len];
	dsyev("Vectors", "Upper", &size, cov.get_buf(), &lda, w, w_work, &w_len, &info);

	if (info > 0) {
		printf("cannot compute eigen!\n"); fflush(stdout);
		return false;
	}
	delete w_work;

	// Compute P
	printf("whitening: computing P...\n"); fflush(stdout);
	tmp.create(size, size);
	for (int i = 0; i < size; ++i)
			w[i] = sqrt(1. / (w[i] + 0.1));
	for (int i = 0; i < size; ++i)
		for (int j = 0; j < size; ++j)
			tmp.val(i, j) = cov.val(j, i) * w[j]; // ????????????????????
	P = tmp.multiply(cov); // ????????????????????
	tmp.free();
	cov.free();

	// Modifying patches
	printf("whitening: modifying patches...\n"); fflush(stdout);
	for (int i = 0; i < num; ++i)
		for (int j = 0; j < size; ++j)
			patches.val(i, j) -= M.val(0, j);
	patches.multiply_right(P);

	delete w;
	return true;
}


void mk_centroids::do_whitening(matrix &patches) const
{
	assert(whitening == 1);
	for (int i = 0; i < patches.rows(); ++i)
		for (int j = 0; j < patches.cols(); ++j)
			patches.val(i, j) -= M.get_val(0, j);
	patches.multiply_right(P);
}


void mk_centroids::add(const std::vector<int> &ind, const matrix &cen)
{
	assert(learn_proc == LPROC_KMEANS || learn_proc == LPROC_RICA);
	inds.push_back(ind);
	cens.push_back(cen);
}


void kmeans_map(matrix &patches, const matrix &cen)
{
	printf("    kmeans_map: computing z...\n"); fflush(stdout);
	int num = patches.rows();
	int patch_size = patches.cols();
	int centroid_n = cen.rows();
	printf("    kmeans_map: computing cen^T (mem: %.3lf GB)\n", system_info().memory()); fflush(stdout);
	matrix centroids_T = cen.transpose();

	printf("    kmeans_map: computed cen^T (mem: %.3lf GB)\n", system_info().memory()); fflush(stdout);

	double *p2 = new double[num];
	double *c2 = new double[centroid_n];
	memset(p2, 0, sizeof(double) * num);
	memset(c2, 0, sizeof(double) * centroid_n);
	for (int i = 0; i < num; ++i)
		for (int j = 0; j < patch_size; ++j)
			p2[i] += sqr(patches.val(i, j));
	for (int i = 0; i < patch_size; ++i)
		for (int j = 0; j < centroid_n; ++j)
			c2[j] += sqr(centroids_T.val(i, j));

	patches.multiply_right(centroids_T);
	for (int i = 0; i < num; ++i)
		for (int j = 0; j < centroid_n; ++j) {
			double x = p2[i] + c2[j] - 2. * patches.val(i, j);
			if (x > 0) x = sqrt(x); else x = 0.;
			patches.val(i, j) = x;
		}

	memset(p2, 0, sizeof(double) * num);
	for (int i = 0; i < num; ++i)
		for (int j = 0; j < centroid_n; ++j)
			p2[i] += patches.val(i, j);
	for (int i = 0; i < num; ++i) {
		double mu = p2[i] / (double)centroid_n;
		for (int j = 0; j < centroid_n; ++j) {
			double x = max(0., mu - patches.val(i, j));
			patches.val(i, j) = x;
		}
	}
	delete c2;
	delete p2;
	centroids_T.free();
	printf("    kmeans_map: finished. (mem: %.3lf GB)\n", system_info().memory()); fflush(stdout);
}


void rica_map(matrix &patches, const matrix &cen)
{
	printf("    rica_map: computing...\n"); fflush(stdout);

	int num = patches.rows();
	int centroid_n = cen.rows();
	matrix centroids_T = cen.transpose();
	double *p2 = new double[num];
	patches.multiply_right(centroids_T);
	memset(p2, 0, sizeof(double) * num);
	for (int i = 0; i < num; ++i)
		for (int j = 0; j < centroid_n; ++j)
			p2[i] += patches.val(i, j);
	for (int i = 0; i < num; ++i) {
		double mu = p2[i] / (double)centroid_n;
		for (int j = 0; j < centroid_n; ++j) {
			double x = max(0., mu - patches.val(i, j));
			patches.val(i, j) = x;
		}
	}
	delete p2;
	centroids_T.free();
}


void mk_centroids::map_comm(string learn_proc, matrix &patches) const
{
	int final_size = 0;
	for (size_t cur = 0; cur < inds.size(); ++cur)
		final_size += cens[cur].rows();

	matrix tmp;
	tmp.create(patches.rows(), final_size);
	int posi = 0;

	for (size_t cur = 0; cur < inds.size(); ++cur) {
		matrix p;
		p.create(patches.rows(), (int)inds[cur].size());
		for (int i = 0; i < p.rows(); ++i)
			for (int j = 0; j < p.cols(); ++j)
				p.val(i, j) = patches.val(i, inds[cur][j]);

		if (learn_proc == "KMeans")
			kmeans_map(p, cens[cur]);
		else if(learn_proc == "Rica")
			rica_map(p, cens[cur]);
		else {
			printf("cannot find learn_proc %s!!!\n", learn_proc.c_str());
		}

		assert(p.cols() == cens[cur].rows());
		for (int i = 0; i < p.rows(); ++i)
			for (int j = 0; j < p.cols(); ++j)
				tmp.val(i, posi + j) = p.val(i, j);
		posi += p.cols();
		p.free();
	}
	
	swap(tmp, patches);
	tmp.free();
}


void mk_centroids::map_autoenc(string learn_proc, matrix &patches) const
{
	for (int layer = 0; layer < ae_layers; ++layer) {
		matrix tmp = ae_w[layer].transpose();
		//printf("map_autoenc patches(%d,%d) tmp(%d,%d)\n",patches.rows(),patches.cols(),tmp.rows(),tmp.cols());
		patches.multiply_right(tmp);
		tmp.free();
		for (int i = 0; i < patches.rows(); ++i)
			for (int j = 0; j < patches.cols(); ++j) {
				patches.val(i, j) += ae_b[layer].get_val(0, j);
				if (patches.val(i, j) < 0)
					patches.val(i, j) = 0;
			}
	}

	int num = patches.rows();
	int centroid_n = patches.cols();
	double *p2 = new double[num];
	memset(p2, 0, sizeof(double) * num);
	for (int i = 0; i < num; ++i)
		for (int j = 0; j < centroid_n; ++j)
			p2[i] += patches.val(i, j);
	for (int i = 0; i < num; ++i) {
		double mu = p2[i] / (double)centroid_n;
		for (int j = 0; j < centroid_n; ++j) {
			double x = max(0., mu - patches.val(i, j));
			patches.val(i, j) = x;
		}
	}
	delete p2;
}


void mk_centroids::map(string learn_proc, matrix &patches) const
{
	if (learn_proc == LPROC_KMEANS || learn_proc == LPROC_RICA) {
		map_comm(learn_proc, patches);
	}
	if (learn_proc == LPROC_AUTOENC) {
		map_autoenc(learn_proc, patches);
	}
}


