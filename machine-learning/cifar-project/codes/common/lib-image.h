#pragma once

#include "lib-image.h"
#include "opencv\cv.h"
#include "opencv\highgui.h"
#include <vector>
#include <cassert>
#include <direct.h>
using namespace cv;

#pragma comment(lib, "opencv_core242.lib")
#pragma comment(lib, "opencv_imgproc242.lib")
#pragma comment(lib, "opencv_highgui242.lib")
#pragma comment(lib, "opencv_objdetect242.lib")
#pragma comment(lib, "opencv_features2d242.lib")



class cuboid
{
	double ***a;
	int h, w, d;
public:
	static int __cub_cur;

	double __get(int i, int j, int k) const
	{
		return a[i][j][k];
	}
	cuboid()
	{
		h = w = d = 0;
		a = NULL;
	}
	void create(int rows, int cols, int len)
	{
++__cub_cur;
		assert(a == NULL);
		h = rows;
		w = cols;
		d = len;
		a = new double**[rows];
		//a = (double***)malloc(sizeof(double**) * rows);
		for (int i = 0; i < rows; ++i) {
			a[i] = new double*[cols];
			//a[i] = (double**)malloc(sizeof(double*) * cols);
			for (int j = 0; j < cols; ++j) {
				a[i][j] = new double[len];
				//a[i][j] = (double*)malloc(sizeof(double) * len);
				memset(a[i][j], 0, sizeof(double) * len);
			}
		}
	}
	/*
	void create_empty(int rows, int cols)
	{
		assert(a == NULL);
		h = rows;
		w = cols;
		d = 0;
		//a = new double**[rows];
		a = (double***)malloc(sizeof(double**) * rows);
		for (int i = 0; i < rows; ++i) {
			//a[i] = new double*[cols];
			a[i] = (double**)malloc(sizeof(double*) * cols);
			for (int j = 0; j < cols; ++j)
				a[i][j] = NULL;
		}
	}
	*/
	void pool_from(const cuboid &cub, int p)
	{
		assert(a == NULL);
		int ph = (cub.rows() - 1) / p + 1;
		int pw = (cub.cols() - 1) / p + 1;
		create(p, p, cub.len());
		for (int i = 0; i < cub.rows(); ++i)
			for (int j = 0; j < cub.cols(); ++j)
				for (int k = 0; k < cub.len(); ++k)
					a[i / ph][j / pw][k] += cub.a[i][j][k];
	}
	void to_vector(double p[]) const
	{
		for (int i = 0; i < h; ++i)
			for (int j = 0; j < w; ++j)
				for (int k = 0; k < d; ++k)
					p[i * w * d + j * d + k] = a[i][j][k];
	}
	void assign(const matrix &m)
	{
		assert(h * w == m.rows());
		assert(d == m.cols());
		for (int i = 0; i < h; ++i)
			for (int j = 0; j < w; ++j)
				memmove(a[i][j], m.get_row_buf(i * w + j), sizeof(double) * d);
	}
	void assign_patch(const Mat &mat, int r, int c, int rf_size)
	{
		assert(r >= 0 && r + rf_size - 1 < mat.rows);
		assert(c >= 0 && c + rf_size - 1 < mat.cols);
		create(rf_size, rf_size, 3);
		for (int i = 0; i < rf_size; ++i)
			for (int j = 0; j < rf_size; ++j)
				for (int k = 0; k < d; ++ k)
					a[i][j][d - 1 - k] = (double)mat.at<unsigned char>(i + r, (j + c) * 3 + k);
	}
	void free()
	{
--__cub_cur;
		assert(a != NULL);
		for (int i = h-1; i >= 0; --i) {
			for (int j = w-1; j>= 0; --j)
				delete a[i][j];
			delete a[i];
		}
		delete a;
		h = w = d = 0;
		a = NULL;
	}
	int rows() const { return h; }
	int cols() const { return w; }
	int len() const { return d; }

	bool save(string fname)
	{
		assert(a != NULL);
		int la = -1;
		for (int i = 0; i < (int)fname.size(); ++i)
			if (fname[i] == '\\' || fname[i] == '/')
				la = i;
		if (la > 0) {
			string dir = fname.substr(0, la + 1);
			//system(("mkdir " + dir).c_str());
			_mkdir(dir.c_str());
			printf("created dir %s\n", dir.c_str()); fflush(stdout);
		}
		FILE *f = fopen(fname.c_str(), "wb");
		if (f == NULL)
			return false;
		size_t cnt = 0;
		cnt += fwrite(&h, sizeof(int), 1, f);
		cnt += fwrite(&w, sizeof(int), 1, f);
		cnt += fwrite(&d, sizeof(int), 1, f);
		if (cnt < 3) {
			fclose(f);
			return false;
		}
		for (int i = 0; i < h; ++i)
			for (int j = 0; j < w; ++j)
				cnt += fwrite(a[i][j], sizeof(double) * d, 1, f);
		fclose(f);
		if ((int)cnt < h * w)
			return false;
		return true;
	}

	bool load(string fname)
	{
		assert(a == NULL);
		if (str_extr_ext(fname) == ".cub") {
			// cuboid format
			FILE *f = fopen(fname.c_str(), "rb");
			if (f == NULL)
				return false;
			size_t cnt = 0;
			cnt += fread(&h, sizeof(int), 1, f);
			cnt += fread(&w, sizeof(int), 1, f);
			cnt += fread(&d, sizeof(int), 1, f);
			if (cnt < 3) {
				fclose(f);
				return false;
			}
			create(h, w, d);
			cnt = 0;
			for (int i = 0; i < h; ++i)
				for (int j = 0; j < w; ++j)
					cnt += fread(a[i][j], sizeof(double) * d, 1, f);
			fclose(f);
			if ((int)cnt < h * w)
				return false;
			return true;
		} else {
			// image format
			Mat mat = imread(fname);
			if (!mat.data)
				return false;
			if (mat.rows > 2000) {
				printf(" - loaded mat. memory: %.3lf GB\n", system_info().memory()); fflush(stdout);
			}
			create(mat.rows, mat.cols, 3);
			if (mat.rows > 2000) {
				printf(" - created mat. memory: %.3lf GB\n", system_info().memory()); fflush(stdout);
			}
			for (int i = 0; i < h; ++i) {
				for (int j = 0; j < w; ++j)
					for (int k = 0; k < d; ++k)
						a[i][j][d - 1 - k] = (double)mat.at<unsigned char>(i, j * 3 + k);
			}
			mat.release();
			return true;
		}
	}
	void extract_patch(int r, int c, int rf_size, double p[]) const
	{
		assert(r >= 0 && r + rf_size - 1 < h);
		assert(c >= 0 && c + rf_size - 1 < w);
		for (int i = 0; i < rf_size; ++i)
			for (int j = 0; j < rf_size; ++j)
				for (int k = 0; k < d; ++ k)
					p[k * rf_size * rf_size + i * rf_size + j] = a[r + i][j + c][k];
	}
	cuboid extract_patch(int r, int c, int rf_size) const
	{
		assert(r >= 0 && r + rf_size - 1 < h);
		assert(c >= 0 && c + rf_size - 1 < w);
		cuboid ret;
		ret.create(rf_size, rf_size, d);
		for (int i = 0; i < rf_size; ++i)
			for (int j = 0; j < rf_size; ++j)
				for (int k = 0; k < d; ++ k)
					ret.a[i][j][k] = a[r + i][j + c][k];
		return ret;
	}
};

/*
class image_data
{
	unsigned char **a[3];
	int h, w;
public:
	image_data(int h, int w)
	{
		this->h = h;
		this->w = w;
	}
};

Mat *scale_image(const Mat &mat, int scale)
{
	int h0 = (mat.rows - 1) / scale + 1;
	int w0 = (mat.cols - 1) / scale + 1;
	Mat *m = new Mat(h0, w0, CV_8UC3);
	for (int i = 0; i < h0; ++i)
		for (int j = 0; j < w0; ++j)
			for (int c = 0; c < 3; ++c) {
				int s = 0, sc = 0;
				for (int x = i * scale; x < min((i + 1) * scale, mat.rows); ++x)
					for (int y = j * scale; y < min((j + 1) * scale, mat.cols); ++y) {
						s += (int)mat.at<unsigned char>(x, y*3 + c);
						++sc;
					}
				s /= sc;
				m->at<unsigned char>(i, j*3 + c) = (unsigned char)s;
			}
	return m;
}

std::vector<unsigned char> extract_patch(const Mat &mat, int i, int j, int rf_size)
{
	assert(i >= 0 && i+rf_size-1 < mat.rows);
	assert(j >= 0 && j+rf_size-1 < mat.cols);
	vector<unsigned char> ret(rf_size * rf_size * 3);
	for (int c = 0; c < 3; ++c)
		for (int x = 0; x < rf_size; ++x)
			for (int y = 0; y < rf_size; ++y) {
				int p = (2 - c) * rf_size * rf_size + x * rf_size + y;
				ret[p] = mat.at<unsigned char>(i+x, (j+y)*3 + c);
			}
	return ret;
}

int get_image_hash(const Mat &mat, int M) {
	long long a = 0;
	for (int c = 0; c < 3; ++c)
		for (int x = 0; x < mat.rows; ++x)
			for (int y = 0; y < mat.cols; ++y) {
				a += (int)mat.at<unsigned char>(x, y*3 + c);
				a %= M;
			}
	return (int)a;
}
*/

void normalize_patch(double *p, int patch_size)
{
	// subtract the mean
	double mean = 0.;
	for (int i = 0; i < patch_size; ++i)
		mean += p[i];
	mean /= (double)patch_size;
	for (int i = 0; i < patch_size; ++i)
		p[i] -= mean;
	// divide by the standard deviation	
	double sd = 0.;
	for (int i = 0; i < patch_size; ++i)
		sd += p[i] * p[i];
	sd /= (double)(patch_size - 1);
	sd = sqrt(sd + 10);
	for (int i = 0; i < patch_size; ++i)
		p[i] /= sd;
}

void normalize_patch_2(double *p, int patch_size)
{
	// subtract the mean
	double mean = 0.;
	for (int i = 0; i < patch_size; ++i)
		mean += p[i];
	mean /= (double)patch_size;
	for (int i = 0; i < patch_size; ++i)
		p[i] -= mean;
	// divide by the standard deviation	
	double sd = 0.;
	for (int i = 0; i < patch_size; ++i)
		sd += p[i] * p[i];
	sd /= (double)(patch_size - 1);
	sd = sqrt(sd + 0.01);
	for (int i = 0; i < patch_size; ++i)
		p[i] /= sd;
}

int cuboid::__cub_cur = 0;