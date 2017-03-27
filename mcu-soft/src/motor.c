#include "motor.h"
#include "command.h"
#include "main.h"
#include <stdlib.h>
#include <math.h>

struct motor_cmd_state_t
{
    struct cmd_state_t     base;                    //base state
    uint32_t               total_steps;             //number of steps for the command
    int8_t                 dir;                     //direction 0 - forward, 1 - backward... but it depends on how you've connected your motor
    int8_t                 pulse_edge;              //current motor driver pulse edge (raising or falling)
    uint32_t               max_ccps_speed;          //cpu cycles per micro step (or how much them to skip) * 2 becayse we account pulse edge (raising/failing)
                               
    uint32_t               cpu_cntr;                //cpu cntr last time the code was executed    
    uint32_t               steps_remained;          //count to zero.. how much we should done
};

struct motor_t const cMotors[c_num_motors] = 
{
    {.pulse_group = GPIOD, .pulse_pin=GPIO_PIN_5, .dir_group = GPIOD, .dir_pin=GPIO_PIN_6}
   ,{.pulse_group = GPIOD, .pulse_pin=GPIO_PIN_7, .dir_group = GPIOD, .dir_pin=GPIO_PIN_8}
   ,{.pulse_group = GPIOD, .pulse_pin=GPIO_PIN_9, .dir_group = GPIOD, .dir_pin=GPIO_PIN_10}   
};


static enum ECmdStatus motor_process_fn(struct motor_cmd_state_t * const ms)
{
    enum ECmdStatus ret;
            
    if (ms->total_steps) {
        ret = eCmdStatus_Pending;
        uint32_t const cur_time = main_get_cpu_ticks();
        uint32_t const diff = cur_time - ms->cpu_cntr;
        if (diff > ms->max_ccps_speed) {                    
            ms->cpu_cntr = ms->cpu_cntr + ms->max_ccps_speed;       //avoid drift if possible
            
            if (!ms->pulse_edge)
                ms->total_steps--;
            ms->pulse_edge = !ms->pulse_edge;
            
            //toggle pin
            struct motor_t const * const m = &cMotors[ms->base.cmd.args[0]];
            HAL_GPIO_TogglePin(m->pulse_group, m->pulse_pin);        
        }
    } else {
        ret = eCmdStatus_Ok;
    }
            
    return ret;
}

struct cmd_state_t * motor_cmd_state_create(struct cmd_t * const cmd)
{
    struct motor_cmd_state_t * const ret = (struct motor_cmd_state_t *)malloc(sizeof(struct cmd_state_t));
    int const total_steps = cmd->args[1];
    int const cpu_freq = main_cpu_clock_speed();
    int const max_speed = cmd->args[2];         //micro_steps per second multiplied by 100
    
    ret->base.cmd = *cmd;
    ret->base.proc_fn = (cmd_process_fn_t)motor_process_fn;
    ret->dir = total_steps < 0 ? 1 : 0;
    ret->pulse_edge = 1;        //it's because we use hex invertors!
    ret->total_steps = abs(total_steps);
    ret->steps_remained = ret->total_steps;
    ret->cpu_cntr = main_get_cpu_ticks();
    ret->max_ccps_speed = (cpu_freq / max_speed) * 100;
    
    struct motor_t const * const m = &cMotors[cmd->args[0]];
    HAL_GPIO_WritePin(m->dir_group, m->dir_pin, ret->dir == 1 ? GPIO_PIN_SET : GPIO_PIN_RESET);     
    
    return (struct cmd_state_t *)ret;
}
