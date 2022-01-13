/* A simple program to calculate dBFS.
 *      REF: https://stackoverflow.com/a/4152428
 * 
 * NOTE: it can process 16-bit data only.
 *
 * ### How to make:
 *      gcc dbfs.c -lm -o dbfs 
 * ### How to run:
 *      dbfs -h     # show help message
 *      avg 50000   # run 50000 times
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <getopt.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define DEFAULT_RUN 1000
#define DEFAULT_OUTPUT_INTERVAL 100
#define DEFAULT_LOWER_BOUND 0
#define DEFAULT_UPPER_BOUND 32767

#define STR(x) #x

#define PATTERN_RANDOM      0
#define PATTERN_CONSTANT    1
#define PATTERN_FILE        2

#define PATTERM_MAX PATTERN_FILE

// Global variable to store file descriptor, for generate_value_pattern() and main()
int g_fd = -1;


// NOTE1: for PATTERN_FILE, assume file is open successfully, we don't check here, and don't close it.
// NOTE2: Return type is short to guarantee 16-bit data
short generate_value_pattern(int pattern, int lower_bound, int upper_bound)
{
    short value = 0;
    int len;
    
    switch (pattern)
    {
        case PATTERN_CONSTANT:
            value = (lower_bound + upper_bound) /2;
            break;
        case PATTERN_RANDOM:
            value = rand() % (upper_bound - lower_bound) + lower_bound;
            break;
        case PATTERN_FILE:
            // Assume 16-bit raw sound file
            if ( (len = (int) read(g_fd, &value, 2)) != 2)
            {
                lseek(g_fd, SEEK_SET, 0);
                printf("Read file: Rewind\n");
                if ( (len = (int) read(g_fd, &value, 2)) != 2)
                {
                    printf("READ ERROR, try to seek to the beginning.\n");
                }
            }
            break;
    }
    
    return value;
}


struct option longopts[] = {
	{"help", no_argument, NULL, 'h'},
	{"run", required_argument, NULL, 'r'},
	{"interval", required_argument, NULL, 'i'},
	{"lower", required_argument, NULL, 'l'},
	{"upper", required_argument, NULL, 'u'},
	{"pattern", required_argument, NULL, 'p'},
	{"file", required_argument, NULL, 'f'},
	{"verbose", no_argument, NULL, 'v'},
	{0, 0, 0, 0}
};

void print_help_info(char *prog_name)
{
	fprintf(stdout, "DESCRIPTION:\n");
	fprintf(stdout, "\tSound dB sample code, works only on 16-bit (-32768 ~ 32767) format.\n");
	fprintf(stdout, "USAGE:\n");
	fprintf(stdout, "\t%s\n", prog_name);
	fprintf(stdout, "\t\t-h/--help    show this help message\n");
	fprintf(stdout, "\t\t-r/--run <times>    total run times, default = %d\n", DEFAULT_RUN);
	fprintf(stdout, "\t\t-i/--interval <interval>  output interval, default = %d\n", DEFAULT_OUTPUT_INTERVAL);
	fprintf(stdout, "\t\t-l/--lower <bound>  lower bound of generated value, default = %d\n", DEFAULT_LOWER_BOUND);
	fprintf(stdout, "\t\t-u/--upper <bound>  upper bound of generated value, default = %d\n", DEFAULT_UPPER_BOUND);
	fprintf(stdout, "\t\t-f/--file <filename>  open file to get value\n");
	fprintf(stdout, "\t\t-p/--pattern <pattern(0-%d)>  value generated pattern: (defaulat = %d)\n", PATTERM_MAX, PATTERN_RANDOM);
	fprintf(stdout, "\t\t\t%d: %s\n", PATTERN_RANDOM, STR(PATTERN_RANDOM));
	fprintf(stdout, "\t\t\t%d: %s\n", PATTERN_CONSTANT, STR(PATTERN_CONSTANT));
	fprintf(stdout, "\t\t\t%d: %s\n", PATTERN_FILE, STR(PATTERN_FILE));
	fprintf(stdout, "\t\t-v/--verbose  verbose output to show data of every run\n");

	fprintf(stdout, "EXAMPLE:\n");
	fprintf(stdout, "\t%s -r 1000 -l 0 -u 32767 \n", prog_name);
}


int main(int argc, char **argv)
{
	int c;
    int vol;
    int run = DEFAULT_RUN;
    int lower_bound = DEFAULT_LOWER_BOUND;
    int upper_bound = DEFAULT_UPPER_BOUND;
    int pattern = PATTERN_RANDOM;
    char *filename = NULL;
    int interval = DEFAULT_OUTPUT_INTERVAL;
    int count = 1;
    int i = 1;
    int verbose = 0;
    
    
    // delta_avg: average, delta_avg2: square average
    double vol2, delta_avg = 0, delta_avg2=0;
    double rms = 0, decibel = 0;
    
	while ((c = getopt_long(argc, argv,
				":hr:i:l:u:p:f:v", longopts, NULL)) != -1) {
		switch (c) {
		case 'h':
			print_help_info(argv[0]);
			return 0;
		case 'r':
			run = atoi(optarg);
			break;
		case 'i':
			interval = atoi(optarg);
			break;
		case 'l':
			lower_bound = atoi(optarg);
			break;
		case 'u':
			upper_bound = atoi(optarg);
			break;
		case 'p':
			pattern = atoi(optarg);
			break;
		case 'f':
			filename = optarg;
			break;
		case 'v':
			verbose = 1;
			break;
            
		case ':':
			printf("required argument : -%c\n", optopt);
			return 0;
		case '?':
			printf("invalid param: -%c\n", optopt);
			return 0;
            
		}
	}
    
    // Arguments post check:
    if ( (pattern == PATTERN_FILE) && (filename == NULL))
    {
        printf("ERROR: Assign pattern as PATTERN_FILE, but no filename assigned!\n");
        printf("    Use '-f <filename>\' to assigne file to open\n");
        return -1;
    }

    // Open file if PATTERN_FILE is assigned
    if ( pattern == PATTERN_FILE)
    {
        g_fd = open(filename, O_RDONLY);
        if ( g_fd < 0 )
        {
            char msg[256];
            snprintf(msg, sizeof(msg), "Error: Failed to open file \'%s\'", filename);
            perror(msg);
            return -1;
        }
    }
    
    
    // Use current time as seed for random generator if requested
    srand(time(0));
    
    for(int r = 1; r<=run; r++)
    {
        double a2;
        
        vol = generate_value_pattern(pattern, lower_bound, upper_bound);
        
        delta_avg = delta_avg * ((double)(count-1)/count) + (double)vol/count;
        
        vol2 = ((double)vol/32768.0) * ((double)vol/32768.0);
        delta_avg2 = delta_avg2 * ((double)(count-1)/count) + (double)vol2/count;

        if ( (r % interval) == 0)
        {
            rms = sqrt(delta_avg2);
            decibel = 20 * log10(rms);

            printf("i=%d, r=%d, vol=%d, vol2=%.2f, delta_avg=%.2f, delta_avg2=%.4f, rms=%.2f, dBFS =%.2f dB\n", 
                    i, r, vol, vol2, delta_avg, delta_avg2, rms, decibel);
                    
            // Reset stat:
            delta_avg = delta_avg2 = 0;
            count = 1;
            i++;
        }
        else
        {
            count++;
            
            if ( verbose )
            {
                char dummy[32];
                int space = snprintf(dummy, sizeof(dummy), "i=%d,", i);
                printf("%*s r=%d, vol=%d, vol2=%.2f, delta_avg=%.2f, delta_avg2=%.4f, rms=%.2f, dBFS =%.2f dB\n", 
                        space, "", r, vol, vol2, delta_avg, delta_avg2, rms, decibel);
            }
        }
            
    }
     
    // close file if opened
    if ( g_fd)
    {
        close(g_fd);
    }
    return 0;
}