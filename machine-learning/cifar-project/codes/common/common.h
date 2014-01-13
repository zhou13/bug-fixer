#pragma once
#include <cstdlib>
#include <string>
#include <vector>
#include <ctime>
#include <cassert>
#include <iostream>
#include <algorithm>
#include <Windows.h>
#include <psapi.h>

#pragma warning(disable: 4996)
#pragma comment(lib, "psapi.lib")

typedef long long LL;

std::string str_extr_ext(std::string fname);

bool load_image_list(std::string file_name, int size, int rank, std::vector<std::string> &list, bool _keep_whole = false);
std::string i_to_s(int x);

double randn();
double sqr(double x);

extern int images_per_thread;

extern int __mat_cnt;


struct mk_config
{
	// MPI
	int size, rank;

	// Common
	std::string path;
	std::string log_path;
	int log_count;
	int io_limit;
	std::string learn_proc; // KMeans/Rica
	//int extr_bin;

	// KMeans
	std::string kmeans_imgpath;
	std::string kmeans_imglst;
	std::string kmeans_lablst;
	std::string cen_file;
	int cen_round_src;
	int cen_round_tar;
	//int max_scale;
	int patch_num;
	int rf_size;
	int centroid_num;
	int mil_neg_cen;
	int whitening;
	int kmeans_iterations;
	int kmeans_batch;

	// Rica
	double rica_lambda;
	double rica_epsilon;
	int rica_max_iterations;
	double rica_lbfgs_ftol;
	double rica_lbfgs_epsilon;

	// Extract
	int ext_cub_pool;
	int ext_fea_pool;
	int ext_largeimg_d;
	int ext_largeimg_w;
	std::string ext_field;
	std::string extract_imgpath;
	std::string extract_imglst;
	std::string extract_lablst;
	std::string fea_list;
	std::string fea_file;
	std::string fea_folder;

	mk_config() : ext_field("") {}
	//mk_config(std::string ext_field) : ext_field(ext_field) {}

	bool load_from_ini(std::string ini_file, int size, int rank);
	//int feature_dim() const { return centroid_num * ext_fea_pool * ext_fea_pool; }
};

class system_info
{
private:
	double m_fOldCPUIdleTime;  
	double m_fOldCPUKernelTime;  
	double m_fOldCPUUserTime;  
	double __ft2d(FILETIME &filetime)  
	{  
		return (double)(filetime.dwHighDateTime * 4.294967296E9) + (double)filetime.dwLowDateTime;  
	}  
public:
	system_info()
	{
		cpu_rate_init();
	}
	double memory() const
	{
		MEMORYSTATUSEX statex;
		statex.dwLength = sizeof(statex);
		GlobalMemoryStatusEx (&statex);
		return double(statex.ullAvailPhys / 1024. / 1024. / 1024.);
	}
	double peak_memory() const
	{
		PROCESS_MEMORY_COUNTERS pmc;
		GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
		return double(pmc.PeakWorkingSetSize / 1024. / 1024. / 1024.);
	}
	BOOL cpu_rate_init()
	{       
		FILETIME ftIdle, ftKernel, ftUser;  
		BOOL flag = GetSystemTimes(&ftIdle, &ftKernel, &ftUser);  
		if (flag) {  
			m_fOldCPUIdleTime = __ft2d(ftIdle);  
			m_fOldCPUKernelTime = __ft2d(ftKernel);  
			m_fOldCPUUserTime = __ft2d(ftUser);  
		}  
		return flag;  
	}   
	double cpu_rate()
	{  
		double nCPUUseRate = 0.;  
		FILETIME ftIdle, ftKernel, ftUser;  
		if (GetSystemTimes(&ftIdle, &ftKernel, &ftUser))  
		{  
			double fCPUIdleTime = __ft2d(ftIdle);  
			double fCPUKernelTime = __ft2d(ftKernel);  
			double fCPUUserTime = __ft2d(ftUser);  
			nCPUUseRate = (1. - (fCPUIdleTime - m_fOldCPUIdleTime)  
					/ (fCPUKernelTime - m_fOldCPUKernelTime + fCPUUserTime - m_fOldCPUUserTime));  
			m_fOldCPUIdleTime = fCPUIdleTime;  
			m_fOldCPUKernelTime = fCPUKernelTime;  
			m_fOldCPUUserTime = fCPUUserTime;  
		}  
		return nCPUUseRate * 100.;  
	} 
};

class _timer
{
	int tot;
	time_t st;
	char buf[999];
	char* s_string(double _s) {
		if (_s < 60) {
			sprintf(buf, "%.1lfs", _s);
			return (buf);
		}
		int s = (int) _s;
		int m = s / 60; s %= 60;
		if (m < 60) {
			sprintf(buf, "%dm %ds", m, s);
			return (buf);
		}
		int h = m / 60; m %= 60;
		if (h < 24) {
			sprintf(buf, "%dh %dm %ds", h, m, s);
			return (buf);
		}
		int d = h / 24; h %= 24;
		sprintf(buf, "%dd %dh %dm %ds", d, h, m, s);
		return (buf);
	}
public:
	_timer(int t = -1)
	{
		tot = t;
		st = time(NULL);
	}
	char* predict(int cur)
	{
		time_t t = time(NULL);
		double s = (double) (t - st);
		if (tot < 0 || cur == 0) {
			sprintf(buf, "UNKNOWN");
			return buf;
		}
		s = s / (double)cur * (double)(tot-cur);
		return s_string(s);
	}
	char* total()
	{
		time_t t = time(NULL);
		return s_string((double) (t - st));
	}
};