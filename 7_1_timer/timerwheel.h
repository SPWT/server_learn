/*
 * @Author: your name
 * @Date: 2021-07-12 21:35:50
 * @LastEditTime: 2021-07-18 10:30:56
 * @LastEditors: your name
 * @Description: In User Settings Edit
 * @FilePath: /server_learn/7_1_timer/timerwheel.h
 */
#ifndef __TIMERWHEEL_H__
#define __TIMERWHEEL_H__

#include <stdint.h>

#define TIME_NEAR_SHIFT 8
#define TIME_NEAR (1 << TIME_NEAR_SHIFT)
#define TIME_NEAR_MASK (TIME_NEAR - 1)

#define TIME_LEVEL_SHIFT 6
#define TIME_LEVEL (1 << TIME_LEVEL_SHIFT)
#define TIME_LEVEL_MASK (TIME_LEVEL - 1)

typedef void (*handler_pt)(struct timer_node *node);

typedef struct timer_node {
    struct timer_node *next;
    uint32_t expire;
    handler_pt callback;
    uint8_t cancel;
    int id;
} timer_node_t;


timer_node_t *add_timer(int time, handler_pt func, int threadid);

void del_timer(timer_node_t *node);

void expire_timer(void);

void init_timer(void);

void clear_timer(void);

#endif