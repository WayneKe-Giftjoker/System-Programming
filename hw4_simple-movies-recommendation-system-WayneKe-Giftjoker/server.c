//Discussed with b08208032 胡材溢 同學
#include "header.h"
#include <stdbool.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <fcntl.h>

movie_profile* movies[MAX_MOVIES];
unsigned int num_of_movies = 0;
unsigned int num_of_reqs = 0;

// Global request queue and pointer to front of queue
// TODO: critical section to protect the global resources
request* reqs[MAX_REQ];
int front = -1;

/* Note that the maximum number of processes per workstation user is 512. * 
 * We recommend that using about <256 threads is enough in this homework. */

#ifdef THREAD
//mutex
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER; 
pthread_cond_t ready = PTHREAD_COND_INITIALIZER;
bool using;
// int max_thread_num = MAX_THREAD/(1<<(MAX_LAYER+1)-1) + 1;
int max_thread_num = MAX_THREAD_NUM;
mergesortArgs *thread_args;
int thread_cnt = 0;
#endif

void initialize(FILE* fp);
request* read_request();
int pop();

int pop(){
	front+=1;
	return front;
}
// ============thread=============
#ifdef THREAD
void* thread_mergesort(void *tmpargs){
	mergesortArgs *args = (mergesortArgs *)tmpargs;
	if(args->layerid == MAX_LAYER || (args->ed - args->st + 1) <= SMALLCASE_SORT_NUM){
		sort((args->filtered_mov_title) + (args->st), (args->filtered_mov_score) + (args->st), args->ed - args->st + 1);
		if(args->isThread)
			pthread_exit(0);
		else
			return (void *)0;
	}
	else{
		pthread_t ltid, rtid;
		mergesortArgs largs, rargs;
		largs.layerid = args->layerid+1; rargs.layerid = args->layerid+1;
		int mid = (args->st + args->ed) >> 1;
		largs.st = args->st; largs.ed = mid;
		rargs.st = mid+1; rargs.ed = args->ed;
		largs.filtered_mov_title = rargs.filtered_mov_title = args->filtered_mov_title;
		largs.filtered_mov_score = rargs.filtered_mov_score = args->filtered_mov_score;
		largs.sorted_mov_title   = rargs.sorted_mov_title   = args->sorted_mov_title;
		largs.sorted_mov_score   = rargs.sorted_mov_score   = args->sorted_mov_score;
		largs.isThread = 1; rargs.isThread = 0;
		pthread_create(&ltid, NULL, thread_mergesort, (void *)&largs);
		// fprintf(stderr, "create left worker %d\n", ltid);
		// pthread_create(&rtid, NULL, thread_mergesort, (void *)&rargs);
		// fprintf(stderr, "recursive right worker\n");
		thread_mergesort((void *)&rargs);
		int ldone;//, rdone;
		ldone = pthread_join(ltid, 0); // rdone = pthread_join(rtid, 0);
		if(ldone == 0){
			//merge
			int subsize = args->ed - args->st + 1;
			int lidx = largs.st, ridx = rargs.st, totidx = largs.st;
			while(lidx <= mid && ridx <= args->ed){
				if(args->filtered_mov_score[lidx] > args->filtered_mov_score[ridx]){
					args->sorted_mov_title[totidx] = args->filtered_mov_title[lidx];
					args->sorted_mov_score[totidx] = args->filtered_mov_score[lidx];
					++totidx; ++lidx;
				}
				else if(args->filtered_mov_score[ridx] > args->filtered_mov_score[lidx]){
					args->sorted_mov_title[totidx] = args->filtered_mov_title[ridx];
					args->sorted_mov_score[totidx] = args->filtered_mov_score[ridx];
					++totidx; ++ridx;
				}
				else{
					if(strcmp(args->filtered_mov_title[lidx], args->filtered_mov_title[ridx]) < 0){ //l < r
						args->sorted_mov_title[totidx] = args->filtered_mov_title[lidx];
						args->sorted_mov_score[totidx] = args->filtered_mov_score[lidx];
						++totidx; ++lidx;
					}
					else{
						args->sorted_mov_title[totidx] = args->filtered_mov_title[ridx];
						args->sorted_mov_score[totidx] = args->filtered_mov_score[ridx];
						++totidx; ++ridx;
					}
				}
			}
			while(lidx <= mid){
				args->sorted_mov_title[totidx] = args->filtered_mov_title[lidx];
				args->sorted_mov_score[totidx] = args->filtered_mov_score[lidx];
				++totidx; ++lidx;
			}
			while(ridx <= args->ed){
				args->sorted_mov_title[totidx] = args->filtered_mov_title[ridx];
				args->sorted_mov_score[totidx] = args->filtered_mov_score[ridx];
				++totidx; ++ridx;
			}
			for(int i = args->st; i <= args->ed; ++i){
				args->filtered_mov_score[i] = args->sorted_mov_score[i];
				args->filtered_mov_title[i] = args->sorted_mov_title[i];
			}
		}
		if(args->isThread)
			pthread_exit(0);
		else
			return (void *)0;
	}
	pthread_exit(0);
}

void* thread_func(void *tmpargs){
	threadArgs *args = (threadArgs *)tmpargs;
	int tid = args->tid;
	int reqid = args->reqid;
	int reassign = 0;
	do{
		if(reassign){
			if(front+1 >= num_of_reqs)
				break;
			pthread_mutex_lock(&lock);
			while(using){
				pthread_cond_wait(&ready, &lock);
			}
			using = true;
			reqid = pop();
			using = false;
			pthread_mutex_unlock(&lock);
			pthread_cond_broadcast(&ready);
		}
		if(reqid >= num_of_reqs)
			break;
		// int tid = (int)mytid;
		int filtered_mov_num = 0;
		//filter
		for(int i = 0; i < num_of_movies; ++i){
			if(reqs[reqid]->keywords[0] == '*' && reqs[reqid]->keywords[1] == '\0'){
				thread_args[tid].filtered_mov_title[filtered_mov_num] = movies[i]->title;
				thread_args[tid].filtered_mov_score[filtered_mov_num] = 0;
				for(int j = 0; j < NUM_OF_GENRE; ++j){
					thread_args[tid].filtered_mov_score[filtered_mov_num] += reqs[reqid]->profile[j] * movies[i]->profile[j];
				}
				++filtered_mov_num;
			}
			else if(strstr(movies[i]->title, reqs[reqid]->keywords) != NULL){
				thread_args[tid].filtered_mov_title[filtered_mov_num] = movies[i]->title;
				thread_args[tid].filtered_mov_score[filtered_mov_num] = 0;
				for(int j = 0; j < NUM_OF_GENRE; ++j){
					thread_args[tid].filtered_mov_score[filtered_mov_num] += reqs[reqid]->profile[j] * movies[i]->profile[j];
				}
				++filtered_mov_num;
			}
		}
		pthread_t worker;
		thread_args[tid].st = 0; thread_args[tid].ed = filtered_mov_num-1;
		thread_args[tid].layerid = 0;
		thread_args[tid].isThread = 1;
		pthread_create(&worker, NULL, thread_mergesort, (void *)(thread_args + tid));
		int done = pthread_join(worker, 0);
		if(done == 0){
			char filename[MAX_LEN];
			snprintf(filename, MAX_LEN, "%dt.out", reqs[reqid]->id);
			FILE *output = fopen(filename, "w");
			for(int i = 0; i < filtered_mov_num; ++i){
				fprintf(output, "%s\n", thread_args[tid].filtered_mov_title[i]);
			}
			fclose(output);
		}
		//reassign req		
		reassign = 1;

	} while(front+1 < num_of_reqs);
	pthread_exit(0);
}
// ===============================
#elif defined PROCESS
// ============process============
void filter_movies(char **filtered_mov_title, double *filtered_mov_score, int *filtered_mov_num, int *reqid){
	(*reqid) = pop();
	int keyword_len = strlen(reqs[(*reqid)]->keywords);
	*filtered_mov_num = 0;
	for(int i = 0; i < num_of_movies; ++i){
		if(reqs[(*reqid)]->keywords[0] == '*' && keyword_len == 1){
			strncpy(filtered_mov_title[*filtered_mov_num], movies[i]->title, MAX_LEN);
			filtered_mov_score[*filtered_mov_num] = 0;
			for(int j = 0; j < NUM_OF_GENRE; ++j){
				filtered_mov_score[*filtered_mov_num] += reqs[(*reqid)]->profile[j] * movies[i]->profile[j];
			}
			++(*filtered_mov_num);
		}
		else if(strstr(movies[i]->title, reqs[(*reqid)]->keywords) != NULL){
			strncpy(filtered_mov_title[*filtered_mov_num], movies[i]->title, MAX_LEN);
			filtered_mov_score[*filtered_mov_num] = 0;
			for(int j = 0; j < NUM_OF_GENRE; ++j){
				filtered_mov_score[*filtered_mov_num] += reqs[(*reqid)]->profile[j] * movies[i]->profile[j];
			}
			++(*filtered_mov_num);
		}
	}
}

void process_mergesort(int st, int ed, char **filtered_mov_title, double *filtered_mov_score, int layerid){
	if(layerid == PROCESS_MAX_LAYER || (ed - st + 1) <= SMALLCASE_SORT_NUM){
		char **sorted_mov_title = (char **)malloc(sizeof(char *) * (ed - st + 1));
		for(int i = 0; i < ed-st+1; ++i){
			sorted_mov_title[i] = (char *)malloc(sizeof(char) * MAX_LEN);
			strncpy(sorted_mov_title[i], filtered_mov_title[i+st], MAX_LEN);
		}
		sort(sorted_mov_title, filtered_mov_score+st, ed - st + 1);
		for(int i = 0; i < ed-st+1; ++i){
			strncpy(filtered_mov_title[i+st], sorted_mov_title[i], MAX_LEN);
		}
		free(sorted_mov_title);
		return;
	}
	else{
		int mid = (ed + st) >> 1;
		pid_t lpid, rpid;
		lpid = fork();
		if(lpid == 0){ //=======================left child
			process_mergesort(st, mid, filtered_mov_title, filtered_mov_score, layerid+1);
			exit(0);
		}
		else{
			rpid = fork();
			if(rpid == 0){ //=================right child
				process_mergesort(mid+1, ed, filtered_mov_title, filtered_mov_score, layerid+1);
				exit(0);
			}
			else{ //===============================parent
				pid_t status;
				int cnt = 2;
				while(cnt){
					if(waitpid(lpid, &status, 0) == lpid){
						--cnt; lpid = rpid;
					}
				}

				//merge				
				int lidx = st, ridx = mid+1, totidx = st;
				char **sorted_mov_title = (char **)malloc(sizeof(char *) * MAX_MOVIES);
				double *sorted_mov_score = (double *)malloc(sizeof(double) * MAX_MOVIES);
				while(lidx <= mid && ridx <= ed){
					if(filtered_mov_score[lidx] > filtered_mov_score[ridx]){
						sorted_mov_title[totidx] = filtered_mov_title[lidx];
						sorted_mov_score[totidx] = filtered_mov_score[lidx];
						++totidx; ++lidx;
					}
					else if(filtered_mov_score[ridx] > filtered_mov_score[lidx]){
						sorted_mov_title[totidx] = filtered_mov_title[ridx];
						sorted_mov_score[totidx] = filtered_mov_score[ridx];
						++totidx; ++ridx;
					}
					else{
						if(strcmp(filtered_mov_title[lidx], filtered_mov_title[ridx]) < 0){ //l < r
							sorted_mov_title[totidx] = filtered_mov_title[lidx];
							sorted_mov_score[totidx] = filtered_mov_score[lidx];
							++totidx; ++lidx;
						}
						else{
							sorted_mov_title[totidx] = filtered_mov_title[ridx];
							sorted_mov_score[totidx] = filtered_mov_score[ridx];
							++totidx; ++ridx;
						}
					}
				}
				while(lidx <= mid){
					sorted_mov_title[totidx] = filtered_mov_title[lidx];
					sorted_mov_score[totidx] = filtered_mov_score[lidx];
					++totidx; ++lidx;
				}
				while(ridx <= ed){
					sorted_mov_title[totidx] = filtered_mov_title[ridx];
					sorted_mov_score[totidx] = filtered_mov_score[ridx];
					++totidx; ++ridx;
				}
				for(int i = st; i <= ed; ++i){
					filtered_mov_score[i] = sorted_mov_score[i];
					filtered_mov_title[i] = sorted_mov_title[i];
				}
				free(sorted_mov_title); free(sorted_mov_score);
				exit(0);
			}
		}
	}
}
// ===============================
#endif

int main(int argc, char *argv[]){

	if(argc != 1){
#ifdef PROCESS
		fprintf(stderr,"usage: ./pserver\n");
#elif defined THREAD
		fprintf(stderr,"usage: ./tserver\n");
#endif
		exit(-1);
	}

	FILE *fp;

	if ((fp = fopen("./data/movies.txt","r")) == NULL){
		ERR_EXIT("fopen");
	}

	initialize(fp);
	assert(fp != NULL);
	fclose(fp);	

#ifdef THREAD
	// int max_thread_num = MAX_THREAD_NUM;
	// if(max_thread_num > 2) max_thread_num -= 2;
	pthread_t workers[MAX_THREAD];
	using = false;
	thread_args = (mergesortArgs *)malloc(sizeof(mergesortArgs) * max_thread_num);
	threadArgs initArgs[max_thread_num]; // for thread_func
	for(int i = 0; i < max_thread_num; ++i){
		thread_args[i].filtered_mov_title = (char **)malloc(sizeof(char *) * MAX_MOVIES);
		thread_args[i].filtered_mov_score = (double *)malloc(sizeof(double) * MAX_MOVIES);
		thread_args[i].sorted_mov_title   = (char **)malloc(sizeof(char *) * MAX_MOVIES);
		thread_args[i].sorted_mov_score   = (double *)malloc(sizeof(double) * MAX_MOVIES);
	}
#elif defined PROCESS
	
#endif

	while(front+1 < num_of_reqs){		
#ifdef THREAD
		if(front+1 < num_of_reqs && thread_cnt < max_thread_num){
			pthread_mutex_lock(&lock);
			while(using){
				pthread_cond_wait(&ready, &lock);
			}
			using = true;
			initArgs[thread_cnt].tid = thread_cnt; initArgs[thread_cnt].reqid = pop();
			using = false;
			pthread_mutex_unlock(&lock);
			pthread_cond_broadcast(&ready);
			if(initArgs[thread_cnt].reqid < num_of_reqs){
				pthread_create(&(workers[thread_cnt]), NULL, thread_func, (void *)&(initArgs[thread_cnt]));
				++thread_cnt;
			}
			else
				break;
		}
		// pthread_mutex_unlock(&lock);
		// fprintf(stderr, "front: %d, using: %d\n", front, using);
		if(thread_cnt == max_thread_num){
			// for(int i = 0; i < thread_cnt; ++i){
			// 	pthread_join(workers[i], 0);
			// }
			// thread_cnt = 0;
			break;
		}
#elif defined PROCESS
		char **filtered_mov_title = (char **)mmap(NULL, sizeof(char *) * MAX_MOVIES + sizeof(char) * MAX_LEN * MAX_MOVIES, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
		double *filtered_mov_score = (double *)mmap(NULL, sizeof(double) * MAX_MOVIES, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
		int *filtered_mov_num = (int *)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
		int *reqid = (int *)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
		char *ptr = (char *)(filtered_mov_title + MAX_MOVIES);
		for(int i = 0; i < MAX_MOVIES; ++i)
			filtered_mov_title[i] = (ptr + MAX_LEN * i);
		
		filter_movies(filtered_mov_title, filtered_mov_score, filtered_mov_num, reqid);

		pid_t worker = fork();
		if(worker == 0){ //child
			process_mergesort(0, (*filtered_mov_num)-1, filtered_mov_title, filtered_mov_score, 0);
			exit(0);
		}
		else{
			pid_t status;
			int cnt = 1;
			while(cnt){
				if(waitpid(worker, &status, 0) == worker)
					--cnt;
			}
			char filename[MAX_LEN];
			snprintf(filename, MAX_LEN, "%dp.out", reqs[(*reqid)]->id);
			FILE *output = fopen(filename, "w");
			for(int i = 0; i < (*filtered_mov_num); ++i){
				fprintf(output, "%s\n", filtered_mov_title[i]);
			}
			fclose(output);
			munmap(filtered_mov_title, sizeof(char *) * MAX_MOVIES);
			munmap(filtered_mov_score, sizeof(double) * MAX_MOVIES);
			munmap(filtered_mov_num, sizeof(int));
			munmap(reqid, sizeof(int));
		}
#endif
	}

#ifdef THREAD
	for(int i = 0; i < thread_cnt; ++i){
		pthread_join(workers[i], 0);
		free(thread_args[i].filtered_mov_title);
		free(thread_args[i].filtered_mov_score);
		free(thread_args[i].sorted_mov_title);
		free(thread_args[i].sorted_mov_score);
	}
	free(thread_args);
#elif defined PROCESS
	
#endif

	return 0;
}

/**=======================================
 * You don't need to modify following code *
 * But feel free if needed.                *
 =========================================**/

request* read_request(){
	int id;
	char buf1[MAX_LEN],buf2[MAX_LEN];
	char delim[2] = ",";

	char *keywords;
	char *token, *ref_pts;
	char *ptr;
	double ret,sum;

	scanf("%u %254s %254s",&id,buf1,buf2);
	keywords = malloc(sizeof(char)*strlen(buf1)+1);
	if(keywords == NULL){
		ERR_EXIT("malloc");
	}

	memcpy(keywords, buf1, strlen(buf1));
	keywords[strlen(buf1)] = '\0';

	double* profile = malloc(sizeof(double)*NUM_OF_GENRE);
	if(profile == NULL){
		ERR_EXIT("malloc");
	}
	sum = 0;
	ref_pts = strtok(buf2,delim);
	for(int i = 0;i < NUM_OF_GENRE;i++){
		ret = strtod(ref_pts, &ptr);
		profile[i] = ret;
		sum += ret*ret;
		ref_pts = strtok(NULL,delim);
	}

	// normalize
	sum = sqrt(sum);
	for(int i = 0;i < NUM_OF_GENRE; i++){
		if(sum == 0)
				profile[i] = 0;
		else
				profile[i] /= sum;
	}

	request* r = malloc(sizeof(request));
	if(r == NULL){
		ERR_EXIT("malloc");
	}

	r->id = id;
	r->keywords = keywords;
	r->profile = profile;

	return r;
}

/*=================initialize the dataset=================*/
void initialize(FILE* fp){

	char chunk[MAX_LEN] = {0};
	char *token,*ptr;
	double ret,sum;
	int cnt = 0;

	assert(fp != NULL);

	// first row
	if(fgets(chunk,sizeof(chunk),fp) == NULL){
		ERR_EXIT("fgets");
	}

	memset(movies,0,sizeof(movie_profile*)*MAX_MOVIES);	

	while(fgets(chunk,sizeof(chunk),fp) != NULL){
		
		assert(cnt < MAX_MOVIES);
		chunk[MAX_LEN-1] = '\0';

		const char delim1[2] = " "; 
		const char delim2[2] = "{";
		const char delim3[2] = ",";
		unsigned int movieId;
		movieId = atoi(strtok(chunk,delim1));

		// title
		token = strtok(NULL,delim2);
		char* title = malloc(sizeof(char)*strlen(token)+1);
		if(title == NULL){
			ERR_EXIT("malloc");
		}
		
		// title.strip()
		memcpy(title, token, strlen(token)-1);
	 	title[strlen(token)-1] = '\0';

		// genres
		double* profile = malloc(sizeof(double)*NUM_OF_GENRE);
		if(profile == NULL){
			ERR_EXIT("malloc");
		}

		sum = 0;
		token = strtok(NULL,delim3);
		for(int i = 0; i < NUM_OF_GENRE; i++){
			ret = strtod(token, &ptr);
			profile[i] = ret;
			sum += ret*ret;
			token = strtok(NULL,delim3);
		}

		// normalize
		sum = sqrt(sum);
		for(int i = 0; i < NUM_OF_GENRE; i++){
			if(sum == 0)
				profile[i] = 0;
			else
				profile[i] /= sum;
		}

		movie_profile* m = malloc(sizeof(movie_profile));
		if(m == NULL){
			ERR_EXIT("malloc");
		}

		m->movieId = movieId;
		m->title = title;
		m->profile = profile;

		movies[cnt++] = m;
	}
	num_of_movies = cnt;

	// request
	scanf("%d",&num_of_reqs);
	assert(num_of_reqs <= MAX_REQ);
	for(int i = 0; i < num_of_reqs; i++){
		reqs[i] = read_request();
	}
}
/*========================================================*/