#include "motor.h"
#include "command.h"
#include "main.h"
#include <stdlib.h>

struct motor_cmd_state_t
{
    struct cmd_state_t     base;                    //base state
    uint32_t               total_steps;             //number of steps for the command
    int8_t                 dir;                     //direction 0 - forward, 1 - backward... but it depends on how you've connected your motor
    int8_t                 pulse_edge;              //current motor driver pulse edge (raising or falling)
    uint32_t               max_speed_in_cpu_cntr;   //speed in CPU clock cycles multiplied by 2 (because we need to send pulses i.e. turn on/off pair to make one step)
    
    uint32_t               cpu_cntr;        //cpu cntr last time the code was executed    
    uint32_t               steps_remained;  //count to zero.. how much we should done
};

struct motor_t const cMotors[c_num_motors] = 
{
    {.pulse_group = GPIOD, .pulse_pin=GPIO_PIN_5, .dir_group = GPIOD, .dir_pin=GPIO_PIN_6}
   ,{}
   ,{}   
};


static enum ECmdStatus process_fn(struct motor_cmd_state_t * const ms)
{
    enum ECmdStatus ret;
            
    if (ms->total_steps) {
        ret = eCmdStatus_Pending;
        
        if (ms->pulse_edge)
            ms->total_steps--;
        ms->pulse_edge = !ms->pulse_edge;
        
        //toggle pin
        struct motor_t const * const m = &cMotors[ms->base.cmd.args[0]];
        
        
    } else {
        ret = eCmdStatus_Ok;
    }
            
    return ret;
}

struct cmd_state_t * motor_cmd_state_create(struct cmd_t * const cmd)
{
    struct motor_cmd_state_t * const ret = (struct motor_cmd_state_t *)malloc(sizeof(struct cmd_state_t));
    
        
    return (struct cmd_state_t *)ret;
}
