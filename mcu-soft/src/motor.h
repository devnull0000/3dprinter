#pragma once
#include "main.h"

struct cmd_t;
struct cmd_state_t;

enum {c_num_motors = 4};        //0 & 1 are paired x-direction motors

struct motor_t
{
    GPIO_TypeDef *  pulse_group;
    int             pulse_pin;
    GPIO_TypeDef *  dir_group;
    int             dir_pin;    
};


struct cmd_state_t * motor_cmd_state_create(struct cmd_t * const cmd);
