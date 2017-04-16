#include "mcu-soft/src/command.h"
#include "uart.hpp"
#include <stdio.h>
#include <stdexcept>
#include <stdio.h>
#include <atomic>
#include <thread>

#include <readline/readline.h>
#include <readline/history.h>

//hint in case of problems with readline, use http://thrysoee.dk/editline/
static int s_cmd_id = 0;

static bool exec_command(std::string const & line)
{
        char ccmd[3] = {0};
        cmd_t cmd;
        
        int const numArgs = sscanf(line.c_str(), "%2s %d %d %d %d", ccmd, cmd.args, cmd.args+1, cmd.args+2, cmd.args+3);
        if (numArgs < 1)
        {
                fprintf(stderr, "wrong command: %s\n", line.c_str());
                return false;
        }
        
        std::string const scmd = ccmd;
        if (scmd == "mm") cmd.cmd = eCmd_MotorMove;        
        else if (scmd == "ms") cmd.cmd = eCmd_MotorStop;        
        else if (scmd == "mt") cmd.cmd = eCmd_MotorTestPin;
        else if (scmd == "l") cmd.cmd = eCmd_Led;        
        else if (scmd == "r") cmd.cmd = eCmd_MCUReset;        
        else {
            fprintf(stderr, "unrecognized command: %s\n", scmd.c_str());
            return false;
        }
             
        cmd.cmd_id = ++s_cmd_id;             
        uart_write(&cmd, sizeof(cmd));
        return true;
}

static std::atomic<bool> sCanStart{false};
static std::atomic<bool> sShouldStop{false};

//with sShouldStop support
static int uart_sync_read(void * buf, int expected)
{
    int already_read = 0;
    char * cbuf = (char*)buf;
    while (!sShouldStop)
    {
        int wasRead = uart_read(cbuf + already_read, expected);
        if (already_read + wasRead >= expected)
            break;
        else
        {
            already_read += wasRead;
        }            
    }    
    if (sShouldStop)
        throw std::runtime_error("uart_sync_read - exit requested (sShouldStop is set)");
    return expected;
}

static void response_thread()
{
    while (!sCanStart && !sShouldStop);
    
    try
    {
        while (!sShouldStop)
        {
            struct cmd_printer_event_t pe;
            char msg_buf[2048]; //big enough...
            msg_buf[0] = 0; //for insurance..
            
            
            //cmd_result_t cr;
            int wasRead = uart_sync_read(&pe, sizeof(pe));
            if (wasRead != sizeof(pe))
                fprintf(stderr, "partial event arrived, shit will happen..\n");
            
            if (pe.msg_num_bytes)
            {
                if(pe.msg_num_bytes >= 1900)                    
                    fprintf(stderr, "too long message pe.msg_num_bytes: %d, shit will happen..\n", pe.msg_num_bytes);
                
                wasRead = uart_sync_read(msg_buf, pe.msg_num_bytes);
                if (wasRead != pe.msg_num_bytes)
                    fprintf(stderr, "partial message in event arrived, shit will happen..\n");
                msg_buf[pe.msg_num_bytes] = 0; //force string to be null terminated
            }
            
            if (pe.event_type == ePrinterEventType_CmdResult)            
                printf("\ncommand_result status: %d cmd_id: %d\n", pe.cmd_result.status, pe.cmd_result.cmd_id);                                
            else
            {
                if (pe.event_type == ePrinterEventType_Err)
                    fprintf(stderr, "printer error: %s", msg_buf);
                else if (pe.event_type == ePrinterEventType_Msg)
                    printf("printer message: %s", msg_buf);
                else
                    fprintf(stderr, "host parser error: unknown EPrinterEventType: %d", pe.event_type);

            }            
        }
    } catch (std::exception const & ex)
    {
        fprintf(stderr, "response_thread, shit happend %s; exiting\n", ex.what());
        std::terminate();
    }
}

int main(int const argc, char const * argv[])
{
    char const * dev_name;
    if (argc >= 2)
        dev_name = argv[1];
    else {
        fprintf(stderr, "tty device is not specified, using /dev/ttyACM0\n");
        dev_name = "/dev/ttyACM0";
    }
    
    std::thread sResponseThread(response_thread);
    
    try
    {
        uart_init(dev_name);                    
        sCanStart = true;
        
        std::string line;        
        while (1)
        {
            {
                char * cline = readline("3dprinter>");
                if (!cline)
                {
                    fprintf(stderr, "EOF gotten, exiting...\n");
                    break;
                }
                line = cline;
                free(cline); cline = nullptr;
            }
        
            bool ret = exec_command(line);
            if (ret)
                add_history(line.c_str());        
        }                                
        
    } catch(std::exception const & ex)
    {
        sShouldStop = true;
        sResponseThread.join();
        uart_free();        
        exit(1);
    }    
    
    sShouldStop = true;
    sResponseThread.join();
    uart_free();
}
