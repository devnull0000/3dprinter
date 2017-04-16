#include "command.h"
#include "motor.h"
#include "main.h"
#include <memory.h>

void usbd_cdc_interface_transmit_data(void * const data, int len);       //obviously see usbd_cdc_interface.c

static struct cmd_ring_buffer_t sCmdBuf = {.r_ptr = 0, .w_ptr = 0};
static struct cmd_state_t * sMotorStates[c_num_motors];
//static struct cmd_def_list_t * sCmdDefList = 0;

static void send_cmd_result(struct cmd_t const * const cmd, enum ECmdStatus status)
{
    //struct cmd_result_t cr = {.cmd_id = cmd->cmd_id, .status = status};
    struct cmd_printer_event_t pe = {.event_type = ePrinterEventType_CmdResult, .cmd_result={.cmd_id = cmd->cmd_id, .status = status}, .msg_num_bytes=0};
    usbd_cdc_interface_transmit_data(&pe, sizeof(pe));
}

void cmd_send_message_to_host(enum EPrinterEventType type, char const * msg, int const msg_len)        //the function is used from newlib-supp.c to transfer data over USB!
{
    int const msg_real_len = (msg_len == -1 ? (strlen(msg) + 1) : msg_len);     //+1 copy zero as well
    int const struct_size = sizeof(struct cmd_printer_event_t) + msg_real_len;
    char buf[struct_size];
    struct cmd_printer_event_t * pe = (struct cmd_printer_event_t *)buf;
    pe->event_type = type; 
    pe->cmd_result.cmd_id = -1; pe->cmd_result.status = eCmdStatus_Failed;
    pe->msg_num_bytes=msg_real_len;
    memcpy(pe->msg, msg, msg_real_len);
    usbd_cdc_interface_transmit_data(pe, struct_size);
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

static void cmdh_test_motor_pin(struct cmd_t * const cmd)
{        
    //YS: we have hex inverters there so if we want to turn motor +5V pin on, we should reset (send logical 0 there)
    //and vice versa
    if (cmd->args[0] == 0)
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_1, cmd->args[1] ? GPIO_PIN_RESET : GPIO_PIN_SET);         
    else
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_10, cmd->args[1] ? GPIO_PIN_RESET : GPIO_PIN_SET);         
    send_cmd_result(cmd, eCmdStatus_Ok);
}

static void cmdh_motor_stop(struct cmd_t * const cmd)
{
    uint32_t const motorIdx = cmd->args[0];
    if (motorIdx < c_num_motors)
    {
        if (sMotorStates[motorIdx])
        {
            send_cmd_result(&sMotorStates[motorIdx]->cmd, eCmdStatus_Canceled);
            free(sMotorStates[motorIdx]);
            sMotorStates[motorIdx] = 0;
            send_cmd_result(cmd, eCmdStatus_Ok);
        }
    }
    else
        send_cmd_result(cmd, eCmdStatus_Failed);
}

static void cmdh_motor(struct cmd_t * const cmd)
{
    uint32_t const motorIdx = cmd->args[0];
    if (motorIdx < c_num_motors)
    {
        struct cmd_state_t * const newState = motor_cmd_state_create(cmd);
        if (sMotorStates[motorIdx]) { //there is some command already... just replace it
            send_cmd_result(&sMotorStates[motorIdx]->cmd, eCmdStatus_Canceled);
            free(sMotorStates[motorIdx]);
        }
        sMotorStates[motorIdx] = newState;                                
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
            case eCmd_MotorTestPin: cmdh_test_motor_pin(cmd); break;
            case eCmd_MotorStop: cmdh_motor_stop(cmd); break;
            case eCmd_MotorMove:
            case eCmd_MotorGoHome: 
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
    uint32_t const end = rb->r_ptr + sizeof(struct cmd_t);    
    if ((int32_t)(end - rb->w_ptr) > 0)
        return 0;       //just not enough data...
 
    uint32_t const cr_ptr = rb->r_ptr % cmd_rb_size;
    uint32_t const cr_end = cr_ptr + sizeof(struct cmd_t);
 
    if (cr_end > cmd_rb_size) {
        //take two parts
        uint32_t const part1_length = cmd_rb_size - cr_ptr;
        uint32_t const part2_length = sizeof(struct cmd_t) - part1_length;
        memcpy(ocmd, &rb->buf[cr_ptr], part1_length);
        memcpy((char*)ocmd + part1_length, rb->buf, part2_length);
    }
    else { //only part, just copy...        
        memcpy(ocmd, rb->buf + cr_ptr, sizeof(struct cmd_t));
    }        
    rb->r_ptr = end;        
    return 1;
}

void cmd_data_arrived(uint8_t const * const data, int const length)
{
    if (length%sizeof(uint32_t) != 0)
        main_fail_with_error("cmd_data_arrived: length is not aligned by 4, i.e. most probably there is no valid command...");
        
    if (length <= cmd_rb_size) {
        uint32_t const end = sCmdBuf.w_ptr + length;
        uint32_t const cw_ptr = sCmdBuf.w_ptr % cmd_rb_size;
        uint32_t const cw_end = cw_ptr + length;
        
        if (end <= cmd_rb_size) {
            memcpy(sCmdBuf.buf + cw_ptr, data, length);
        }
        else {   //divide to two parts
            int const p0 = cmd_rb_size - cw_ptr;
            int const p2 = length - p0;
            memcpy(sCmdBuf.buf + cw_ptr, data, p0);
            memcpy(sCmdBuf.buf, data + p0, p2);            
        }        
        sCmdBuf.w_ptr = end;
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
        {
            enum ECmdStatus status = s->proc_fn(s);
            if (status != eCmdStatus_Pending)
            {   //i.e. the command done (either okay or failed, we don't care just tell about this...)
                send_cmd_result(&s->cmd, status);
                free(s);
                sMotorStates[i] = 0;                
            }            
        }
    }
}

void cmd_enter_command_loop()
{
    while(1)
        cmd_iterate_command_loop();                      
}

