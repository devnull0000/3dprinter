#include "gcpu.hpp"
#include "error.hpp"
#include "mcu-soft/src/command.h"
#include "uart.hpp"
#include <memory.h>

namespace gcpu {
static int gs_cmd_id = 0;
    
    
static int64_t rel_location_in_msteps(g_state_t * const state, int const motor, double const location)
{
    int64_t ret;
    if (state->is_location_in_inches)
        ret = location / c_pitch_step_mm * c_mstep_rev;
    else
        ret = location * 25.4 / c_pitch_step_mm * c_mstep_rev;
            
    if (state->is_abs_mode) 
            ret += state->motorLocMS[motor];
    
    return ret;
}


void send_motor_to(g_state_t * const state, int const motor, double const location)
{
    int64_t const rel_location_ms = rel_location_in_msteps(state, motor, location);
    cmd_t c;
    memset(&c, 0, sizeof(c));
    c.cmd = eCmd_MotorMove;
    c.cmd_id = add_to_completion(state, &state->comp);
    c.args[0] = motor;
    c.args[1] = rel_location_ms;
    c.args[2] = state->gnum == 0 ? c_motor_rapid_speed : c_motor_work_speed;
    uart_write(&c, sizeof(c));    
} 

void go_hardware_home(g_state_t * const state)
{
    cmd_t c;
    memset(&c, 0, sizeof(c));
    c.cmd = eCmd_MotorGoHome;    
    c.args[2] = state->gnum == c_motor_rapid_speed;    
    for (int motorIdx = 0; motorIdx < c_num_motors; ++motorIdx)
    {
        c.args[0] = 0;    
        c.cmd_id = add_to_completion(state, &state->comp);
        uart_write(&c, sizeof(c));        
    }        
}

void reset_completion(pcompletion_t * const comp)
{
    memset(comp, 0, sizeof(pcompletion_t));
}

int add_to_completion(g_state_t const * const debug_info, pcompletion_t * const comp, int const cmd_id)
{
    if (comp->num_cmds + 1 == c_max_pcmds)
        error::throw_err<error::gcpu_error>(debug_info, "gcpu::add_to_completion, too much commands, maximum %d is allowed", c_max_pcmds);
 
    int ecmd_id = cmd_id;
    if (cmd_id == -1)
        ecmd_id = ++gs_cmd_id;
    
    comp->status[comp->num_cmds].cmd_id = ecmd_id;
    comp->status[comp->num_cmds].pending = true;
    comp->num_cmds++;
    
    return ecmd_id;        
}

void wait_completion(pcompletion_t * const comp)
{
}



}
