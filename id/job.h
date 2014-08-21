/**
 * @file job.h
 * @brief job management
 * @author Ben Blum <bblum@andrew.cmu.edu>
 */

#ifndef __ID_JOB_H
#define __ID_JOB_H

#include <pthread.h>

#include "pp.h"
#include "io.h"

struct job {
	struct pp_set *config;
	struct file config_file;
	struct file results_file;
	struct file log_stdout;
	struct file log_stderr;
	unsigned int id;
	unsigned int generation; /* max among generations of pps + 1 */
	bool done;
	pthread_cond_t done_cvar;
	pthread_mutex_t done_lock;
	pthread_mutex_t config_lock; /* protects everything but 'done' */
};

struct job *new_job(struct pp_set *config);
void start_job(struct job *j);
void wait_on_job(struct job *j);
void finish_job(struct job *j);


#endif