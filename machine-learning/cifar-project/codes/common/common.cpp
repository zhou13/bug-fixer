#include "common.h"
#include <Windows.h>
#include <cassert>
#include <cstdio>
using namespace std;

int images_per_thread;
int __mat_cnt;

string str_extr_ext(string fname)
{
	string s = "";
	for (int i = 0; i < (int)fname.size(); ++i) {
		if (fname[i] == '.')
			s = ".";
		else
			s = s + fname[i];
	}
	if (s.size() > 0 && s[0] != '.')
		s = "";
	return s;
}

double sqr(double x)
{
	return x * x;
}

double randn()
{
	double x1, x2, w, y1;
	do {
		x1 = 2.0 * (double)rand()/RAND_MAX - 1.0;
		x2 = 2.0 * (double)rand()/RAND_MAX - 1.0;
		w = x1 * x1 + x2 * x2;
	} while (w >= 1.0);
	w = sqrt((-2.0 * log(w)) / w);
	y1 = x1 * w;
	return y1;
}

string i_to_s(int x)
{
	char buf[99];
	sprintf(buf, "%d", x);
	return string(buf);
}

string read_ini_str(string ini_file, string section, string key, string def_val)
{
	char buf[MAX_PATH];
	GetPrivateProfileString(section.c_str(), key.c_str(), def_val.c_str(),
		buf, MAX_PATH, ini_file.c_str());
	//printf("ini=%s sec=%s key=%s buf=%s\n",ini_file.c_str(),section.c_str(),key.c_str(),def_val.c_str());
	return string(buf);
}

int read_ini_int(string ini_file, string section, string key, int def_val)
{
	int v = GetPrivateProfileInt(section.c_str(), key.c_str(), def_val, ini_file.c_str());
	return v;
}

double read_ini_double(string ini_file, string section, string key, string def_val)
{
	char buf[MAX_PATH];
	GetPrivateProfileString(section.c_str(), key.c_str(), def_val.c_str(),
		buf, MAX_PATH, ini_file.c_str());
	return atof(buf);
}

void write_ini_int(string ini_file, string section, string key, int val)
{
	char buf[99];
	sprintf(buf, " %d", val);
	WritePrivateProfileString(section.c_str(), key.c_str(), buf, ini_file.c_str());
}

bool mk_config::load_from_ini(string ini_file, int size, int rank)
{
	this->size = size;
	this->rank = rank;

	// Read Common Info
	string unknown = "[UNKNOWN]";
	//printf("loadini #1\n");fflush(stdout);
	path = read_ini_str(ini_file, "Common", "Path", unknown);
	if (path == unknown)
		return false;
	if (path.size() > 0 && path.back() != '\\' && path.back() != '/')
		path = path + "\\";
	log_path = read_ini_str(ini_file, "Common", "LogPath", unknown);
	if (log_path == unknown)
		return false;
	if (log_path.size() > 0 && log_path.back() != '\\' && log_path.back() != '/')
		log_path = log_path + "\\";
	log_count = read_ini_int(log_path + "log.ini", "Common", "LogCount", 0) + 1;
	io_limit = read_ini_int(ini_file, "Common", "IOLimit", 10);
	//extr_bin = read_ini_int(ini_file, "Common", "ExtrBin", 0);
	learn_proc = read_ini_str(ini_file, "Common", "LearnProc", "");
	if (learn_proc == "")
		return false;
	
	// Make Log
	if (rank == size - 1)
		write_ini_int(log_path + "log.ini", "Common", "LogCount", log_count);
	CreateDirectory(log_path.c_str(), NULL);
	CreateDirectory((log_path + i_to_s(log_count)).c_str(), NULL);
	log_path = log_path + i_to_s(log_count) + "\\" + i_to_s(rank) + ".txt";
	
	// Read KMeans Info
	cen_round_src = read_ini_int(ini_file, "KMeans", "CenRoundSrc", -1);
	cen_round_tar = read_ini_int(ini_file, "KMeans", "CenRoundTar", -1);

	if ((mil_neg_cen = read_ini_int(ini_file, "KMeans", "MILNegCen", -1)) < 0)
		return false;
	
	kmeans_imgpath = read_ini_str(ini_file, "KMeans", "ImgPath", unknown);
	if (kmeans_imgpath == unknown)
		return false;
	if (kmeans_imgpath.size() > 0 && kmeans_imgpath.back() != '\\' && kmeans_imgpath.back() != '/')
		kmeans_imgpath = kmeans_imgpath + "\\";

	kmeans_imglst = read_ini_str(ini_file, "KMeans", "ImgList", unknown);
	if (kmeans_imglst == unknown)
		return false;
	kmeans_imglst = path + kmeans_imglst;

	kmeans_lablst = read_ini_str(ini_file, "KMeans", "LabelList", unknown);
	if (kmeans_lablst == unknown)
		return false;
	kmeans_lablst = path + kmeans_lablst;

	cen_file = read_ini_str(ini_file, "KMeans", "CenFile", unknown);
	if (cen_file == unknown)
		return false;
	cen_file = path + cen_file;

//	if ((max_scale = read_ini_int(ini_file, "KMeans", "MaxScale", -1)) < 0)
//		return false;
	if ((patch_num = read_ini_int(ini_file, "KMeans", "Patches", -1)) < 0)
		return false;
	if ((rf_size = read_ini_int(ini_file, "KMeans", "RfSize", -1)) < 0)
		return false;
	if ((centroid_num = read_ini_int(ini_file, "KMeans", "Centroids", -1)) < 0)
		return false;
	if ((whitening = read_ini_int(ini_file, "KMeans", "Whitening", -1)) < 0)
		return false;
	if ((kmeans_iterations = read_ini_int(ini_file, "KMeans", "Iterations", -1)) < 0)
		return false;
	if ((kmeans_batch = read_ini_int(ini_file, "KMeans", "BatchSize", -1)) < 0)
		return false;	

	// Read Rica Info
	rica_lambda = read_ini_double(ini_file, "Rica", "Lambda", "-1");
	rica_epsilon = read_ini_double(ini_file, "Rica", "Epsilon", "-1");
	rica_max_iterations = read_ini_int(ini_file, "Rica", "MaxIter", -1);
	rica_lbfgs_ftol = read_ini_double(ini_file, "Rica", "LbfgsFtol", "-1");
	rica_lbfgs_epsilon = read_ini_double(ini_file, "Rica", "LbfgsEpsilon", "-1");

	// Read Extract Info
	if (ext_field != "") {
		if ((ext_cub_pool = read_ini_int(ini_file, "Extract", "CubPool", -2)) < -1)
			return false;
		if ((ext_fea_pool = read_ini_int(ini_file, "Extract", "FeaPool", -1)) < 0)
			return false;
		if ((ext_largeimg_d = read_ini_int(ini_file, ext_field, "LargeImageD", -2)) < -1)
			return false;
		if ((ext_largeimg_w = read_ini_int(ini_file, ext_field, "LargeImageW", -2)) < -1)
			return false;
		extract_imglst = read_ini_str(ini_file, ext_field, "ImgList", unknown);
		if (extract_imglst == unknown)
			return false;
		extract_imglst = path + extract_imglst;
		extract_lablst = read_ini_str(ini_file, ext_field, "LabelList", unknown);
		if (extract_lablst == unknown)
			return false;
		extract_lablst = path + extract_lablst;
		extract_imgpath = read_ini_str(ini_file, ext_field, "ImgPath", unknown);
		if (extract_imgpath == unknown)
			return false;
		if (extract_imgpath.size() > 0 && extract_imgpath.back() != '\\' && extract_imgpath.back() != '/')
			extract_imgpath = extract_imgpath + "\\";

		fea_list = read_ini_str(ini_file, ext_field, "FeaList", unknown);
		if (fea_list == unknown)
			return false;
		fea_list = path + fea_list;
		fea_file = read_ini_str(ini_file, ext_field, "FeaFile", unknown);
		if (fea_file == unknown)
			return false;
		fea_file = path + fea_file;
		fea_folder = read_ini_str(ini_file, ext_field, "FeaFolder", unknown);
		if (fea_folder == unknown)
			return false;
		fea_folder = path + fea_folder;
		if (fea_folder.size() > 0 && fea_folder.back() != '\\' && fea_folder.back() != '/')
			fea_folder = fea_folder + "\\";
		CreateDirectory(fea_folder.c_str(), NULL);
	}

	return true;
}

bool load_image_list(string file_name, int size, int rank, vector<string> &list, bool _keep_whole)
{
	FILE *f = fopen(file_name.c_str(), "r");
	if (f == NULL) {
		printf("cannot open image list!\n"); fflush(stdout);
		return false;
	}
	char buf[MAX_PATH];
	vector<string> tmp;
	while (!feof(f)) {
		if (fgets(buf, MAX_PATH, f) == NULL)
			break;
		int len = (int)strlen(buf);
		while (len > 0 && (buf[len-1] == ' ' || buf[len-1] == '\n' || buf[len-1] == '\r'))
			--len;
		buf[len] = '\0';
		if (len == 0)
			continue;
		//printf("cur file = [%s]\n", cur.c_str());
		tmp.push_back(string(buf));
	}
	fclose(f);

	int per = ((int)tmp.size() - 1) / size + 1;
	images_per_thread = per;
	for (int i = per * rank; i < min(per * (rank+1), (int)tmp.size()); ++i)
		list.push_back(tmp[i]);

	if (_keep_whole)
		list = tmp;

	printf("totally image number = %d\n", tmp.size()); fflush(stdout);
	printf("my image number = %d\n", list.size()); fflush(stdout);
	return true;
}