#include <errno.h>
#include <fcntl.h> 
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <stdexcept>

static int g_tty_fd = -1;

int uart_fd()
{
    return g_tty_fd;
}

static void
set_interface_attribs (int fd, int speed, int parity)
{
        struct termios tty;
        memset (&tty, 0, sizeof tty);
        if (tcgetattr (fd, &tty) != 0)
        {
                fprintf(stderr, "error %d from tcgetattr\n", errno);
                throw std::runtime_error("set_interface_attribs");
        }

        cfsetospeed (&tty, speed);
        cfsetispeed (&tty, speed);

        tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
        // disable IGNBRK for mismatched speed tests; otherwise receive break
        // as \000 chars
        tty.c_iflag &= ~IGNBRK;         // disable break processing
        tty.c_lflag = 0;                // no signaling chars, no echo,
                                        // no canonical processing
        tty.c_oflag = 0;                // no remapping, no delays
        tty.c_cc[VMIN]  = 0;            // read doesn't block
        tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

        tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

        tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
                                        // enable reading
        tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
        tty.c_cflag |= parity;
        tty.c_cflag &= ~CSTOPB;
        tty.c_cflag &= ~CRTSCTS;

        if (tcsetattr (fd, TCSANOW, &tty) != 0)
        {
                fprintf(stderr, "error %d from tcsetattr\n", errno);
                throw std::runtime_error("set_interface_attribs");
        }        
}

void
set_blocking (int fd, int should_block)
{
        struct termios tty;
        memset (&tty, 0, sizeof tty);
        if (tcgetattr (fd, &tty) != 0)
        {
                fprintf(stderr, "error %d from tggetattr\n", errno);
                throw std::runtime_error("set_blocking");
        }

        tty.c_cc[VMIN]  = should_block ? 1 : 0;
        tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

        if (tcsetattr (fd, TCSANOW, &tty) != 0)
        {
                fprintf(stderr, "error %d setting term attributes\n", errno);
                throw std::runtime_error("set_blocking");
        }
}

void uart_init(char const * dev_name)
{
    int fd = open (dev_name, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0)
    {
        fprintf(stderr, "error %d opening %s: %s\n", errno, dev_name, strerror (errno));
        throw std::runtime_error("uart_init");
    }
    
    set_interface_attribs (fd, B115200, 0);  // set speed to 115,200 bps, 8n1 (no parity)
    set_blocking (fd, 0);                // set no blocking
    
    g_tty_fd = fd;
}

void uart_free()
{
    if (g_tty_fd != -1)
    {
        close(g_tty_fd);
        g_tty_fd = -1;
    }
}

void uart_write(void const * const data, int const size)
{
    int err = 0;
    
    do
    {
        int code = write (g_tty_fd, data, size);
        if (code == -1)
        {
            err = errno;
            if (err == EAGAIN)  //just handle non-blocking case, sleep on fd via select...
            {
                fd_set wr, exc;
                FD_ZERO(&wr); FD_ZERO(&exc);
                FD_SET(g_tty_fd, &wr);
                FD_SET(g_tty_fd, &exc);
                code = select(g_tty_fd+1, 0, &wr, 0, 0);
                if (code == -1)
                {
                    fprintf(stderr, "uart_write select function failed, errno: %d (%s)\n", errno,  strerror (errno));
                    throw std::runtime_error("uart_write");
                }
            }
            else    //shit happened, throw...
            {
                fprintf(stderr, "uart_write write function failed, errno: %d (%s)\n", errno,  strerror (errno));
                throw std::runtime_error("uart_write");
            }
        }
    } while (err == EAGAIN);
    
}

int  uart_read(void * const buf, int const buf_size)
{
    ssize_t const ret = read(g_tty_fd, buf, buf_size);
    if (ret == -1 && errno == EAGAIN)
        return 0;
    else {
        if (ret == -1) {
            fprintf(stderr, "uart_read, error: %d (%s)\n",  errno,  strerror (errno));
            return 0;
        }
    }
    return ret;
}
