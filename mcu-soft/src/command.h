#pragma once
#include <stdint.h>

enum { cmd_rb_size = 512 };

struct cmd_ring_buffer_t
{
    char        buf[cmd_rb_size];
    uint32_t    r_ptr;  //read
    uint32_t    w_ptr;  //write
};

void cmd_data_arrived(uint8_t const * const data, int const length);        //interrupt context, length is assumed to be a multiple of 4 (sizeof(int32_t))

//take as much commands from buffer as possible, returns number of bytes taken from the buffer (they can be skipped)
//void cmd_process_ring_buffer(struct cmd_ring_buffer_t * const rb);

//make one pass of commands which are executing currently
//the function should be called from a loop
void cmd_iterate_command_loop();

//mcu will never exit it...
void cmd_enter_command_loop();


enum EHowToMove
{
     eHowToMove_Lineary
    //,eHowToMove_FadeIn
    //,eHowToMove_FadeOut
    //,eHowToMove_FadeInOut
};


//num_msteps - are signed number of micro steps == number of impulses sent to motor driver
//max_speed/move_speed is unsigned number of micro_steps per second multiplied by 100
//sample valid command on highest microstep resolution: mm 0 -100000 2400000 (motor move, motor0, make minus 100000 steps, at 2400000 speed)
enum ECmd
{                       
     eCmd_MCUReset      //no args, self-reset
 // ,eCmd_Wait          //arg0: int mcs_to_wait;                                                      wait for specified microseconds, then send cmd_result; doesn't affect MotorMove commands
    ,eCmd_Led           //arg0: int led_index;      arg1: int turn_on/off ? ;                         turn on or off led
    ,eCmd_MotorMove     //arg0: int motor_index;    arg1: int num_msteps;   ; 
                        //arg2: int max_speed  ;    arg3: EHowToMove how_to_move;
    ,eCmd_MotorStop     //arg0: int motor_index;    arg1: bool immediately?;                          stops the motor, cmd_result gets eCmdStatus_Canceled status
    ,eCmd_MotorGoHome   //arg0: int motor_index;    arg1: unused            
                        //arg2: int max_speed  ;                                                      moves motor to its home (initial/zero) location
    ,eCmd_MotorTestPin  //arg0: select hex invertor (==0 - first, it will try to turn on PD1 (115) and show 0/+5V at MTR0_PUL pin)
                        //arg1: turn pin on or off   == 1, it will turn on/off PD7, MT3_PUL
  //,eCmd_Laser         //arg0: int turn_on    ;    arg1: power;                                      turns on/off laser. for turn_off power is ignored    
    ,eCmd_Last    
    ,eCmd_Force32BitSize = 0x7fffffff
};

enum {c_max_args = 4};
enum {c_num_motors = 3};        //physical 0 & 1 are paired x-direction motors; n3 physical is 2 logical so on

struct cmd_t
{
    enum ECmd   cmd;
    int32_t     cmd_id;                 //will be returned in command response
    int32_t     args[c_max_args];
};

//enum { c_buf_cmd_size = c_buf_byte_size/sizeof(struct cmd_t) };
// struct cmd_ring_buffer_t
// {
//     cmd_t  buf[c_buf_cmd_size];
//     int    read_ptr;
//     int    write_ptr;
// };

enum ECmdStatus
{
     eCmdStatus_Failed
    ,eCmdStatus_Ok    
    ,eCmdStatus_Canceled                        //the command was canceled by another command...
    ,eCmdStatus_Pending                         //obviously will not be returned in cmd_result...
    ,eCmdStatus_Force32BitSize = 0x7fffffff
};

struct cmd_result_t
{
    int              cmd_id;     //it's response on a command with such id
    enum ECmdStatus  status;
};

enum EPrinterEventType
{
     ePrinterEventType_CmdResult
    ,ePrinterEventType_Err          //an error (usually reset is needed..)
    ,ePrinterEventType_Msg          //just a message, printer likes to say you something
    ,ePrinterEventType_Force32BitSize = 0x7fffffff
};

//i.e. event which will go to host (PC, workstation, big computer)
struct cmd_printer_event_t
{
    enum EPrinterEventType event_type;
    struct cmd_result_t    cmd_result;      //in case event_type is ePrinterEventType_CmdResult, else is empty
    int                    msg_num_bytes;   //length of the message below
    char                   msg[];           //either message or error depending on event_type (or just nothing...)
};

//sends message over USB to host, if msg_len == -1 msg will be assumed to be zero-terminated string
void cmd_send_message_to_host(enum EPrinterEventType type, char const * msg, int const msg_len);       //type can be ePrinterEventType_Err or ePrinterEventType_Msg

//int cmd_printer_event_t

struct cmd_state_t;

typedef enum ECmdStatus (*cmd_process_fn_t)(struct cmd_state_t * state);
typedef void (*cmd_cleanup_fn_t)(struct cmd_state_t * state);

struct cmd_state_t
{
    struct cmd_t       cmd;        //a command which is executing now
    cmd_process_fn_t   proc_fn;
  //cmd_cleanup_fn_t   cleanup_fn;
};

// struct cmd_def_list_t
// {
//     struct cmd_def_list_t * next;
//     struct cmd_t cmd;    
// };

