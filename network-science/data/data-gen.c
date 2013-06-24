#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <time.h>
#include <limits.h>

#include "array.h"

static int state = 1;
char buffer[1000];

typedef struct {
    int v;
    int idx;
} edge_t;

array(edge_t) *edge;
array(int) e_q;
array(int) v_q;

int timer;
int time_delta;

int num_nodes = 1;

int *src, *dst;
bool *v_inq, *e_inq, *visited;

int my_rand(void)
{
    const int M = 2147483647;
    const int A = 48271;
    const int Q = M/A;
    const int R = M%A;

    state = A*(state%Q) - R*(state/Q);

    if (state < 0)
        state += M;
    return state;
}

void my_srand(time_t seed)
{
    state = (int)seed;
}

int main(int argc, char *argv[])
{
    if (argc == 1) {
        puts("data-gen [number of vertices] [number of edges] [question density] ");
        puts("data-gen -e filename [question density]");
        return 0;
    }

    assert(argc == 4);

    if (strcmp(argv[1], "-e") == 0) {
        freopen(argv[2], "r", stdin);
        int n, m;

        double p = atof(argv[3]);
        gets(buffer);
        gets(buffer);
        scanf("# Nodes: %d Edges: %d ", &n, &m);
        printf("%d %d\n", n, m);
        gets(buffer);

        edge    = calloc(n, sizeof(edge[0]));
        v_inq   = calloc(n, sizeof(bool));
        e_inq   = calloc(m, sizeof(bool));
        visited = calloc(n, sizeof(bool));
        src     = malloc(m * sizeof(int));
        dst     = malloc(m * sizeof(int));

        for (int i = 0; i < m; ++i) {
            scanf("%d %d", src+i, dst+i);
            arr_push(edge[src[i]], ((edge_t){dst[i], i}));
            arr_push(edge[dst[i]], ((edge_t){src[i], i}));
        }

        arr_init(e_q);
        arr_init(v_q);

        v_inq[src[0]] = true;

        arr_for(p, edge[src[0]])
            arr_push(e_q, p->idx);

        timer = 0;
        while (!arr_empty(e_q)) {
            int idx = my_rand() % e_q.size;
            swap(e_q.v[idx], e_q.v[e_q.size-1]);
            idx = arr_pop(e_q);

            if (!visited[src[idx]]) {
                visited[src[idx]] = true;
                arr_push(v_q, src[idx]);
            }
            if (!visited[dst[idx]]) {
                visited[dst[idx]] = true;
                arr_push(v_q, dst[idx]);
            }

            if (!v_inq[src[idx]]) {
                v_inq[src[idx]] = true;
                arr_for(p, edge[src[idx]])
                    arr_push(e_q, p->idx);
            }
            if (!v_inq[dst[idx]]) {
                v_inq[dst[idx]] = true;
                arr_for(p, edge[dst[idx]])
                    arr_push(e_q, p->idx);
            }

            if (!e_inq[idx]) {
                e_inq[idx] = true;

                ++num_nodes;
                double flt = INT_MAX / 1000000. / num_nodes;
                if (flt > 1)
                    time_delta = max(1, 
                timer += time_delta;
                printf("d %d\n", time_delta);

                printf("a %d %d %.2f\n", 1+src[idx], 1+dst[idx], 1.);

                while ((double)my_rand()/INT_MAX < p) {
                    int r_time = my_rand() % (timer+1);
                    int node1 = v_q.v[my_rand()%v_q.size]+1;
                    int node2 = v_q.v[my_rand()%v_q.size]+1;
                    if (my_rand() % 3 == 0)
                        printf("q -c %d %d %d\n", r_time, node1, node2);
                    else if (my_rand() % 3 == 1)
                        printf("q -d %d %d\n", r_time, node1);
                    else
                        printf("q -a %d %d\n", r_time, node2);
                }
            }
        }
    } else {
        int n = atoi(argv[1]);
        int m = atoi(argv[2]);
        double p = atof(argv[3]);

        my_srand(time(NULL));

        printf("%d\n", n);
        for (int i = 0; i < m; ++i) {
            ++num_nodes;
            time_delta = max(1, INT_MAX / 100 / num_nodes);
            timer += time_delta;
            printf("d %d\n", time_delta);
            printf("a %d %d %.2f\n", 1+my_rand()%n, 1+my_rand()%n, 1+(double)my_rand()/INT_MAX);

            while ((double)my_rand()/INT_MAX < p) {
                int r_time = my_rand() % timer;

                if (my_rand() % 3 == 0)
                    printf("q -c %d %d %d\n", my_rand()%r_time, 1+my_rand()%n, 1+my_rand()%n);
                else if (my_rand() % 3 == 1)
                    printf("q -d %d %d\n", my_rand()%r_time, 1+my_rand()%n);
                else
                    printf("q -a %d %d\n", my_rand()%r_time, 1+my_rand()%n);
            }
        }
    }
}
