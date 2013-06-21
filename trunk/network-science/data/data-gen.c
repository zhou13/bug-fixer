#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <time.h>
#include <limits.h>

static int state = 1;

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
        // puts("data-gen -e filename [number of question]");
        return 0;
    }

    assert(argc == 4);

    /*
    if (strcmp(argv[1], "-e") == 0) {
        freopen(argv, stdin);
        scanf("%d %d ", &n, &m);
        for (int i = 0; i < m; ++i) {
            int src, dst;
            scanf("%d %d", &src, &dst);
        }
    } else {
    */
    int n = atoi(argv[1]);
    int m = atoi(argv[2]);
    double p = atof(argv[3]);

    my_srand(time(NULL));

    printf("%d\n", n);
    for (int i = 0; i < m; ++i) {
        printf("a %d %d\n", 1+my_rand()%n, 1+my_rand()%n);
        while ((double)my_rand()/INT_MAX < p) {
            if (my_rand() & 1)
                printf("q -c %d %d %d\n", my_rand()%(i+2), 1+my_rand()%n, 1+my_rand()%n);
            else
                printf("q -d %d %d\n", my_rand()%(i+2), 1+my_rand()%n);
        }
    }
}
