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

static void response_thread()
{
    while (!sCanStart && !sShouldStop);
    
    try
    {
        while (!sShouldStop)
        {
            cmd_result_t cr;
            int remainToRead = sizeof cr;
            
            int wasRead = uart_read(&cr, remainToRead);
            remainToRead -= wasRead;
            
            if (!remainToRead)
            {
                printf("command_result status: %d cmd_id: %d\n", cr.status, cr.cmd_id);
                
                remainToRead = sizeof cr;
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
