/* the same two tests as the `bench` command of the board shell (nfs/shell.pas), for comparing with other machines */
#include <stdio.h>
#include <time.h>
static long now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec * 1000L + t.tv_nsec / 1000000L; }
int main(void)
{
    static volatile int s[1000];
    int c = 0, acc = 0;
    long t0 = now();
    for (int r = 1; r <= 20; r++) {
        for (int i = 0; i < 1000; i++) s[i] = 1;
        c = 0;
        for (int i = 2; i < 1000; i++)
            if (s[i] == 1) { c++; for (int j = i + i; j <= 999; j += i) s[j] = 0; }
    }
    long t1 = now();
    for (int i = 1; i <= 200000; i++) acc = (acc + i * 7 + i / 3) % 65521;
    long t2 = now();
    printf("sieve 20 x 1000: %d primes, %ld ms\n", c, t1 - t0);
    printf("arithmetic loop 200000: %d, %ld ms\n", acc, t2 - t1);
    return 0;
}
