#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <mkl.h>
#include "..\common\common.h"
#include "..\common\lib-matrix.h"
#include "..\common\lib-image.h"
#include "..\mk-kmeans\mk-centroids.h"
#include <Windows.h>
#include <set>
using namespace std;

#include "mpi.h"
#pragma comment(lib, "msmpi.lib")

#define _USE_LOG_FILE

const int MAX_THREAD_N = 8;
int THREAD_N = MAX_THREAD_N;
double min_memory;

FILE *global_log;

//int matrix::__cnt = 0;


bool extract_feature(const cuboid &img, const mk_config &config, const mk_centroids &centroids, cuboid &cub)
{
	printf("    extract_feature img=(%d, %d) cen_n=%d...\n", img.rows(), img.cols(), config.centroid_num); fflush(stdout);
	printf("       - memory: %.3lf GB\n", system_info().memory()); fflush(stdout);
	// Initialize
	int rf_size = config.rf_size;
	int patch_size = rf_size * rf_size * img.len();
	int centroid_n = config.centroid_num;
	
	int rows = img.rows() - rf_size + 1;
	int cols = img.cols() - rf_size + 1;
	int num = rows * cols;

	// Extract Patches
	//printf("    extract: make patches...\n"); fflush(stdout);
	matrix patches;
	patches.create(num, patch_size);
	for (int x = 0; x < rows; ++x)
		for (int y = 0; y < cols; ++y) {
			int i = x * cols + y;
			img.extract_patch(x, y, rf_size, patches.get_row_buf(i));
			//if (config.learn_proc == LPROC_KMEANS || config.learn_proc == LPROC_RICA)
			normalize_patch(patches.get_row_buf(i), patch_size);
		}
	printf("     - finished extracting - memory: %.3lf GB\n", system_info().memory()); fflush(stdout);

	// Whitening
	if (config.whitening) {
		//printf("    extract: whitening patches...\n");
		_timer timer;
		centroids.do_whitening(patches);
		//printf("    whitening time: %s\n", timer.total());
		printf("     - finished whitening - memory: %.3lf GB\n", system_info().memory()); fflush(stdout);
	} else {
		printf("     - we do not need whitening~\n"); fflush(stdout);
	}
	

	// Computing
	//printf("    extract: mapping...\n"); fflush(stdout);
	centroids.map(config.learn_proc, patches);

	// Finalizing
	//printf("    extract: processing...\n"); fflush(stdout);
	//cub.create(img.rows() - rf_size + 1, img.cols() - rf_size + 1, centroid_n);
	cub.assign(patches);
	min_memory = std::min(min_memory, system_info().memory());
	patches.free();
	
	return true;
}


struct extract_data
{
	mk_config config;
	cuboid img;
	mk_centroids *centroids_ptr;
	cuboid cub;
};


DWORD WINAPI extract_task(LPVOID lpParam)
{
	extract_data *data = (extract_data*)lpParam;
	if (!extract_feature(data->img, data->config, *(data->centroids_ptr), data->cub))
		printf("THREAD: fail to extract!\n"); fflush(stdout);
	return 0;
}


struct node_result
{
	mk_config config;
	int dim;
	double *d;
	float *f;
	FILE *ftxt, *fbin, *flst;
	node_result(const mk_config &config)
	{
		this->config = config;
		dim = config.centroid_num * config.ext_fea_pool * config.ext_fea_pool;
		d = new double[dim];
		f = new float[dim];
		ftxt = fopen((config.fea_file + ".txt" + i_to_s(config.rank)).c_str(), "w");
		fbin = fopen((config.fea_file + ".bin" + i_to_s(config.rank)).c_str(), "wb");
		flst = fopen((config.fea_list + i_to_s(config.rank)).c_str(), "w");

		if (config.rank == 0)
			fwrite(&dim, sizeof(int), 1, fbin);
	}
	~node_result() {
		delete d;
		delete f;
	}
	void push(string img_file, const cuboid &cub, int lab)
	{
		cuboid tmp;
		
		printf("result::push: pooling fea...\n"); fflush(stdout);
		tmp.pool_from(cub, config.ext_fea_pool);
		tmp.to_vector(d);
		tmp.free();

		// save to txt file
		printf("result::push: saving txt...\n"); fflush(stdout);
		fprintf(ftxt, "%d", lab);
		for (int j = 0; j < dim; ++j)
			fprintf(ftxt, " %.9g", d[j]);
			//if (d[j] != 0)
			//	fprintf(ftxt, " %d:%.9g", j + 1, d[j]);
		fprintf(ftxt, "\n");

//for (int i = 0; i < 1600; ++i) fprintf(ftxt, "%.9g ", cub.__get(0, 0, i)); fprintf(ftxt, "\n");

		// save to bin file
		printf("result::push: saving bin...\n"); fflush(stdout);
		for (int j = 0; j < dim; ++j)
			f[j] = (float)d[j];
		fwrite(&lab, sizeof(int), 1, fbin);
		fwrite(f, sizeof(float) * dim, 1, fbin);

		// save to cub file
		if (config.ext_cub_pool > 0) {
			printf("result::push: saving cub [%s]...\n", (config.fea_folder + img_file + ".cub").c_str()); fflush(stdout);
			tmp.pool_from(cub, config.ext_cub_pool);
			if (!tmp.save(config.fea_folder + img_file + ".cub"))
				{printf("result::push cannot save cub!!\n"); fflush(stdout);}
			tmp.free();
		}

		// save to lst file
		printf("result::push: saving lst...\n"); fflush(stdout);
		fprintf(flst, "%s\n", (img_file + ".cub").c_str()); fflush(flst);

		printf("result::push: finished.\n"); fflush(stdout);
	}
	void finish()
	{
		fclose(ftxt);
		fclose(fbin);
		fclose(flst);
	}
	void __merge(string fname)
	{		
		printf("merging %s...\n", fname.c_str()); fflush(stdout);
		FILE *f = fopen(fname.c_str(), "wb");
		const int buflen = 1024 * 1024;
		char *buf = new char[buflen];
		for (int r = 0; r < config.size; ++r) {
			FILE *fin = fopen((fname + i_to_s(r)).c_str(), "rb");
			while (1) {
				size_t n = fread(buf, 1, buflen, fin);
				if (n == 0)
					break;
				fwrite(buf, 1, n, f);
			}
			fclose(fin);
			if (!DeleteFile((fname + i_to_s(r)).c_str()))
				{printf("cannot delete %s!\n", (fname + i_to_s(r)).c_str()); fflush(stdout);}
		}
		delete buf;
		fclose(f);
	}
	void merge()
	{
		if (config.rank == 0) {
			printf("merging results....\n"); fflush(stdout);
			__merge(config.fea_file + ".txt");
			__merge(config.fea_file + ".bin");
			__merge(config.fea_list);
			//fprintf(f, "## Feature file generated in %s.\n", asctime(timeinfo));
			//fprintf(f, "##              generated by mk-extract.\n");
			//fprintf(f, "# dimension = %d\n", config.centroid_num * 4);
			//fprintf(f, "# num       = %d\n", config.centroid_num * 4);
		} else {
			printf("waiting for merging...\n"); fflush(stdout);
		}
	}
};


void extr_small_images(const mk_config &config, vector<string> list, vector<int> labels, mk_centroids &centroids)
{	
	printf("$$ extracting small images now (#=%d)\n", list.size()); fflush(stdout);
	// Extract Features
	node_result my_result(config);
	_timer tm2((int)list.size());
	system_info info;

	extract_data data[MAX_THREAD_N];
	HANDLE threads[MAX_THREAD_N];
	bool allocated[MAX_THREAD_N];
	memset(allocated, false, sizeof(allocated));
	THREAD_N = 1;
	int num;

	assert(labels.size() >= list.size());
	for (int cur = 0; cur < (int)list.size(); cur += num) {
		num = THREAD_N;
		printf("\n"); fflush(stdout);
		_timer ctm;
		system_info s_info;
		num = min((int)(list.size() - cur), THREAD_N);
		printf("extracting %d - %d (/ %d) (remain %s)\n", cur, cur + num - 1, (int)list.size(), tm2.predict(cur)); fflush(stdout);
		min_memory = s_info.memory();
		double max_memory = s_info.memory();

		
		mkl_set_num_threads(MAX_THREAD_N / num);
		
		if (cur == 0) {
			int flag;
			MPI_Status status;
			// Lock It: allow only io_limit threads to read at the same time
			if (config.rank >= config.io_limit)		
				MPI_Recv(&flag, 1, MPI_INT, config.rank - config.io_limit, 0, MPI_COMM_WORLD, &status);
		}
		for (int i = 0; i < num; ++i) {
			cuboid img;
			if (!img.load(config.extract_imgpath + list[cur + i])) {
				printf("load %s failed!\n", config.extract_imgpath + list[cur + i].c_str()); fflush(stdout);
				return;
			}

			data[i].img = img;
			data[i].config = config;
			data[i].centroids_ptr = &centroids;
			
			int rows = img.rows() - config.rf_size + 1;
			int cols = img.cols() - config.rf_size + 1;
			if (allocated[i] && (data[i].cub.rows() != rows || data[i].cub.cols() != cols)) {
				data[i].cub.free();
				allocated[i] = false;
			}
			if (!allocated[i]) {
				data[i].cub.create(rows, cols, config.centroid_num);
				allocated[i] = true;
			}
			
			DWORD t_id;
			threads[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)extract_task, (LPVOID)&data[i], 0, &t_id);
			if (threads[i] == NULL) {
				printf(" - unable to create thread!!! error=%d\n", GetLastError()); fflush(stdout);
				exit(1);
			}
		}
		if (cur == 0) {
			int flag;
			// Unlock It
			if (config.rank + config.io_limit < config.size)
				MPI_Send(&flag, 1, MPI_INT, config.rank + config.io_limit, 0, MPI_COMM_WORLD);
		}
		system_info comp_info;
		WaitForMultipleObjects(num, threads, TRUE, INFINITE);
		for (int i = 0; i < num; ++i)
			CloseHandle(threads[i]);
		double comp_cpu_rate = comp_info.cpu_rate();
		
		printf("@ max mem: %.3lf GB\n", s_info.peak_memory()); fflush(stdout);
		printf("@ memory: %.3lf GB\n", s_info.memory()); fflush(stdout);
		printf("pushing results...\n"); fflush(stdout);
		for (int i = 0; i < num; ++i) {
		//printf("pushing %d @ memory: %.3lf GB\n", i, s_info.memory());
			data[i].img.free();
			my_result.push(list[cur + i], data[i].cub, labels[cur + i]);
		//printf("pushed %d @ memory: %.3lf GB\n", i, s_info.memory());
		}
		
		//MKL_FreeBuffers();
		printf("# current extract time: %s\n", ctm.total()); fflush(stdout);
		printf("# cpu rate = %.1lf%%\n", s_info.cpu_rate()); fflush(stdout);
		printf("# comp. cpu = %.1lf%%\n", comp_cpu_rate); fflush(stdout);
		printf("# memory: %.3lf GB\n", s_info.memory()); fflush(stdout);

		if (cur == 0) {
			double used = s_info.peak_memory();
			int n = THREAD_N;
			while (n < MAX_THREAD_N && used / THREAD_N * (n + 1.5) < max_memory)
				++n;
			THREAD_N = n;
		}
	}
	my_result.finish();

	// Images
	double cpu_rate = info.cpu_rate();
	double all_cpu_rate;
	MPI_Allreduce(&cpu_rate, &all_cpu_rate, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
	all_cpu_rate /= (double)config.size;
	printf("\n");
	printf("# computation cpu rate = %.1lf%%\n", cpu_rate); fflush(stdout);
	printf("# global comp. cpu rate = %.1lf%%\n", all_cpu_rate); fflush(stdout);

	// Merge all Features
	printf("\n"); fflush(stdout);
	printf("waiting...\n"); fflush(stdout);
	MPI_Barrier(MPI_COMM_WORLD);
	my_result.merge();
}


struct large_image_result
{
	int pool;
	int dim;
	double *d;
	float *f;
	FILE *ftxt, *fbin;
	large_image_result(string out_file, int centroid_num, int pool)
	{
		this->pool = pool;
		dim = centroid_num * pool * pool;
		d = new double[dim];
		f = new float[dim];
		ftxt = fopen((out_file + ".txt").c_str(), "w");
		fbin = fopen((out_file + ".bin").c_str(), "wb");
	}
	~large_image_result() {
		delete d;
		delete f;
	}
	void push(const cuboid &cub, int lab)
	{
		printf("result::push: begin push...\n"); fflush(stdout);

		cuboid tmp;
		
		printf("result::push: pooling fea...\n"); fflush(stdout);
		tmp.pool_from(cub, pool);
		tmp.to_vector(d);
		tmp.free();

		// save to txt file
		printf("result::push: saving txt...\n"); fflush(stdout);
		fprintf(ftxt, "%d", lab);
		for (int j = 0; j < dim; ++j)
			if (d[j] != 0)
				fprintf(ftxt, " %d:%.9g", j + 1, d[j]);
		fprintf(ftxt, "\n");

		// save to bin file
		printf("result::push: saving bin...\n"); fflush(stdout);
		for (int j = 0; j < dim; ++j)
			f[j] = (float)d[j];
		fwrite(&lab, sizeof(int), 1, fbin);
		fwrite(f, sizeof(float) * dim, 1, fbin);

		fflush(ftxt);
		fflush(fbin);
		printf("result::push: finished.\n"); fflush(stdout);
	}
	void finish()
	{
		fclose(ftxt);
		fclose(fbin);
	}
};


void extr_large_images(const mk_config &config, vector<string> list, vector<int> labels, mk_centroids &centroids)
{
	printf("\n$$ extracting LARGE images now (#=%d)\n", list.size()); fflush(stdout);
	
	assert(labels.size() >= list.size());
	for (int ind = 0; ind < (int)list.size(); ++ind) {
		printf("\n"); fflush(stdout);
		system_info s_info;
		_timer ctm;
		printf("extracting LARGE %d / %d\n", ind+1, list.size()); fflush(stdout);
		printf("loading '%s'...\n", (config.extract_imgpath + list[ind]).c_str()); fflush(stdout);
		printf(" - memory: %.3lf GB\n", s_info.memory()); fflush(stdout);


		if (ind == 0) {
			int flag;
			MPI_Status status;
			// Lock It: allow only io_limit threads to read at the same time
			if (config.rank >= config.io_limit)		
				MPI_Recv(&flag, 1, MPI_INT, config.rank - config.io_limit, 0, MPI_COMM_WORLD, &status);
		}
		//cuboid img;
		Mat img = imread(config.extract_imgpath + list[ind]);
		//if (!img.load(config.extract_imgpath + list[ind])) {
		if (!img.data) {
			printf("load %s failed!\n", config.extract_imgpath + list[ind].c_str()); fflush(stdout);
			return;
		}
		if (ind == 0) {
			int flag;
			// Unlock It
			if (config.rank + config.io_limit < config.size)
				MPI_Send(&flag, 1, MPI_INT, config.rank + config.io_limit, 0, MPI_COMM_WORLD);
		}
		printf("loaded. [time = %s]\n", ctm.total()); fflush(stdout);
		printf(" - memory: %.3lf GB\n", s_info.memory()); fflush(stdout);
		
		large_image_result result(config.fea_folder + list[ind], config.centroid_num, config.ext_fea_pool);
		int psize = config.ext_largeimg_w;
		int delta = config.ext_largeimg_d;
		vector<pair<int, int> > tasks;
		for (int x = 0; x + psize - 1 < img.rows; x += delta)
			for (int y = 0; y + psize - 1 < img.cols; y += delta)
				tasks.push_back(make_pair(x, y));
		ctm = _timer((int)tasks.size());

		extract_data data[MAX_THREAD_N];
		HANDLE threads[MAX_THREAD_N];
		bool allocated[MAX_THREAD_N];
		memset(allocated, false, sizeof(allocated));
		THREAD_N = 1;
		int num;
		for (size_t cur = 0; cur < tasks.size(); cur += num) {
			num = THREAD_N;
			if (cur + num > tasks.size())
				num = (int)(tasks.size() - cur);
			printf("extracting %d - %d (/ %d)... [remain: %s]\n", (int)cur, (int)cur + num - 1, (int)tasks.size(), ctm.predict((int)cur)); fflush(stdout); 
			printf(" - memory: %.3lf GB\n", s_info.memory()); fflush(stdout);
			printf(" - cpu   : %.1lf%%\n", s_info.cpu_rate()); fflush(stdout);
			min_memory = s_info.memory();
			double max_memory = s_info.memory();
			double old_memory = s_info.peak_memory();

			mkl_set_num_threads(MAX_THREAD_N / num);

			for (int i = 0; i < num; ++i) {
				int x = tasks[cur + i].first;
				int y = tasks[cur + i].second;
				//data[i].img = img.extract_patch(x, y, psize);
				//printf("data[%d].assign_patch...\n", i); fflush(stdout);
				//printf("  -- memory: %.3lf GB\n", s_info.memory()); fflush(stdout);
				data[i].img.assign_patch(img, x, y, psize);
				//printf("data[%d].assign_patch finished.\n", i); fflush(stdout);
				//printf("  -- memory: %.3lf GB\n", s_info.memory()); fflush(stdout);
				data[i].config = config;
				data[i].centroids_ptr = &centroids;

				int rows = data[i].img.rows() - config.rf_size + 1;
				int cols = data[i].img.cols() - config.rf_size + 1;
				if (allocated[i] && (data[i].cub.rows() != rows || data[i].cub.cols() != cols)) {
					data[i].cub.free();
					allocated[i] = false;
				}
				if (!allocated[i]) {
					data[i].cub.create(rows, cols, config.centroid_num);
					allocated[i] = true;
				}

				//printf("  #7- memory: %.3lf GB\n", s_info.memory()); fflush(stdout);

				DWORD t_id;
				threads[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)extract_task, (LPVOID)&data[i], 0, &t_id);
				if (threads[i] == NULL) {
					printf(" - unable to create thread!!! error=%d\n", GetLastError()); fflush(stdout);
					exit(1);
				}
				//printf("  #8- memory: %.3lf GB\n", s_info.memory()); fflush(stdout);
			}
			WaitForMultipleObjects(num, threads, TRUE, INFINITE);
			for (int i = 0; i < num; ++i)
				CloseHandle(threads[i]);
			printf("finish current batch.\n"); fflush(stdout);

			printf("saving...\n"); fflush(stdout);
			for (int i = 0; i < num; ++i) {
				printf("i=%d num=%d\n", i, num); fflush(stdout);
				printf("freeing img memory: %.3lf GB\n", s_info.memory()); fflush(stdout);
				data[i].img.free();
				printf("freed   img memory: %.3lf GB\n", s_info.memory()); fflush(stdout);
				result.push(data[i].cub, labels[ind]);
				printf("push finished i=%d num=%d\n", i, num); fflush(stdout);
				//data[i].cub.free();
			}
			
			printf("endloop:num=%d\n", num); fflush(stdout);
			MKL_FreeBuffers();
			printf("cub_cur=%d\n", cuboid::__cub_cur); fflush(stdout);
			printf("mat_cur=%d\n\n", __mat_cnt); fflush(stdout);
			printf("@ memory: %.3lf GB\n", s_info.memory()); fflush(stdout);
			

			if (cur == 0) {
				double used = s_info.peak_memory() - old_memory;
				printf("cur=0: used=%.3lf max=%.3lf N=%d\n", used, max_memory, THREAD_N); fflush(stdout);
				int n = THREAD_N;
				while (n < MAX_THREAD_N && used / THREAD_N * (n + 1.5) < max_memory)
					++n;
				THREAD_N = n;
			}
		}
		for (int i = 0; i < MAX_THREAD_N; ++i)
			if (allocated[i])
				data[i].cub.free();
		/*
		for (int x = 0; x + psize - 1 < img.rows(); x += delta)
			for (int y = 0; y + psize - 1 < img.cols(); y += delta) {

				cuboid patch = img.extract_patch(x, y, psize);
				cuboid cub;
				if (!extract_feature(patch, config, centroids, cub)) {
					printf("fatal error: cannot extract!\n"); fflush(stdout);
					break;
				}
				result.push(cub, labels[i]);
				patch.free();
				cub.free();
			}
		result.finish();
		img.free();
		*/
		result.finish();
		img.release();


		printf("# current extract time: %s\n", ctm.total()); fflush(stdout);
		printf("# cpu rate = %.1lf%%\n", s_info.cpu_rate()); fflush(stdout);
		printf("# memory: %.3lf GB\n", s_info.memory()); fflush(stdout);
	}
}


void _main(int argc, char *argv[])
{
	if (argc != 3) {
		printf("usage: %s <ini_file> <ini_field>\n", argv[0]); fflush(stdout);
		printf("sample: %s d:\a.ini Extract1\n", argv[0]); fflush(stdout);
		return;
	}

	// Get Basic Informations
	int size, rank, flag;
	MPI_Status status;
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	printf("## CUR_THREAD_ID = %d / %d\n", rank, size); fflush(stdout);
	system_info global_sinfo;

	// Read Configuration File
	if (rank != 0)
		MPI_Recv(&flag, 1, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, &status);
	mk_config config;
	config.ext_field = (string(argv[2]));
	bool healthy = true;
	if(!config.load_from_ini(string(argv[1]), size, rank)) {
		printf("cannot load config from '%s'!\n", argv[1]); fflush(stdout);
		healthy = false;
	}
	if (rank != size - 1)
		MPI_Send(&flag, 1, MPI_INT, rank + 1, 0, MPI_COMM_WORLD);
	
	if (!healthy)
		assert(false);
#ifdef _USE_LOG_FILE
	printf("start logging. see log file %s for details.\n", config.log_path.c_str()); fflush(stdout);
	freopen(config.log_path.c_str(), "w", stdout);
#endif

	_timer timer;
	time_t rawtime;
	struct tm *timeinfo;
	time(&rawtime);
	timeinfo = gmtime(&rawtime);
	printf("## MK-Extract\n"); fflush(stdout);
	printf("## CUR_THREAD_ID = %d / %d\n", rank, size); fflush(stdout);
	printf("## Current UTC date/time is: %s", asctime(timeinfo)); fflush(stdout);
	printf("\n"); fflush(stdout);

	printf("config.path  = %s\n", config.path.c_str()); fflush(stdout);
	printf("  kmeans.list         = %s\n", config.kmeans_imglst.c_str()); fflush(stdout);
	printf("  kmeans.cen_file     = %s\n", config.cen_file.c_str()); fflush(stdout);
	printf("  kmeans.patch_num    = %d\n", config.patch_num); fflush(stdout);
	printf("  kmeans.rf_size      = %d\n", config.rf_size); fflush(stdout);
	printf("  kmeans.centroid_num = %d\n", config.centroid_num); fflush(stdout);
	printf("  kmeans.whitening    = %d\n", config.whitening); fflush(stdout);
	printf("  extract.path   = %s\n", config.extract_imgpath.c_str()); fflush(stdout);
	printf("  extract.list   = %s\n", config.extract_imglst.c_str()); fflush(stdout);
	printf("  extract.fea_f  = %s\n", config.fea_file.c_str()); fflush(stdout);
	printf("\n"); fflush(stdout);

	// Load Centroids
	printf("loading centroids and whitening data...\n"); fflush(stdout);
	mk_centroids centroids(config.learn_proc);
	if (!centroids.load(config.cen_file)) {
		printf("fatal error: cannot load data!\n"); fflush(stdout);
		return;
	}

	// Get Whole Image List and Create Directories
	if (config.rank == 0) {
		printf("creating directories...\n"); fflush(stdout);
		vector<string> list;
		if (!load_image_list(config.extract_imglst, config.size, config.rank, list, true)) {
			printf("fatal error: cannot load image list!\n"); fflush(stdout);
			return;
		}
		set<string> hash;
		for (size_t i = 0; i < list.size(); ++i) {
			string dir = config.fea_folder;
			for (size_t j = 0; j < list[i].size(); ++j) {
				dir += list[i][j];
				if (list[i][j] == '\\' || list[i][j] == '/') {
					//printf("make %s : ", dir.c_str()); fflush(stdout);
					if (hash.find(dir) == hash.end()) {
						//printf("maked.\n"); fflush(stdout);
						CreateDirectory(dir.c_str(), NULL);
						hash.insert(dir);
					} else {
						//printf("no.\n"); fflush(stdout);
					}
				}
			}
		}
	}
	printf("waiting...\n"); fflush(stdout);
	MPI_Barrier(MPI_COMM_WORLD);
	printf("all finished init.\n\n"); fflush(stdout);

	// Get Image List and Label List
	vector<string> list;
	if (!load_image_list(config.extract_imglst, config.size, config.rank, list)) {
		printf("fatal error: cannot load image list!\n"); fflush(stdout);
		return;
	}
	//for (int i = 0; i < (int)list.size(); ++i)
	//	list[i] = config.extract_imgpath + list[i];

	// Reading Labels
	FILE *f = fopen(config.extract_lablst.c_str(), "r");
	vector<int> labels;
	if (f != NULL) {
		printf("reading labels...\n"); fflush(stdout); 
		for (int x; fscanf(f, "%d", &x) == 1; )
			labels.push_back(x);
		fclose(f);
		printf("read %d labels...\n", labels.size()); fflush(stdout);
		vector<int> tmp = labels;
		labels.clear();
		int st = config.rank * images_per_thread;
		int en = (config.rank + 1) * images_per_thread;
		for (int i = st; i < min((int)tmp.size(), en); ++i)
			labels.push_back(tmp[i]);
	} else {
		printf("cannot read labels...\n"); fflush(stdout);
	}
	while (labels.size() < list.size())
		labels.push_back(-1);

	// Extract Features
	if (config.ext_largeimg_d > 0) {
		extr_large_images(config, list, labels, centroids);
	} else {
		extr_small_images(config, list, labels, centroids);
	}
	
	// Finish
	double cpu_rate = global_sinfo.cpu_rate();
	double all_cpu_rate;
	// MPI_Allreduce(&cpu_rate, &all_cpu_rate, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
	all_cpu_rate /= (double)config.size;

	time(&rawtime);
	timeinfo = localtime(&rawtime);
	printf("## Finished.\n"); fflush(stdout);
	printf("## The current date/time is: %s", asctime(timeinfo)); fflush(stdout);
	printf("## Cost time: %s.\n", timer.total()); fflush(stdout);
	printf("## Avg cpu rate: %.3lf%%\n", cpu_rate); fflush(stdout);
	printf("## All avg cpu: %.3lf%%\n", all_cpu_rate); fflush(stdout);
}


int main(int argc, char *argv[])
{
	__mat_cnt = 0;
	MPI_Init(&argc, &argv);	
	_main(argc, argv);
	MPI_Finalize();	
	return 0;
}