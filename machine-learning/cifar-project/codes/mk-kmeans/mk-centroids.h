#pragma once
#include <string>
#include <vector>
#include "..\common\common.h"
#include "..\common\lib-matrix.h"


#include "mpi.h"
#pragma comment(lib, "msmpi.lib")


const std::string LPROC_KMEANS = "KMeans";
const std::string LPROC_RICA = "Rica";
const std::string LPROC_AUTOENC = "AutoEnc";


class mk_centroids
{
	std::string learn_proc;

	int whitening;
	matrix M, P;
	std::vector<std::vector<int> > inds;
	std::vector<matrix> cens;

	int ae_layers;
	std::vector<matrix> ae_w, ae_b;

	void map_comm(std::string learn_proc, matrix &patches) const;
	void map_autoenc(std::string learn_proc, matrix &patches) const;
public:
	mk_centroids(std::string learn_proc);
	void initialize();
	void save(std::string fname) const;
	bool load(std::string fname);
	bool run_whitening(matrix &patches);
	void do_whitening(matrix &patches) const;
	void add(const std::vector<int> &ind, const matrix &cen);
	void map(std::string learn_proc, matrix &patches) const;
};


//	if (config.learn_proc == "KMeans") {
//	} else if (config.learn_proc == "Rica") {
//		// !!!!!!!!!!!!!!!!!!!!!
//		// !!!!!!!!!!!!!!!!!!!!!
//		// !!!!!!!!!!!!!!!!!!!!!
//	} else {
//		printf("Cannot find learn_proc %s!\n", config.learn_proc.c_str());
//	}






//bool load_data(const mk_config &config, matrix &centroids_T, matrix &w_M, matrix &w_P)
//{
//	// Lock It: allow only io_limit threads to read at the same time
//	int flag;
//	MPI_Status status;
//	if (config.rank >= config.io_limit)		
//		MPI_Recv(&flag, 1, MPI_INT, config.rank - config.io_limit, 0, MPI_COMM_WORLD, &status);
//
//	FILE *f = fopen(config.cen_file.c_str(), "r");
//	if (f == NULL) {
//		printf("cannot open cen_file!\n");
//		return false;
//	}
//	int centroid_n;
//	int patch_size;
//	fscanf(f, "%d%d", &centroid_n, &patch_size);
//	if (centroid_n != config.centroid_num) {
//		printf("invalid config & cen: neq <centroid_num>!\n");
//		return false;
//	}
//	/*
//	if (patch_size != config.rf_size * config.rf_size * 3) {
//		printf("invalid config & cen: neq <patch_size>!\n");
//		return false;
//	}
//	*/
//	centroids_T.create(patch_size, centroid_n);
//	for (int i = 0; i < centroid_n; ++i)
//		for (int j = 0; j < patch_size; ++j)
//			fscanf(f, "%lf", &centroids_T.val(j, i));
//	int tmp;
//	fscanf(f, "%d", &tmp);
//	if (tmp != config.whitening) {
//		printf("invalid config & cen: neq <whitening>!\n");
//		return false;
//	}
//	if (config.whitening) {
//		w_M.create(1, patch_size);
//		for (int i = 0; i < patch_size; ++i)
//			fscanf(f, "%lf", &w_M.val(0, i));
//		w_P.create(patch_size, patch_size);
//		for (int i = 0; i < patch_size; ++i)
//			for (int j = 0; j < patch_size; ++j)
//				fscanf(f, "%lf", &w_P.val(i, j));
//	}
//	fclose(f);
//
//	// Unlock It
//	if (config.rank + config.io_limit < config.size)
//		MPI_Send(&flag, 1, MPI_INT, config.rank + config.io_limit, 0, MPI_COMM_WORLD);
//	return true;
//}

