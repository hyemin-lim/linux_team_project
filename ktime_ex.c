#include <linux/ktime.h>

#define BILLION 1000000000

unsigned long long total_time = 0;
unsigned long long total_count = 0;

unsigned long long calclock3(struct timespec *spclock, unsigned long long *total_time, unsigned long long *total_count)
{
    long temp, temp_n;
    unsigned long long timedelay = 0;
    if (spclock[1].tv_nsec >= spclock[0].tv_nsec)
    {
        temp = spclock[1].tv_sec - spclock[0].tv_sec;
        temp_n = spclock[1].tv_nsec - spclock[0].tv_nsec;
        timedelay = BILLION * temp + temp_n;
    }
    else 
    {
        temp = spclock[1].tv_sec - spclock[0].tv_sec - 1;
        temp_n = BILLION + spclock[1].tv_nsec - spclock[0].tv_nsec;
        timedelay = BILLION * temp + temp_n;
    }

    __sync_fetch_and_add(total_time, timedelay);
    __sync_fetch_and_add(total_count, 1);
    return timedelay;
}

void example() {
    struct timespec spclock[2];
    ktime_get_ts(&spclock[0]);
    // Something to measure
    ktime_get_ts(&spclock[1]);
    calclock3(spclock, &total_time, &total_count);
}