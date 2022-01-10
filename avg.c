/* A simple program to demo delta_avg and sum_avg
 * Advantage of delta_avg: No need to worry about the overflow of sum
 *      As a comtrast, run 'avg 50000' you'll see sum_avg overflow
 * ### How to make:
 *      gcc avg.c -o avg 
 * ### How to run:
 *      avg         # default to run 1000 times
 *      avg 50000   # run 50000 times

 
 */
 
#include <stdio.h>
#include <stdlib.h>
#include<time.h>

#define DEFAULT_RUN 1000
int main(int argc, char **argv)
{
    int vol, run = 0;
    float delta_avg=0;
    
    int sum=0;
    float sum_avg=0;
    
    if (argc > 1)
    {
        run = atoi(argv[1]);
    }
    
    if ( run <= 0)
    {
        run = DEFAULT_RUN;
    }
    
    // Use current time as seed for random generator
    srand(time(0));
    
    for(int i = 1; i<=run; i++)
    {
        float a2;
        vol = rand() % 100000;
        delta_avg = delta_avg * ((float)(i-1)/i) + (float)vol/i;
        
        sum += vol;
        sum_avg = (float)sum /i;
        printf("i=%d, vol=%d, delta_avg=%.2f, sum_avg=%.2f\n", i, vol, delta_avg, sum_avg);
    }
     
    
    return 0;
}