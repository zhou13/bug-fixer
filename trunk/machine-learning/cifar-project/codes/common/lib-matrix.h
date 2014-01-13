#pragma once

#include <cstring>
#include <cassert>
#include <iostream>
#include "common.h"

#pragma warning(disable: 4996)


#include "mkl.h"
#pragma message(" - Linking with mkl_intel_lp64.lib")
#pragma comment(lib,            "mkl_intel_lp64.lib")	

#pragma message(" - Linking with mkl_intel_thread.lib")
#pragma comment(lib,            "mkl_intel_thread.lib")	

#pragma message(" - Linking with mkl_solver_lp64.lib")
#pragma comment(lib,            "mkl_solver_lp64.lib")

#pragma message(" - Linking with mkl_core.lib")
#pragma comment(lib,            "mkl_core.lib")

#pragma message(" - Linking with libguide.lib")
#pragma comment(lib,            "libguide.lib")

class matrix
{
	double *a;
	int h, w;

public:
	matrix()
	{
		h = w = 0;
		a = NULL;
	}

	bool exists() const
	{
		return a != NULL;
	}

	void copy(const matrix &t)
	{
__mat_cnt++;
		assert(t.a != NULL);
		h = t.h;
		w = t.w;
		//a = new double[h * w];
		a = (double*)MKL_malloc(h * w * sizeof(double), 64);
		memmove(a, t.a, sizeof(double) * h * w);
	}

	double* get_buf() const
	{
		return a;
	}

	double* get_row_buf(int i) const
	{
		assert(i >= 0 && i < h);
		return &a[i * w];
	}

	void clear()
	{
		assert(a != NULL);
		memset(a, 0, sizeof(double) * h * w);
	}

	void mult(double c)
	{
		assert(a != NULL);
		for (int i = 0; i < h * w; ++i)
			a[i] *= c;
	}

	void create(int h, int w)
	{
__mat_cnt++;
		assert(a == NULL);
		this->h = h;
		this->w = w;
		//a = new double[h * w];
		a = (double*)MKL_malloc(h * w * sizeof(double), 64);
		memset(a, 0, sizeof(double) * h * w);
	}

	void free()
	{
__mat_cnt--;
		assert(a != NULL);
		//delete a;
		MKL_free(a);
		h = w = 0;
		a = NULL;
	}

	int rows() const
	{
		return h;
	}

	int cols() const
	{
		return w;
	}

	double& val(int i, int j)
	{
		assert(i >= 0 && i < h);
		assert(j >= 0 && j < w);
		return a[i * w + j];
	}

	double get_val(int i, int j) const
	{
		assert(i >= 0 && i < h);
		assert(j >= 0 && j < w);
		return a[i * w + j];
	}

	void multiply_right(const matrix &b)
	{
//printf(" [multiply_right (%d,%d,%d)] - memory: %.3lf GB\n", h, w, b.w, system_info().memory());
		assert(a != NULL);
		assert(w == b.h);
		//double *c = new double[h * b.w];
		double *c = (double*)MKL_malloc(h * b.w * sizeof(double), 64);
		memset(c, 0, sizeof(double) * h * b.w);
//printf(" [multiply_right ] - memory: %.3lf GB\n",  system_info().memory());
		cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
			h, b.w, w, 1., a, w, b.a, b.w, 0., c, b.w);
//printf(" [multiply_right ] - memory: %.3lf GB\n",  system_info().memory());
		//delete a;
		MKL_free(a);
		w = b.w;
		a = c;
//printf(" [multiply_right (%d,%d)] - memory: %.3lf GB\n", h, w, system_info().memory()); fflush(stdout);
	}

	void multiply_left(const matrix &b)
	{
		assert(a != NULL);
		assert(b.w == h);
		//double *c = new double[b.h * w];
		double *c = (double*)MKL_malloc(b.h * w * sizeof(double), 64);
		cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
			b.h, w, b.w, 1., b.a, b.w, a, w, 0., c, w);
		//delete a;
		MKL_free(a);
		h = b.h;
		a = c;
	}

	matrix multiply(const matrix &b) const
	{
		assert(a != NULL);
		assert(w == b.h);
		matrix ret;
		ret.create(h, b.w);
		cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
			h, b.w, w, 1., a, w, b.a, b.w, 0., ret.a, b.w);
		return ret;
	}

	matrix transpose() const
	{
		assert(a != NULL);
		matrix b;
		b.create(w, h);
		for (int i = 0; i < h; ++i)
			for (int j = 0; j < w; ++j)
				b.val(j, i) = a[i * w + j];
		return b;
	}

	void write(FILE *f) const
	{
		assert(a != NULL);
		fprintf(f, "%d %d\n", h, w);
		for (int i = 0; i < h; ++i) {
			for (int j = 0; j < w; ++j)
				fprintf(f, "%.9g ", a[i * w + j]);
			fprintf(f, "\n");
		}
	}

	void read(FILE *f)
	{
		assert(a == NULL);
		fscanf(f, "%d %d", &h, &w);
		create(h, w);
		for (int i = 0; i < h; ++i)
			for (int j = 0; j < w; ++j)
				fscanf(f, "%lf", &a[i * w + j]);
	}
};