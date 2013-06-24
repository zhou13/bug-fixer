/* The MIT License (MIT)
 *
 * Copyright (c) 2012, Yichao Zhou
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

/* This is a macro library to implement a dynamic-sizegth array.  Notice that
 * arr_pop will *NOT* shrink the actual memory it uses. */

#pragma once

#include "utility.h"

#define array(type) struct { \
    type *v; \
    int size; \
    int mem_size; \
}

#define arr_init(arr) ({ \
    (arr).v = NULL; \
    (arr).size = 0; \
    (arr).mem_size = 0; \
})

#define arr_push(arr, element) \
({ \
    if ((arr).size == (arr).mem_size) { \
        (arr).mem_size = (arr).mem_size * 2 + 2; \
        (arr).v = realloc((arr).v, sizeof((arr).v[0]) * (arr).mem_size); \
    } \
    (arr).v[(arr).size++] = (element); \
})

#define arr_erase(arr, index) \
({ \
})

#define arr_empty(arr) ((arr).size == 0)

#define arr_resize(arr, new_size) \
({ \
    if ((arr).mem_size < new_size) { \
        (arr).mem_size = min((arr).mem_size*2, new_size); \
        (arr).v = realloc((arr).v, sizeof((arr).v[0]) * (arr).mem_size); \
    } \
})

#define arr_pop(arr)   ({(arr).size--; (arr).v[(arr).size];})
#define arr_free(arr)  (free((arr).v))
#define arr_back(arr)  ((arr).v[(arr).size-1])
#define arr_front(arr) ((arr).v[0])

#define arr_for(i, arr) for (typeof((arr).v) i = (arr).v; i < (arr).v + (arr).size; ++i)
