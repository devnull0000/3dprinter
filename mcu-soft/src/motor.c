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
    
    //YS: I think I faced with what is called electromagnetic interference (EMI) or токи наводки (in russian)
    //that is my input sensor time to time shows false positive, so I have no place on current PCB for a capacitor, as well as I have no desire
    //to use shielded cable, so I'll try to just use "number of attempts" approach .. i.e. if we read pin 5 times and it was set, then we will assume
    //that input is really set.
    int32_t                stop_pin_countdown;      
};

enum { c_stop_pin_countdown = 5 };

struct motor_t const cMotors[c_num_motors] = 
{
    {.pulse_group = GPIOD, .pulse_pin=GPIO_PIN_5, .dir_group = GPIOD, .dir_pin=GPIO_PIN_6, .stop_group=GPIOE, .stop_pin=GPIO_PIN_0, .stop_pin_dir=1}
   ,{.pulse_group = GPIOD, .pulse_pin=GPIO_PIN_7, .dir_group = GPIOD, .dir_pin=GPIO_PIN_8, .stop_group=GPIOE, .stop_pin=GPIO_PIN_1, .stop_pin_dir=1}
   ,{.pulse_group = GPIOD, .pulse_pin=GPIO_PIN_9, .dir_group = GPIOD, .dir_pin=GPIO_PIN_10, .stop_group=GPIOE, .stop_pin=GPIO_PIN_2, .stop_pin_dir=1}   
};


static enum ECmdStatus motor_process_fn(struct motor_cmd_state_t * const ms)
{
    enum ECmdStatus ret;
            
    struct motor_t const * const m = &cMotors[ms->base.cmd.args[0]];
    int const stop_pin_imm = (GPIO_PIN_SET == HAL_GPIO_ReadPin(m->stop_group, m->stop_pin));
    int stop_pin = 0;    
    if (stop_pin_imm) {
        if(0 == --ms->stop_pin_countdown) {
            stop_pin = 1;
            ms->stop_pin_countdown = c_stop_pin_countdown;
        }
    }
    else
        ms->stop_pin_countdown = c_stop_pin_countdown;
    
    int const motor_at_base = (stop_pin && m->stop_pin_dir == ms->dir);
    
    if (ms->total_steps && !motor_at_base) {
        ret = eCmdStatus_Pending;
        uint32_t const cur_time = main_get_cpu_ticks();
        uint32_t const diff = cur_time - ms->cpu_cntr;
        if (diff > ms->max_ccps_speed) {                    
            ms->cpu_cntr = ms->cpu_cntr + ms->max_ccps_speed;       //avoid drift if possible
            
            if (!ms->pulse_edge)
                ms->total_steps--;
            ms->pulse_edge = !ms->pulse_edge;
            
            //toggle pin            
            HAL_GPIO_TogglePin(m->pulse_group, m->pulse_pin);        
        }
    } else {
        ret = eCmdStatus_Ok;
    }
            
    return ret;
}

struct cmd_state_t * motor_cmd_state_create(struct cmd_t * const cmd)
{
    struct motor_cmd_state_t * const ret = (struct motor_cmd_state_t *)malloc(sizeof(struct motor_cmd_state_t));
    struct motor_t const * const m = &cMotors[cmd->args[0]];
    char const goHomeSign = (m->stop_pin_dir)? 1 : -1;        
    int const total_steps = cmd->cmd == eCmd_MotorGoHome ? INT32_MAX * goHomeSign: cmd->args[1];
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
    ret->stop_pin_countdown = c_stop_pin_countdown;
        
    HAL_GPIO_WritePin(m->dir_group, m->dir_pin, ret->dir == 1 ? GPIO_PIN_RESET : GPIO_PIN_SET);     
    
    return (struct cmd_state_t *)ret;
}
