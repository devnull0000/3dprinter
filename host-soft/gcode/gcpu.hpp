#include "gcommand.hpp"

namespace gcpu {
void send_motor_to(g_state_t * const state, int const motor, double const location);

void reset_completion(pcompletion_t * const comp);
int add_to_completion(g_state_t const * const debug_info, pcompletion_t * const comp, int const cmd_id = -1);      //if cmd_id==-1 it means create new via an auto increment
void wait_completion(pcompletion_t * const comp);  //automatically is called from interpret_line

void go_hardware_home(g_state_t * const state);     //send all motors to 0,0 absolute point; hardware_home as opposed to G-home which can change

}
