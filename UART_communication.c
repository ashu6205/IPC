#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/select.h>
#include <errno.h>

#define BUFFER_SIZE 1024
#define SERIAL_DEVICE "/dev/ttyUSB0"

int setup_uart(const char *device) {
    int fd = open(device, O_RDWR | O_NOCTTY);
    if (fd < 0) {
        perror("open");
        return -1;
    }

    struct termios tty;
    tcgetattr(fd, &tty);

    cfsetispeed(&tty, B9600);
    cfsetospeed(&tty, B9600);

    tty.c_cflag &= ~PARENB;     // No parity
    tty.c_cflag &= ~CSTOPB;     // 1 stop bit
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;         // 8 data bits
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~CRTSCTS;    // No flow control

    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_oflag &= ~OPOST;

    tty.c_cc[VMIN]  = 1;
    tty.c_cc[VTIME] = 0;

    tcsetattr(fd, TCSANOW, &tty);
    tcflush(fd, TCIOFLUSH);

    return fd;
}

int main() {
    int uart_fd = setup_uart(SERIAL_DEVICE);
    if (uart_fd < 0) return 1;

    printf("=== UART Chat Started ===\n");
    printf("Device: %s | Baud: 9600\n", SERIAL_DEVICE);
    printf("Type to send, receive appears automatically\n\n");

    fd_set readfds;
    char buffer[BUFFER_SIZE];

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        FD_SET(uart_fd, &readfds);

        int maxfd = (uart_fd > STDIN_FILENO) ? uart_fd : STDIN_FILENO;

        if (select(maxfd + 1, &readfds, NULL, NULL, NULL) < 0) {
            if (errno == EINTR) continue;
            perror("select");
            break;
        }

        // Keyboard → UART
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            int n = read(STDIN_FILENO, buffer, BUFFER_SIZE);
            if (n > 0) {
                write(uart_fd, buffer, n);
            }
        }

        // UART → Screen
        if (FD_ISSET(uart_fd, &readfds)) {
            int n = read(uart_fd, buffer, BUFFER_SIZE - 1);
            if (n > 0) {
                buffer[n] = '\0';
                printf("[Peer]: %s", buffer);
                fflush(stdout);
            }
        }
    }

    close(uart_fd);
    return 0;
}
