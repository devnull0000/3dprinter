#pragma once

enum {
     c_mstep_rev      = 51200      //micro-steps per revolution    
    //,c_num_motors     = 3          //see mcu-soft/src/command.h
};
constexpr double c_pitch_step_mm = 5.;       //thread pitch (i.e. difference between two notches on a ball screw) in millimeters

constexpr double c_mstep_length_in_mm = c_pitch_step_mm / c_mstep_rev;
//constexpr double c_mm_ = c_mstep_rev / c_pitch_step_mm;

enum {
     c_max_gcmds = 6 //how many g-commands can be executed simultaneously (i.e. xyz ijk for G02 (arc))
    ,c_max_pcmds = 3     //how many printer commands we can wait simultaneously to finish
};

enum {
     c_motor_rapid_speed = 240000000     //in microsteps per second; positioning/go home
    ,c_motor_work_speed =  24000000
};

// struct gcmd_t
// {
//      char    letter;     //i.e. G, X, Y so on. 0 - means end of list
//      double  val;
// };
// 
// struct gpos_t        //future "Position" of a printer head. One line of g-code; a list of commands which are intended to be executed simultaneously.
// {
//     gcmd_t  cmds[c_max_gcmds];
// };


struct pcmd_status_t
{
    int     cmd_id;
    bool    pending;        //false means 'done'
};

struct pcompletion_t
{
    pcmd_status_t  status[c_max_pcmds];       //if all of them are done, we can go to next G-code line/position
    int            num_cmds = 0;
};

struct g_state_t
{
    int             gnum = 0;               //current G-number
    int             motorLocMS[3];          //abs location of motors in microsteps (at the entry on the line)

    double          vals[26];               //value letters i.e. X, Y so on which were set on the line i.e. for X-2.0 val[23] == -2.0
    char            letters[26];            //contains letters which were set in vals i.e. if there was X2Y0Z-1 letters will have XYZ\0
    int             letters_size;           //number of actual valid letters in letters above
    
    bool            is_abs_mode = true;             //G90/G91    
    bool            is_location_in_inches = false;  //if true - in millimeters
    
    pcompletion_t   comp;                   //status of commands which are executing on the printer now
    
    int             line_number = 1;            //in text file for debug help
    bool            in_comments = false;            //we are inside () comments,
};

