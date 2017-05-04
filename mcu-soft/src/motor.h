#pragma once
#include "main.h"

struct cmd_t;
struct cmd_state_t;

struct motor_t
{
    GPIO_TypeDef *  pulse_group;
    int             pulse_pin;
    GPIO_TypeDef *  dir_group;
    int             dir_pin;    
    
    GPIO_TypeDef *  stop_group;     //sensor stop pin group
    int             stop_pin;
    int             stop_pin_dir;   //stop will stop motor only if current motor move direction (see motor_cmd_state_t::dir) is stop_pin_dir
                                    //this will allow motor to leave blind zero point
};


struct cmd_state_t * motor_cmd_state_create(struct cmd_t * const cmd);
