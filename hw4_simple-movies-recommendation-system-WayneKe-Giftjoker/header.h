#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<assert.h>
#include<math.h>
#include<fcntl.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<errno.h>
#include<pthread.h>

// error handling
#define ERR_EXIT(a) do {perror(a);exit(1);} while(0)

// max number of movies
#define MAX_MOVIES 70000

// max length of each movies
#define MAX_LEN 0xff

// number of requests
#define MAX_REQ 0xff

// number of genre
#define NUM_OF_GENRE 19

// number of l1,l2 threads
#define MAX_CPU 0x200
#define MAX_THREAD 0x100

#define MAX_LAYER 5
#define MAX_THREAD_NUM 16
#define SMALLCASE_SORT_NUM 1024

#define MAX_PROCESS 0x200
#define PROCESS_MAX_LAYER 7

void sort(char** movies, double* pts, int size);

typedef struct request{
	int id;
	char* keywords;
	double* profile;
}request;

typedef struct movie_profile{
	int movieId;
	char* title;
	double* profile;
}movie_profile;

typedef struct mergesortArgs{
	int layerid;
	int st;
	int ed; //included
	char **filtered_mov_title;
	double *filtered_mov_score;
	// int filtered_mov_num;
	char **sorted_mov_title;
	double *sorted_mov_score;
	int isThread;
}mergesortArgs;

typedef struct threadArgs{
	int tid;
	int reqid;
}threadArgs;