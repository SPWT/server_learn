#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define LL_ADD(item, list) do {					\
	item->prev = NULL;							\
	item->next = list;							\
	if (list != NULL) list->prev = item;		\
	list = item;								\
} while (0)

#define LL_REMOVE(item, list) do {							\
	if (item->prev != NULL) item->prev->next = item->next;	\
	if (item->next != NULL) item->next->prev = item->prev;	\
	if (item == list) list = item->next;					\
	item->prev = item->next = NULL;							\
} while (0)

typedef struct _NWORKER {
	pthread_t threadid;
	int terminate;

	struct _NMANAGER *pool;

	struct _NWORKER *prev;
	struct _NWORKER *next;
} nWorker;

typedef struct _NJOB {
	void (*func)(void *arg);
	void *user_data;

	struct _NJOB * prev;
	struct _NJOB *next;
} nJob;

typedef struct _NMANAGER {
	nWorker *workers;
	nJob *jobs;

	int max_thread;
	int free_thread;

	pthread_mutex_t jobs_mtx;
	pthread_cond_t jobs_cond;
} nManager;

typedef nManager nThreadPool;

int ExtendThreadPool(nThreadPool *pool);
int ReduceThreadPool(nThreadPool *pool);

void *nWorkerCallback(void *arg)
{
	nWorker *worker = (nWorker *)arg;

	while (1)
	{
		pthread_mutex_lock(&worker->pool->jobs_mtx);
		while (worker->pool->jobs == NULL)
		{
			if (worker->terminate == 1) break;
			pthread_cond_wait(&worker->pool->jobs_cond, &worker->pool->jobs_mtx);
		}

		worker->pool->free_thread--;

		if (worker->terminate == 1)
		{
			worker->pool->free_thread++;
			pthread_mutex_unlock(&worker->pool->jobs_mtx);
			break;
		}

		nJob *job = worker->pool->jobs;

		if (job != NULL)
		{
			LL_REMOVE(job, worker->pool->jobs);
		}

		pthread_mutex_unlock(&worker->pool->jobs_mtx);

		if (job == NULL)
		{
			worker->pool->free_thread++;
			continue;
		}

		ExtendThreadPool(worker->pool);
		job->func(job->user_data);

		pthread_mutex_lock(&worker->pool->jobs_mtx);
		worker->pool->free_thread++;
		pthread_mutex_unlock(&worker->pool->jobs_mtx);
		free(job);

		ReduceThreadPool(worker->pool);
	}

	LL_REMOVE(worker, worker->pool->workers);
	free(worker);
}

int nThtreadPoolCreate(nThreadPool *pool, int numWorkers)
{
	int i;

	if (pool == NULL) return -1;
	if (numWorkers < 1) numWorkers = 1;
	memset(pool, 0, sizeof(nThreadPool));

	pthread_mutex_t jobs_mtx = PTHREAD_MUTEX_INITIALIZER;
	memcpy(&pool->jobs_mtx, &jobs_mtx, sizeof(pthread_mutex_t));

	pthread_cond_t jobs_cond = PTHREAD_COND_INITIALIZER;
	memcpy(&pool->jobs_cond, &jobs_cond, sizeof(pthread_cond_t));

	for (i = 0; i < numWorkers; i++)
	{
		nWorker *worker = (nWorker *)malloc(sizeof(nWorker));
		if (worker == NULL)
		{
			perror("malloc");
			return 1;
		}
		memset(worker, 0, sizeof(nWorker));
		worker->pool = pool;

		if (pthread_create(&worker->threadid, NULL, nWorkerCallback, worker) != 0)
		{
			nWorker *w = NULL;
			perror("pthread_create");
			for (w = pool->workers; w != NULL; w = w->next)
			{
				w->terminate = 1;
			}
			return 2;
		}
		LL_ADD(worker, pool->workers);
		pool->max_thread++;
		pool->free_thread = pool->max_thread;
	}

	return 0;
}

int ExtendThreadPool(nThreadPool *pool)
{
	int free = 0, max = 0;

	pthread_mutex_lock(&pool->jobs_mtx);
	free = pool->free_thread;
	max = pool->max_thread;

	while ((float)free / (float)max <= 0.5)
	{
		nWorker *worker = (nWorker *)malloc(sizeof(nWorker));
		if (worker == NULL)
		{
			perror("malloc");
			return 1;
		}
		memset(worker, 0, sizeof(nWorker));
		worker->pool = pool;

		if (pthread_create(&worker->threadid, NULL, nWorkerCallback, worker) != 0)
		{
			nWorker *w = NULL;
			perror("pthread_create");
			for (w = pool->workers; w != NULL; w = w->next)
			{
				w->terminate = 1;
			}
			return 2;
		}
		LL_ADD(worker, pool->workers);
		max = ++pool->max_thread;
		free = ++pool->free_thread;
	}
	pthread_mutex_unlock(&pool->jobs_mtx);

	return 0;
}

int ReduceThreadPool(nThreadPool *pool)
{
	int free = 0, max = 0;
	nWorker *w = NULL;

	pthread_mutex_lock(&pool->jobs_mtx);
	free = pool->free_thread;
	max = pool->max_thread;

	while ((float)free / (float)max >= 0.8)
	{
		for (w = pool->workers; w->terminate == 1; w = w->next)
		{
		}
		if (w != NULL)
		{
			w->terminate = 1;

			free = --pool->free_thread;
			max = --pool->max_thread;
		}
	}
	pthread_mutex_unlock(&pool->jobs_mtx);
}

int nThreadPoolDestory(nThreadPool *pool)
{
	nWorker *w = NULL;
	for (w = pool->workers; w != NULL; w = w->next)
	{
		w->terminate = 1;
	}

	pthread_mutex_lock(&pool->jobs_mtx);
	pthread_cond_broadcast(&pool->jobs_cond);
	pthread_mutex_unlock(&pool->jobs_mtx);
}

int nThreadPoolPushJob(nThreadPool *pool, nJob *job)
{
	pthread_mutex_lock(&pool->jobs_mtx);
	LL_ADD(job, pool->jobs);

	pthread_cond_signal(&pool->jobs_cond);
	pthread_mutex_unlock(&pool->jobs_mtx);

	return 0;
}

int nJobCreate(nThreadPool *pool, void *callback, void *arg)
{
	nJob *job = (nJob *)malloc(sizeof(nJob));
	memset(job, 0, sizeof(nJob));
	job->func = callback;
	job->user_data = arg;

	nThreadPoolPushJob(pool, job);
}

void HelloFunc(void *arg)
{
	int *i = arg;
	printf("[%d]Hello!!!\n", *i);
//	while(1);
}

int main()
{
	nThreadPool pool;

	nThtreadPoolCreate(&pool, 2);

	int i, count[4] = {0};
	for (i = 0; i < sizeof(count)/sizeof(count[0]); i++)
	{
		count[i] = i;
		nJobCreate(&pool, HelloFunc, &count[i]);
	}


	sleep(5);
	pthread_mutex_lock(&pool.jobs_mtx);
	printf("+++ func: %s, line: %d +++, max: %d, free: %d\n", __FUNCTION__, __LINE__, pool.max_thread, pool.free_thread);
	pthread_mutex_unlock(&pool.jobs_mtx);

	while (1);

	return 0;
}


