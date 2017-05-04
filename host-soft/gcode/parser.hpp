#pragma once

struct g_state_t;

namespace parser {

    
void interpret(char const * const program);
void interpret_line(g_state_t * const state, char const * const line);   //interpret one line, it may end with \n or with \0


}
