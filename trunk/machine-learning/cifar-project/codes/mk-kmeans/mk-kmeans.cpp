#include <cstdlib>
#include <cstdio>
#include <ctime>
#include "../common/common.h"
#include "../common/lib-matrix.h"
#include "../common/lib-image.h"
#include "rica.h"
#include "mk-centroids.h"
#include <Windows.h>
using namespace std;

#include "mpi.h"
#pragma comment(lib, "msmpi.lib")

#define _USE_LOG_FILE
//#define _PSEUDO_RAND


void autoenc_save_patches(const mk_config &config, matrix &patches)
{
	assert(config.learn_proc == LPROC_AUTOENC);

	int patch_n = patches.rows();
	int patch_size = patches.cols();
	string fname_whitening = config.cen_file + "-whitening.txt";
	string fname_patch_bin = config.cen_file + "-patches.bin";
	string fname_patch_txt = config.cen_file + "-patches.txt";

	// Normalize Patches
	printf("save_patches: normalizing patches...\n"); fflush(stdout);
	printf("  - memory: %.3lf GB\n", system_info().memory()); fflush(stdout);
	for (int i = 0; i < patch_n; ++i)
		normalize_patch(patches.get_row_buf(i), patch_size);
	printf("  - memory: %.3lf GB\n", system_info().memory()); fflush(stdout);
	printf("\n");

	// Whitening
	printf("save_patches: whitening...\n"); fflush(stdout);
	printf("  - memory: %.3lf GB\n", system_info().memory()); fflush(stdout);
	mk_centroids centroids(config.learn_proc);
	centroids.initialize();
	if (config.whitening) {
		if (!centroids.run_whitening(patches))
			assert(false);
		printf("waiting...\n"); fflush(stdout);
		printf("  - memory: %.3lf GB\n", system_info().memory()); fflush(stdout);
		MPI_Barrier(MPI_COMM_WORLD);
		printf("save_patches: all finished whitening.\n"); fflush(stdout);
		printf("\n");
	} else {
		printf("save_patches: well i do not need whitening~\n"); fflush(stdout);
		printf("\n");
	}
	
	if (config.rank == 0) {
		printf("save_patches: saving whitening info...\n"); fflush(stdout);
		centroids.save(fname_whitening);
	} else {
		printf("save_patches: waiting for saving whitening info...\n"); fflush(stdout);
	}
	MPI_Barrier(MPI_COMM_WORLD);
	printf("saved.\n\n"); fflush(stdout);

	// Write Patches
	printf("save_patches: writing patches...\n"); fflush(stdout);
	int tot_size, _ = patches.rows();
	MPI_Allreduce(&_, &tot_size, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);

	int flag;
	MPI_Status status;
	if (config.rank >= 0)		
		MPI_Recv(&flag, 1, MPI_INT, config.rank - 1, 0, MPI_COMM_WORLD, &status);

	printf("save_patches: yet i am writing!\n"); fflush(stdout);

	FILE *f_txt, *f_bin;
	if (config.rank == 0) {
		f_txt = fopen(fname_patch_txt.c_str(), "w");
		f_bin = fopen(fname_patch_bin.c_str(), "wb");
		fprintf(f_txt, "%d %d\n", tot_size, patches.cols());
		fwrite(&tot_size, sizeof(int), 1, f_bin);
		_ = patches.cols(); fwrite(&_, sizeof(int), 1, f_bin);
	} else {
		f_txt = fopen(fname_patch_txt.c_str(), "a");
		f_bin = fopen(fname_patch_bin.c_str(), "ab");
	}

	float *buf = new float[patches.cols()];
	for (int i = 0; i < patches.rows(); ++i) {
		for (int j = 0; j < patches.cols(); ++j) {
			fprintf(f_txt, "%.9g ", patches.get_val(i, j));
			buf[j] = (float)patches.get_val(i, j);
		}
		fprintf(f_txt, "\n");
		fwrite(buf, sizeof(float) * patches.cols(), 1, f_bin);
	}
	delete buf;

	fclose(f_txt);
	fclose(f_bin);

	printf("save_patches: i m done.\n"); fflush(stdout);

	if (config.rank + 1 < config.size)
		MPI_Send(&flag, 1, MPI_INT, config.rank + 1, 0, MPI_COMM_WORLD);
	MPI_Barrier(MPI_COMM_WORLD);
	
	printf("save_patches: all done.\n"); fflush(stdout);
}


bool sample_patches(const mk_config &config, matrix &patches, vector<int> &labels)
{
	srand((unsigned int)(time(NULL) + config.rank * 10007));

	int flag;
	MPI_Status status;

	// Load Image List
	printf("loading image list...\n"); fflush(stdout);
	vector<string> list;
	if (!load_image_list(config.kmeans_imglst, config.size, config.rank, list))
		return false;
	for (int i = 0; i < (int)list.size(); ++i)
		list[i] = config.kmeans_imgpath + list[i];

	// Load Labels if Necessary
	vector<int> tmp_labels;
	if (config.mil_neg_cen > 0) {
		FILE *f = fopen(config.kmeans_lablst.c_str(), "r");
		if (f != NULL) {
			labels.clear();
			printf("reading labels...\n"); fflush(stdout);
			for (int x; fscanf(f, "%d", &x) == 1; )
				labels.push_back(x);
			fclose(f);
			printf("read %d labels...\n", labels.size()); fflush(stdout);
			int st = config.rank * images_per_thread;
			int en = (config.rank + 1) * images_per_thread;
			for (int i = st; i < min((int)labels.size(), en); ++i)
				tmp_labels.push_back(labels[i]);
			printf("successfully load labels!!! my size = %d.\n", tmp_labels.size()); fflush(stdout);
			assert(tmp_labels.size() == list.size());
		} else {
			printf("cannot read labels!!!!!!!!!\n"); fflush(stdout);
			printf("cannot read labels!!!!!!!!!\n"); fflush(stdout);
			printf("cannot read labels!!!!!!!!!\n"); fflush(stdout);
			printf("cannot read labels!!!!!!!!!\n"); fflush(stdout);
			printf("cannot read labels!!!!!!!!!\n"); fflush(stdout);
		}
	}
	labels.clear();

	// Lock It: allow only io_limit threads to read at the same time
	if (config.rank >= config.io_limit)		
		MPI_Recv(&flag, 1, MPI_INT, config.rank - config.io_limit, 0, MPI_COMM_WORLD, &status);
	
	// Load Images and Extract patches
	printf("loading images and extracting patches...\n"); fflush(stdout);
	int rf_size = config.rf_size;
	int patch_size;
	int patches_per_img = config.patch_num;

	int cur = 0;
	for (int i = 0; i < (int)list.size(); ++i) {
		cuboid cub;
		if (!cub.load(list[i])) {
			printf("load %s failed!\n", list[i].c_str()); fflush(stdout);
			continue;
		}
		if (cub.rows() > 2000) {
			printf("loaded %s.\n", list[i].c_str()); fflush(stdout);
			printf("  - memory: %.3lf GB\n", system_info().memory()); fflush(stdout);
		}
		if (i == 0) {
			patch_size = rf_size * rf_size * cub.len();
			patches.create((int)list.size() * patches_per_img, patch_size);
		}
#ifdef _PSEUDO_RAND
		int seed = 123456;
#endif
		for (int t = 0; t < patches_per_img; ++t) {
#ifdef _PSEUDO_RAND
			int r = seed % (cub.rows() - rf_size + 1); seed = (seed * 113 + 17) % 1000007;
			int c = seed % (cub.cols() - rf_size + 1); seed = (seed * 113 + 17) % 1000007;
#else
			//if (i == 0) printf("r=%d c=%d\n", r, c);
			int r = rand() % (cub.rows() - rf_size + 1);
			int c = rand() % (cub.cols() - rf_size + 1);
#endif
			cub.extract_patch(r, c, rf_size, patches.get_row_buf(cur));
			if (tmp_labels.size() > 0)
				labels.push_back(tmp_labels[i]);
			++cur;
		}
		cub.free();
	}

	// Unlock It
	if (config.rank + config.io_limit < config.size)
		MPI_Send(&flag, 1, MPI_INT, config.rank + config.io_limit, 0, MPI_COMM_WORLD);

	return true;
}


void kmeans_iteration(int batch_size, matrix &patches, matrix &centroids, int fixed_n, matrix &sum)
{
	int patch_n = patches.rows();
	int patch_size = patches.cols();
	int centroid_n = centroids.rows();
	assert(fixed_n >= 0 && fixed_n <= centroid_n);
	// c2 coeff
	double *c2 = new double[centroid_n];
	for (int i = 0; i < centroid_n; ++i) {
		c2[i] = 0.;
		for (int j = 0; j < patch_size; ++j)
			c2[i] += sqr(centroids.val(i, j));
		c2[i] /= 2.;
	}
	// sum and cnt
	int *cnt = new int[centroid_n];
	int *cnt2 = new int[centroid_n];
	memset(cnt, 0, sizeof(int) * centroid_n);
	sum.clear();
	matrix cp;
	for (int cur = 0; cur < patches.rows(); cur += batch_size) {
		int num = min(batch_size, patches.rows() - cur);
		cp.create(patch_size, num);
		for (int i = 0; i < num; ++i)
			for (int j = 0; j < patch_size; ++j)
				cp.val(j, i) = patches.val(cur + i, j);
		cp.multiply_left(centroids);
		for (int i = 0; i < num; ++i) {
			int max_j = 0;
			double max_val = -1E50;
			for (int j = 0; j < centroid_n; ++j) {
				double cur_val = cp.val(j, i) - c2[j];
				if (cur_val > max_val) {
					max_val = cur_val;
					max_j = j;
				}
			}
			cnt[max_j]++;
			for (int j = 0; j < patch_size; ++j)
				sum.val(max_j, j) += patches.val(cur + i, j);
		}
		cp.free();
	}
	// gather
	MPI_Allreduce(cnt, cnt2, centroid_n, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
	for (int i = fixed_n; i < centroid_n; ++i) {
		MPI_Allreduce(sum.get_row_buf(i), centroids.get_row_buf(i), patch_size,
					MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
		if (cnt2[i] > 0)
			for (int j = 0; j < patch_size; ++j)
				centroids.val(i, j) /= (double)cnt2[i];
	}
	delete c2;
	delete cnt;
	delete cnt2;
}


void kmeans_proc(const mk_config &config, matrix &patches, matrix &centroids, matrix &cen0)
{
	int patch_n = patches.rows();
	int patch_size = patches.cols();
	int centroid_n = config.centroid_num;
	if (cen0.exists()) {
		assert(cen0.rows() < centroid_n);
		assert(cen0.cols() == patch_size);
	}

//FILE *f = fopen("D:\\v-qifen\\MPIKmeans\\expr\\cifar-test\\patches.txt", "w");
//patches.write(f);
//fclose(f);

	// Initialize Centroids
	printf("kmeans_proc: initializing centroids...\n"); fflush(stdout);
	matrix sum;
	sum.create(centroid_n, patch_size);
	centroids.create(centroid_n, patch_size);
	
	for (int i = config.rank; i < centroid_n; i+= config.size) {
#ifdef _PSEUDO_RAND
		for (int j = 0; j < patch_size; ++j)
          sum.val(i, j) = (((i) * 1001 + (j) * 595) % 20007) / 10000. - 1.;
#else
		for (int j = 0; j < patch_size; ++j)
			sum.val(i, j) = randn() * 0.1;
#endif
	}
	MPI_Allreduce(sum.get_buf(), centroids.get_buf(), centroid_n * patch_size,
			MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
	if (cen0.exists()) {
		printf("kmeans_proc: copying fixed centroids.\n"); fflush(stdout);
		for (int i = 0; i < cen0.rows(); ++i)
			memmove(centroids.get_row_buf(i), cen0.get_row_buf(i), sizeof(double) * centroids.cols());
	}
	printf("kmeans_proc: all finished init centroids.\n"); fflush(stdout);

	// Do K-Means
	_timer timer(config.kmeans_iterations);
	for (int itr = 1; itr <= config.kmeans_iterations; ++itr) {
		system_info s_info;
		_timer ctm;
		int f_verbose = (centroid_n <= 100 ? 10 : 1);
		bool b_verbose = (itr % f_verbose == 0);
		if (b_verbose) {
			printf("K-Means iteration %d / %d ", itr, config.kmeans_iterations);
			printf("(remain: %s)\n", timer.predict(itr - 1)); fflush(stdout);
		}
		if (cen0.exists()) {
			kmeans_iteration(config.kmeans_batch, patches, centroids, cen0.rows(), sum);
		} else {
			kmeans_iteration(config.kmeans_batch, patches, centroids, 0, sum);
		}
		if (b_verbose) {
			printf("  # current gather time: %s\n", ctm.total());
			printf("  # cpu rate = %.1lf%%\n", s_info.cpu_rate());
			printf("  # memory: %.3lf GB\n", s_info.memory());
		}
	}
	sum.free();
}


void kmeans_mil_proc(const mk_config &config, matrix &patches, const vector<int> &labels, matrix &centroids)
{
	assert(patches.rows() == (int)labels.size());

	printf("kmeans_mil_proc...\n"); fflush(stdout);

	// Get Negative Centroids
	assert(config.mil_neg_cen > 0 && config.mil_neg_cen <= config.centroid_num);
	printf("\nkmeans_mil_proc: getting negative centroids...\n"); fflush(stdout);
	int n_neg = 0;
	for (int i = 0; i < patches.rows(); ++i)
		if (labels[i] == 0)
			++n_neg;
	matrix p0, c0, junk;
	mk_config tmp_config = config;
	tmp_config.centroid_num = config.mil_neg_cen;
	p0.create(max(1, n_neg), patches.cols());
	for (int i = 0, cur=0; i < patches.rows(); ++i)
		if (labels[i] == 0) {
			memmove(p0.get_row_buf(cur), patches.get_row_buf(i), sizeof(double) * patches.cols());
			++cur;
		}
	kmeans_proc(tmp_config, p0, c0, junk);
	p0.free();

	// Get All Positive Centroids
	printf("\nkmeans_mil_proc: getting all centroids...\n"); fflush(stdout);
	kmeans_proc(config, patches, centroids, c0);
	c0.free();

	printf("kmeans_mil_proc finished.\n"); fflush(stdout);
}


void learn_round(const mk_config &config, int num, matrix &patches, const vector<int> &labels,
	const vector<int> &ind, mk_centroids &centroids)
{
	printf("\nLEARN ROUND : src = %d tar = %d\n", ind.size(), num); fflush(stdout);

	// Select Current Patches
	matrix p;
	p.create(patches.rows(), (int)ind.size());
	for (int i = 0; i < p.rows(); ++i)
		for (int j = 0; j < p.cols(); ++j)
			p.val(i, j) = patches.val(i, ind[j]);

	// Compute Current Centroids
	mk_config tmp_config = config;
	tmp_config.centroid_num = num;

	matrix cen;
	if (config.learn_proc == "KMeans") {
		if (labels.size() > 0)
			kmeans_mil_proc(tmp_config, p, labels, cen);
		else {
			matrix junk;
			kmeans_proc(tmp_config, p, cen, junk);
		}
	} else if (config.learn_proc == "Rica") {
		rica::rica_proc(tmp_config, p, cen);
	} else {
		printf("Cannot find learn_proc %s!\n", config.learn_proc.c_str());
		assert(false);
	}
	p.free();

	// Add Centroids Data
	centroids.add(ind, cen);
}

/*
double get_square_avg(const matrix &patches, int j)
{
	assert(j >= 0 && j < patches.cols());
	double mu = 0.;
	for (int i = 0; i < patches.rows(); ++i)
		mu += sqr(patches.get_val(i, j));
	mu /= (double)patches.rows();
	return mu;
}
*/


struct similarity_info {
	double a, b, c;
	similarity_info()
		: a(0), b(0), c(0) { }
	similarity_info(double a, double b, double c)
		: a(a), b(b), c(c) { }
	double value() const {
		assert(a * b > 1E-10);
		return c / sqrt(a * b);
	}
};

similarity_info compute_similarity(const matrix &patches, int j, int k)
{
	/*
	double a = 0.;
	double b = 0.;
	double c = 0.;
	double mu_x = get_square_avg(patches, j);
	double mu_y = get_square_avg(patches, k);
	for (int i = 0; i < patches.rows(); ++i) {
		double x = sqr(patches.get_val(i, j));
		double y = sqr(patches.get_val(i, k));
		a += sqr(x - mu_x);
		b += sqr(y - mu_y);
		c += (x - mu_x) * (y - mu_y);
	}
	if (a * b < 1E-12)
		return 0;
	return c / sqrt(a * b);
	*/
	similarity_info info;
	for (int i = 0; i < patches.rows(); ++i) {
		double x = sqr(patches.get_val(i, j));
		double y = sqr(patches.get_val(i, k));
		info.a += x * x - 1;
		info.b += y * y - 1;
		info.c += x * y - 1;
	}
	return info;
}


vector<int> select_rf(const matrix &patches, int dim, int num)
{
	printf("select_rf: waiting into selecting RF...\n"); fflush(stdout);
	MPI_Barrier(MPI_COMM_WORLD);
	printf("select_rf: selecting RF patches(%d,%d) dim=%d num=%d...\n", patches.rows(), patches.cols(), dim, num); fflush(stdout);
	int mpi_rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
	int width = patches.cols() / dim;
	assert(patches.cols() % dim == 0);

/*/
vector<int> rfs;
for (int i = 0; i < num; ++i) {
	int k = rand() % dim;
	for (int j = k * width; j < (k + 1) * width; ++j)
		rfs.push_back(j);
} return rfs;
/*/

	// Select The Seed Position
	int z0, tmp;
	if (mpi_rank == 0)
		z0 = rand() % patches.cols();
	else
		z0 = 0;
	MPI_Allreduce(&z0, &tmp, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
	z0 = tmp;

	// Computing The Similarities
	printf("select_rf: computing similarities...\n");
	double *sa = new double[patches.cols()];
	double *sb = new double[patches.cols()];
	double *sc = new double[patches.cols()];
	double *stmp = new double[patches.cols()];
	for (int i = 0; i < patches.cols(); ++i) {
		similarity_info info = compute_similarity(patches, i, z0);
		sa[i] = info.a;
		sb[i] = info.b;
		sc[i] = info.c;
	}
	MPI_Allreduce(sa, stmp, patches.cols(), MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
	memmove(sa, stmp, patches.cols() * sizeof(double));
	MPI_Allreduce(sb, stmp, patches.cols(), MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
	memmove(sb, stmp, patches.cols() * sizeof(double));
	MPI_Allreduce(sc, stmp, patches.cols(), MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
	memmove(sc, stmp, patches.cols() * sizeof(double));

	vector<pair<double, int> > list;
	for (int i = 0; i < dim; ++i)
		list.push_back(make_pair(0, i));
	assert(patches.cols() % width == 0 && patches.cols() / width == dim);
	for (int i = 0; i < patches.cols(); ++i)
		list[i / width].first -= similarity_info(sa[i], sb[i], sc[i]).value();
	delete sa;
	delete sb;
	delete sc;
	delete stmp;

	// Selecting Closest Columns
	printf("select_rf: selecting closest columns...\n");
	int *flag_1 = new int[dim];
	int *flag_2 = new int[dim];
	memset(flag_1, 0, sizeof(int) * dim);
	if (mpi_rank == 0) {
		sort(list.begin(), list.end());
		for (int i = 0; i < min(num, dim); ++i)
			flag_1[list[i].second] = 1;
	}
	MPI_Allreduce(flag_1, flag_2, dim, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
	vector<int> rfs;
	for (int i = 0; i < dim; ++i)
		if (flag_2[i]) {
			for (int j = i * width; j < (i + 1) * width; ++j)
				rfs.push_back(j);
		}
	delete flag_1;
	delete flag_2;

	// Finish
	printf("select_rf: waiting finishing selecting RF...\n"); fflush(stdout);
	MPI_Barrier(MPI_COMM_WORLD);
	printf("select_rf: finished selecting RF.\n"); fflush(stdout);
	return rfs;
//*/
}


bool main_proc(const mk_config &config, matrix &patches, mk_centroids &centroids, const vector<int> &labels)
{
	int patch_n = patches.rows();
	int patch_size = patches.cols();
	int centroid_n = config.centroid_num;
//
//FILE *f = fopen("D:\\v-qifen\\MPIKmeans\\expr\\cifar-test\\patches-0.txt", "w");
//patches.write(f);
//fclose(f);

	centroids.initialize();

	printf("\n");
	printf("START: patch_num=%d patch_size=%d centroid_n=%d\n", patch_n, patch_size, centroid_n);
	fflush(stdout);

	// Normalize Patches
	printf("main_proc: normalizing patches...\n"); fflush(stdout);
	printf("  - memory: %.3lf GB\n", system_info().memory()); fflush(stdout);
	for (int i = 0; i < patch_n; ++i)
		normalize_patch(patches.get_row_buf(i), patch_size);
	printf("  - memory: %.3lf GB\n", system_info().memory()); fflush(stdout);
	printf("\n");

	// Whitening
	printf("main_proc: whitening...\n"); fflush(stdout);
	printf("  - memory: %.3lf GB\n", system_info().memory()); fflush(stdout);
	if (config.whitening)
		if (!centroids.run_whitening(patches))
			return false;
	printf("waiting...\n"); fflush(stdout);
	printf("  - memory: %.3lf GB\n", system_info().memory()); fflush(stdout);
	MPI_Barrier(MPI_COMM_WORLD);
	printf("main_proc: all finished whitening.\n"); fflush(stdout);
	printf("\n");

	// Process
	printf("main_proc: main procedure...\n"); fflush(stdout);
	printf("main_proc: config.cen_round_src = %d\n", config.cen_round_src); fflush(stdout);
	printf("\n");

	if (config.cen_round_src < 0) {
		vector<int> ind;
		for (int i = 0; i < patches.cols(); ++i)
			ind.push_back(i);
		learn_round(config, config.centroid_num, patches, labels, ind, centroids);
	} else {
		int round_num = config.centroid_num / config.cen_round_tar;
		_timer timer(round_num);
		for (int round = 0; round < round_num; ++round) {
			printf("\nSELECTING ROUND %d / %d [remain %s]\n", round+1, round_num, timer.predict(round)); fflush(stdout);
			int original_dim = patches.cols() / config.rf_size / config.rf_size;
			vector<int> ind = select_rf(patches, original_dim, config.cen_round_src);
			learn_round(config, config.cen_round_tar, patches, labels, ind, centroids);
		}
	}

	// Return
	return true;
}


void _main(int argc, char *argv[])
{
	if (argc != 2 && argc != 3) {
		printf("usage: %s ini_file [-patch]\n", argv[0]);
		return;
	}

	// Get Basic Informations
	int size, rank, flag;
	MPI_Status status;
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	printf("## CUR_THREAD_ID = %d / %d\n", rank, size); fflush(stdout);
	system_info global_sinfo;

	bool b_gen_patches = false;
	if (argc == 3) {
		if (strcmp(argv[2], "-patch") == 0) {
			b_gen_patches = true;
		} else {
			printf("ERROR! Unknown parameter %s!\n", argv[2]);
			return;
		}
	}

	// Read Configuration File
	bool ini_flag = true;
	if (rank != 0)
		MPI_Recv(&flag, 1, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, &status);
	mk_config config;
	if(!config.load_from_ini(string(argv[1]), size, rank)) {
		printf("cannot load config from '%s'!\n", argv[1]);
		ini_flag = false;
	}
	if (rank != size - 1)
		MPI_Send(&flag, 1, MPI_INT, rank + 1, 0, MPI_COMM_WORLD);
	
	if (!ini_flag) {
		printf("read ini fail!\n"); fflush(stdout);
		return;
	}
	
#ifdef _USE_LOG_FILE
	printf("start logging. see log file %s for details.\n", config.log_path.c_str()); fflush(stdout);
	freopen(config.log_path.c_str(), "w", stdout);
#endif

	_timer timer;
	time_t rawtime;
	struct tm *timeinfo;
	time(&rawtime);
	timeinfo = gmtime(&rawtime);
	printf("## MK-KMeans\n"); fflush(stdout);
	printf("## CUR_THREAD_ID = %d / %d\n", rank, size); fflush(stdout);
	printf("## Current UTC date/time is: %s", asctime(timeinfo)); fflush(stdout);
	printf("\n"); fflush(stdout);

	printf("config.path  = %s\n", config.path.c_str());
	printf("  kmeans.list         = %s\n", config.kmeans_imglst.c_str());
	printf("  kmeans.cen_file     = %s\n", config.cen_file.c_str());
	printf("  kmeans.patch_num    = %d\n", config.patch_num);
	printf("  kmeans.rf_size      = %d\n", config.rf_size);
	printf("  kmeans.centroid_num = %d\n", config.centroid_num);
	printf("  kmeans.whitening    = %d\n", config.whitening);
	printf("\n");

	// Sample Patches
	matrix patches;
	vector<int> labels;
	if (!sample_patches(config, patches, labels)) {
		printf("fatal error: cannot sample patches!\n"); fflush(stdout);
		return;
	}
	printf("waiting...\n"); fflush(stdout);
	printf("  - memory: %.3lf GB\n", system_info().memory()); fflush(stdout);
	MPI_Barrier(MPI_COMM_WORLD);
	printf("finished sampling.\n"); fflush(stdout);

	// If I'm here to generate the patches ~~
	if (b_gen_patches) {
		autoenc_save_patches(config, patches);
		return;
	}

	// Do Feature Learning Process
	mk_centroids centroids(config.learn_proc);
	if (!main_proc(config, patches, centroids, labels)) {
		printf("fatal error: cannot run kmeans!\n"); fflush(stdout);
		return;
	}
	printf("\nwaiting...\n"); fflush(stdout);
	MPI_Barrier(MPI_COMM_WORLD);
	printf("finished feature learning.\n\n"); fflush(stdout);

	// Save Centroids
	if (rank == 0) {
		printf("saving centroids...\n"); fflush(stdout);
		centroids.save(config.cen_file);
	} else {
		printf("waiting for saving...\n"); fflush(stdout);
	}
	MPI_Barrier(MPI_COMM_WORLD);

	// Finished
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	printf("\n");
	printf("## Finished.\n"); fflush(stdout);
	printf("## The current date/time is: %s\n", asctime(timeinfo)); fflush(stdout);
	printf("## Cost time: %s.\n", timer.total());
	printf("## Avg cpu rate: %.3lf%%\n", global_sinfo.cpu_rate());
}


int main(int argc, char *argv[])
{
	MPI_Init(&argc, &argv);	
	_main(argc, argv);
	MPI_Finalize();	
	return 0;
}