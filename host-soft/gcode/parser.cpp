#include "parser.hpp"
#include "gcpu.hpp"
#include "error.hpp"
#include <memory.h>


namespace parser {
    
static bool is_space(char const c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '%';      //we are not interested in program start i.e. %
}
    
static bool is_oef(char const c) {
    return c == 0;
}
    
static bool is_eol(char const c) {
    return c == '\n' || is_oef(c);
}
  
static char const * skip_spaces(g_state_t * const state, char const * cc) {      //end comments
        
    if (state->in_comments)
    {
            while(!is_eol(*cc) && *cc != ')') ++cc;
            if (*cc == ')') ++cc; state->in_comments = false;
    }
    bool again = false;
    do
    {
        while (is_space(*cc)) ++cc;
        if (!is_eol(*cc))
        {
            if (*cc=='(')   //comment
            {
                while(!is_eol(*cc) && *cc != ')') ++cc;
                if (*cc == ')') {
                    ++cc;            
                    again = true;
                }
                else 
                    state->in_comments = true; break;
            }
        }
    } while(again);
    return cc;
}

static char const * skip_number(char const * cc) {
    
    bool goOn;        
    char c;
    
lagain:    
        c = *cc;
        goOn = (c >= 0 && c <=9) || c=='.' || c=='-';
        if (goOn) { ++cc; goto lagain; }
    return cc;
}


static void handle_G(g_state_t * const state, double const val)
{
    int const ival = (int)val;
    switch (ival)
    {
        case 90: state->is_abs_mode = true; break;
        case 91: state->is_abs_mode = false; break;
        default:
            state->gnum = ival;
    }    
}

static void handle_M(g_state_t * const state, double const val)
{
    int const ival = (int)val;
}

static void handle_Letter(g_state_t * const state, double const val, char c)
{
    state->vals[state->letters_size] = val;
    state->letters[state->letters_size] = c;
    state->letters_size++;    
}

void handle_linear_move(g_state_t * const state)
{
        for(int ai = 0; ai < state->letters_size; ++ai) {
            char const l = state->letters[ai];
            double const val = state->vals[ai];                        
            int motor;
            switch (l)
            {
                case 'X': motor = 0;
                case 'Y': motor = 1;
                case 'Z': motor = 2;
                default: motor = -1;
            }
            
            if (motor != -1)
                gcpu::send_motor_to(state, motor, val);
            else
                error::write_msg(state, "handle_linear_move, line %d - unsupported letter: %c; skipping", state->line_number, l);
        }
}


void commit(g_state_t * const state)
{
    gcpu::reset_completion(&state->comp);
    switch(state->gnum)
    {
        case 0: //rapid positioning
        case 1: //move
            handle_linear_move(state);
            break;
        case 2: //cw arc
            break;
        case 3: //ccw arc
            break;
        case 4: //dwell/wait; do nothing...
            break;
        case 20: //inches as units
            break;
        case 21: //mm as units
            break;
        case 28: //go home
            break;
        case 30: //go home via intermediate point
            break;
        //case 90: //Absolute Positioning            //argument-less, so handled in handle_G function
        //case 91: //incremental positioning            
        //    break;
        case 92: //current as home
            break;
        default:
            error::write_msg(state, "line %d; Unsopported G: %d; skipping", state->line_number, state->gnum);            
            break;
    }
}


void interpret(char const * const program)
{
    
}

void interpret_line(g_state_t * const state, char const * const line)
{
    char const * cc = line;
    gcpu::wait_completion(&state->comp);
    
    do
    {
        cc = skip_spaces(state, cc);      
        char const curLetter = *cc;
        
        if (is_eol(curLetter))  //we've done
        {
            commit(state);
            break;
        }
        ++cc;
        
        cc = skip_spaces(state, cc);    //this also skips comments
        
        double val = 0.;
        int gotten = sscanf(cc, "%lf", &val);
        if (!gotten)
            error::throw_err<error::interpret_error>(state, "We got code: %c, column: %d and expected number here", curLetter, cc-line);
        else
            cc = skip_number(cc);
                    
        switch (curLetter)
        {        
            case 'G': handle_G(state, val); break;
            case 'M': handle_M(state, val); break;
            
            case 'X': case 'Y': case 'Z': 
            case 'I': case 'J': case 'K':
            case 'R':
                handle_Letter(state, val, curLetter); break;
                                            
            default: 
                error::write_msg(state, "Unknown code: %c; skipping\n");            
        };
    } while(1);    
}

}
