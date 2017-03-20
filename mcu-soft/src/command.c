#include "command.h"
#include "motor.h"
#include "main.h"
#include <memory.h>

void usbd_cdc_interface_transmit_data(void * const data, int len);       //obviously see usbd_cdc_interface.c

static struct cmd_ring_buffer_t sCmdBuf = {.read_ptr = 0, .write_ptr = 0};
static struct cmd_state_t * sMotorStates[c_num_motors];
static struct cmd_def_list_t * sCmdDefList = 0;

static void send_cmd_result(struct cmd_t const * const cmd, enum ECmdStatus status)
{
    struct cmd_result_t cr = {.cmd_id = cmd->cmd_id, .status = status};
    usbd_cdc_interface_transmit_data(&cr, sizeof(cr));
}

//******************* command handlers *********************************************
static void cmdh_led(struct cmd_t * const cmd)
{
    if (cmd->args[0] == 0) {
        if (cmd->args[1])
             HAL_GPIO_WritePin(GPIOG, GPIO_PIN_0, GPIO_PIN_SET);        
        else
            HAL_GPIO_WritePin(GPIOG, GPIO_PIN_0, GPIO_PIN_RESET);        
    }    
    else{ //led 1
        if (cmd->args[1]) 
            HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, GPIO_PIN_SET);        
        else
            HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, GPIO_PIN_RESET);        
    }
    send_cmd_result(cmd, eCmdStatus_Ok);
}

static void cmdh_motor(struct cmd_t * const cmd)
{
    uint32_t const motorIdx = cmd->args[0];
    if (motorIdx < c_num_motors)
    {
        struct cmd_state_t * const newState = motor_cmd_state_create(cmd);
        if (sMotorStates[motorIdx]) { //there is some command already... just replace it
         free(sMotorStates[motorIdx]);
        }
        sMotorStates[motorIdx] = newState;                        
        send_cmd_result(cmd, eCmdStatus_Ok);
    }
    else
        send_cmd_result(cmd, eCmdStatus_Failed);
}

//******************* command core *********************************************

static void process_command(struct cmd_t * const cmd)
{
        switch(cmd->cmd)
        {
            case eCmd_MCUReset: NVIC_SystemReset(); break;      //actually we won't return from here...
            case eCmd_Led: cmdh_led(cmd); break;            
            case eCmd_MotorMove:
          //case eCmd_MotorGoHome: 
                cmdh_motor(cmd); 
                break;            
            default:
                //shit happened...
                main_fail_with_error("Unknown command: %d cmd_id: %d arg0: %d\n", cmd->cmd, cmd->cmd_id, cmd->args[0]);
            break;
        }
}

static int take_command(struct cmd_t * ocmd, struct cmd_ring_buffer_t * const rb)
{
    uint32_t const end = rb->read_ptr + sizeof(struct cmd_t)/sizeof(int32_t);
    uint32_t const rend = end % c_buf_int_size;
    if (rend > rb->write_ptr)
        return 0;       //just not enough data...
    
    if (end > c_buf_int_size) {
        //take two parts
        uint32_t const part1_length = c_buf_int_size - rb->read_ptr;
        uint32_t const part2_length = sizeof(struct cmd_t)/sizeof(int32_t) - part1_length;
        memcpy(ocmd, &rb->buf[rb->read_ptr], part1_length * sizeof(int32_t));
        memcpy((int32_t*)ocmd + part1_length, rb->buf, part2_length * sizeof(int32_t));
    }
    else { //only part, just copy...
        *ocmd = *(struct cmd_t *)&(rb->buf[rb->read_ptr]);
    }        
    rb->read_ptr = rend;        
    return 1;
}

void cmd_data_arrived(uint8_t const * const data, int const length)
{
        if (length%sizeof(uint32_t) != 0)
        main_fail_with_error("CDC_Itf_Receive: length is not aligned by 4");
    
    int intLen = length / sizeof(uint32_t);
    if (intLen <= c_buf_int_size)
    {
        if (sCmdBuf.write_ptr + length <= c_buf_int_size)
        {
            memcpy(sCmdBuf.buf + sCmdBuf.write_ptr, data, length);
            sCmdBuf.write_ptr += intLen;
        }
        else
        {   //divide to two ports
            int const p0 = c_buf_int_size - sCmdBuf.write_ptr;
            int const p2 = intLen - p0;
            memcpy(sCmdBuf.buf + sCmdBuf.write_ptr, data, p0 * sizeof(uint32_t));
            memcpy(sCmdBuf.buf, &data[p0 * sizeof(uint32_t)], p2 * sizeof(uint32_t));
            sCmdBuf.write_ptr = p2;            
        }        
    }
    else
        main_fail_with_error("CDC_Itf_Receive: too long packet...");        //you can try to copy sizeof(cmd_t) from *Len and call command.h command_process_ring_buffer
    
}

static void cmd_process_ring_buffer()
{    
    struct cmd_t cmd;
    while (take_command(&cmd, &sCmdBuf))
        process_command(&cmd);            
}


void cmd_iterate_command_loop()
{
    cmd_process_ring_buffer();
    
    for (int i = 0; i < c_num_motors; ++i)
    {
        struct cmd_state_t * const s = sMotorStates[i];
        if (s)
            s->proc_fn(s);
    }
}

void cmd_enter_command_loop()
{
    while(1)
        cmd_iterate_command_loop();
}

