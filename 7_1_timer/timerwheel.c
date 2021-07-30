#include "spinlock.h"
#include "timerwheel.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <time.h>

typedef struct link_list {
    timer_node_t head;
    timer_node_t *tail;
} link_list_t;

typedef struct timer {
    link_list_t near[TIME_NEAR];
    link_list_t t[4][TIME_LEVEL];
    struct spinlock lock;
    uint32_t time;
    uint64_t current;
    uint64_t current_point;
} s_timer_t;

static s_timer_t *TI = NULL;

/**
 * @brief 移除list下面所有的节点
 * @param {link_list_t} *list
 * @return {timer_node_t} *ret 节点的链表头
 */
timer_node_t *link_clear(link_list_t *list)
{
    timer_node_t *ret = list->head.next;

    list->head.next = 0;
    list->tail = &(list->head);

    return ret;
}

void link(link_list_t *list, timer_node_t *node)
{
    list->tail->next = node;
    list->tail = node;
    node->next = 0;
}

void add_node(s_timer_t *T, timer_node_t *node)
{
    uint32_t time = node->expire;
    uint32_t current_time = T->time;
    uint32_t msec = time - current_time;

    if (msec < TIME_NEAR) { //[0, 0x100)
        link(&T->near[time & TIME_NEAR_MASK], node);
    } else if (msec < (1 << (TIME_NEAR_SHIFT + TIME_LEVEL_SHIFT))) { //[0x100, 0x4000)
        link(&T->t[0][((time >> TIME_NEAR_SHIFT) & TIME_LEVEL_MASK)], node);
    } else if (msec < (1 << (TIME_NEAR_SHIFT + 2*TIME_LEVEL_SHIFT))) { //[0x4000, 0x100000)
        link(&T->t[1][((time >> (TIME_NEAR_SHIFT + TIME_LEVEL_SHIFT)) & TIME_LEVEL_MASK)], node);
    } else if (msec < (1 << (TIME_NEAR_SHIFT + 3*TIME_LEVEL_SHIFT))) { //[0x100000, 0x4000000)
        link(&T->t[2][((time >> (TIME_NEAR_SHIFT + 2*TIME_LEVEL_SHIFT)) & TIME_LEVEL_MASK)], node);
    } else { //[0x4000000, 0xffffffff]
        link(&T->t[3][((time >> (TIME_NEAR_SHIFT + 3*TIME_LEVEL_SHIFT)) & TIME_LEVEL_MASK)], node);
    }
}

/**
 * @brief 从timer子数组(level)取出idx的链表节点, 并重新添加到timer中
 * @param {s_timer_t} *T
 * @param {int} level
 * @param {int} idx
 * @return {*}
 */
void move_list(s_timer_t *T, int level, int idx)
{
    timer_node_t *current = link_clear(&T->t[level][idx]);

    while(current) {
        timer_node_t *temp = current->next;
        add_node(T, current);
        current = temp;
    }
}

/**
 * @brief 重新映射, 将该层时间节点下的链表重新映射到上一层去
 * @param {s_timer_t} *T
 * @return {*}
 */
void timer_shift(s_timer_t *T)
{
    int mask = TIME_NEAR;
    uint32_t ct = ++T->time;

    if (ct == 0) {
        move_list(T, 3, 0);
    } else {
        // ct / 256
        uint32_t time = ct >> TIME_NEAR_SHIFT;
        int i = 0;

        // ct % 256 == 0
        while ((ct & (mask - 1)) == 0) {
            int idx = time & TIME_LEVEL_MASK;

            if (idx != 0) {
                move_list(T, i, idx);
                break;
            }
            mask <<= TIME_LEVEL_SHIFT;
            time >>= TIME_LEVEL_SHIFT;
            ++i;
        }
    }
}

/**
 * @brief 执行所有节点的回调
 * @param {timer_node_t} *current
 * @return {*}
 */
void dispatch_list(timer_node_t *current)
{
    do {
        timer_node_t *temp = current;

        current = current->next;
        if (temp->cancel == 0) {
            temp->callback(temp);
        }
        free(temp);
    } while (current);
}

void timer_execute(s_timer_t *T)
{
    int idx = T->time & TIME_NEAR_MASK;

    while (T->near[idx].head.next) {
        timer_node_t *current = link_clear(&T->near[idx]);
        spinlock_unlock(&T->lock);
        dispatch_list(current);
        spinlock_lock(&T->lock);
    }
}

/**
 * @brief 更新定时器
 * @param {s_timer_t} *T
 * @return {*}
 */
void timer_update(s_timer_t *T)
{
    spinlock_lock(&T->lock);
    timer_execute(T);
    timer_shift(T);
    timer_execute(T);
    spinlock_unlock(&T->lock);
}

s_timer_t *timer_create_timer(void)
{
    s_timer_t *timer = (s_timer_t *)malloc(sizeof(s_timer_t));
    memset(timer, 0, sizeof(s_timer_t));

    int i, j;
    for (i = 0; i < TIME_NEAR; i++) {
        link_clear(&timer->near[i]);
    }
    for (i = 0; i < sizeof(timer->t) / sizeof(timer->t[0]); i++) {
        for (j = 0; j < TIME_NEAR; j++) {
            link_clear(&timer->t[i][j]);
        }
    }

    spinlock_init(&timer->lock);
    timer->current = 0;

    return timer;
}

uint64_t gettime(void)
{
    uint64_t t;
    struct timespec ti;

    clock_gettime(CLOCK_MONOTONIC, &ti);
    t = (uint64_t)ti.tv_sec * 100;
    t += ti.tv_nsec / 10000000; // 控制定时器精度, 虽小可调到1ns, 当前为10ms

    return t;
}



/**
 * @brief 添加定时任务
 * @param {int} time
 * @param {handler_pt} func
 * @param {int} threadid
 * @return {*}
 */
timer_node_t *add_timer(int time, handler_pt func, int threadid)
{
    timer_node_t *node = (timer_node_t *)malloc(sizeof(timer_node_t));

    spinlock_lock(&TI->lock);
    node->expire = time + TI->time;// 根据时间精度, 每一次加加1(即精度为10ms, 则每10ms加1)
    node->callback = func;
    node->id = threadid;
    if (time <= 0) {
        node->callback(node);
        free(node);
        spinlock_unlock(&TI->lock);
        return NULL;
    }

    add_node(TI, node);
    spinlock_unlock(&TI->lock);

    return node;
}

/**
 * @brief 取消定时任务
 * @param {timer_node_t} *node
 * @return {*}
 */
void del_timer(timer_node_t *node)
{
    node->cancel = 1;
}

/**
 * @brief 遍历定时器, 并执行过期的定时器回调
 * @param {*}
 * @return {*}
 */
void expire_timer(void)
{
    uint64_t cp = gettime();

    if (cp != TI->current_point) {
        uint32_t diff = (uint32_t)(cp - TI->current_point);
        TI->current_point = cp;
        int i;
        for (i = 0; i < diff; i++) {
            timer_update(TI);
        }
    }
}

void init_timer(void)
{
    TI = timer_create_timer();
    TI->current_point = gettime();
}

void clear_timer(void)
{
    int i, j;

    for (i = 0; i < TIME_NEAR; i++) {
        link_list_t *list = &TI->near[i];
        timer_node_t *current = list->head.next;

        while (current) {
            timer_node_t *temp = current;
            current = current->next;
            free(temp);
        }
        link_clear(&TI->near[i]);
    }

    for (i = 0; i < sizeof(TI->t)/sizeof(TI->t[0]); i++) {
        for (j = 0; j < TIME_LEVEL; j++) {
            link_list_t *list = &TI->t[i][j];
            timer_node_t *current = list->head.next;

            while (current) {
                timer_node_t *temp = current;
                current = current->next;
                free(temp);
            }
            link_clear(&TI->t[i][j]);
        }
    }
}
