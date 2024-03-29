#include "timerwheel.h"

#include <stdio.h>
#include <unistd.h>


#if 1
static int temp_flag = 0;
void timer_func_cb(struct timer_node *node)
{
    temp_flag = 0;
}
int main()
{
    printf("+++ func: %s, line: %d +++\n", __FUNCTION__, __LINE__);
    init_timer();

    add_timer(600, timer_func_cb, 100);
    temp_flag = 1;

    while (temp_flag) {
        expire_timer();
        usleep(2500);
    }
    printf("+++ func: %s, line: %d +++\n", __FUNCTION__, __LINE__);

    return 0;
}
#endif

#if 0
#include <pthread.h>
struct context {
	int quit;
    int thread;
};

struct thread_param {
	struct context *ctx;
	int id;
};

static struct context ctx = {0};

void do_timer(timer_node_t *node) {
    printf("timer expired:%d - thread-id:%d\n", node->expire, node->id);
}

void* thread_worker(void *p) {
	struct thread_param *tp = (struct thread_param *)p;
	int id = tp->id;
    struct context *ctx = tp->ctx;
	while (!ctx->quit) {
        int expire = rand() % 200; 
        add_timer(expire, do_timer, id);
        usleep(expire*(10-1)*1000);
    }
    printf("thread_worker:%d exit!\n", id);
    return NULL;
}

void do_quit(timer_node_t * node) {
    ctx.quit = 1;
}

int main() {
    srand(time(NULL));
    ctx.thread = 8;
    pthread_t pid[ctx.thread];

    init_timer();
    add_timer(600, do_quit, 100);
    struct thread_param task_thread_p[ctx.thread];
    int i;
    for (i = 0; i < ctx.thread; i++) {
        task_thread_p[i].id = i;
        task_thread_p[i].ctx = &ctx;
        if (pthread_create(&pid[i], NULL, thread_worker, &task_thread_p[i])) {
            fprintf(stderr, "create thread failed\n");
            exit(1);
        }
    }

    while (!ctx.quit) {
        expire_timer();
        usleep(2500);
    }
    clear_timer();
    for (i = 0; i < ctx.thread; i++) {
		pthread_join(pid[i], NULL);
    }
    printf("all thread is closed\n");
    return 0;
}
#endif