#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <stdlib.h>

#define ERROR(x) \
    do { \
        perror(x); \
        ret = -1; \
        goto done; \
    } while (0)

int init_tty(int fd);
int main_loop(int fd);
int send_cmd(int fd, char *cmd, size_t len);


int
main(int argc, char **argv) {
    int fd;
    char *device;
    int ret;

    /*
     * Read the device path from input,
     * or default to /dev/ttyACM0
     */
    if (argc == 2) {
        device = argv[1];
    } else {
        device = "/dev/ttyACM0";
    }

    /*
     * Need the following flags to open:
     * O_RDWR: to read from/write to the devices
     * O_NOCTTY: Do not become the process's controlling terminal
     * O_NDELAY: Open the resource in nonblocking mode
     */
    fd = open(device, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd == -1) {
        perror("Error opening serial");
        return -1;
    }

    /* Configure settings on the serial port */
    if (init_tty(fd) == -1) {
        ERROR("init");
    }

    ret = main_loop(fd);

done:
    close(fd);
    return ret;
}

int
main_loop(int fd) {
    /*
     * Main program loop:
     *  - Prompt user for input [Possible protocol, feel free to modify]
     *  - Send corresponding signal over serial to arduino
     *  - pause: send a 1
     *  - resume: send a 2
     *  - blink #: send a 3 and then the length of # 
     *      and then the # as a string
     *  - exit: free input buffer
     */

    char* buffer;
    size_t buffer_size = 10 * sizeof(char);
    buffer = (char *) malloc(buffer_size); 
    while (1) {
        printf("->");
        //read in from command and store in buffer
        if (getline(&buffer, &buffer_size, stdin) < 0) { 
            printf("ERROR: Could not read line\n");
            return 0;
        }
        printf("USER INPUT: %s\n", buffer);
	char delim[] = " ";
	char *ptr = strtok(buffer, delim);
        // NOTE, below, I'm sending numeric values "\1", "\2", "\3" 
        // rather than character values "1", "2", "3"
        // so that we can compare the received byte directly (in a switch) 
        // with the NUMBERS 1, 2, and 3.

        //compare input buffer string to "pause"
        if (strcmp(ptr,"pause\n") == 0)  {  
            printf("Pausing...\n");
            // consume serial port's buffer (clean)
            tcflush(fd,TCIOFLUSH);
	    char send = '1';
            //sending a \1 using send_cmd to ask arduino to signal pause
	    send_cmd(fd, &send , 1);            
        }
        //compare input buffer string to "resume"
        else if (strcmp(ptr,"resume\n") == 0)  {
            printf("Resuming...\n");
	    tcflush(fd,TCIOFLUSH);
            //sending a \2 using send_cmd to ask arduino to signal resume
	    char send = '2';
	    send_cmd(fd,&send, 1);
        }
        //compare input buffer string to "blink "
        else if (strcmp(ptr,"blink") == 0) { 
            //check blink command format is correct
	    //send 3 to signal arduino to blink
	    //send another after # to tell arduino num per sec
	    printf("blinking...Perfomring calculations please wait a couple seconds\n");
	    tcflush(fd,TCIOFLUSH);
	    ptr = strtok(NULL,delim);
	    char send = '3';
	    //send the length of the number.
	    send_cmd(fd,&send,1);
	    int len = strlen(ptr) + 1;
	    char length = (char)len + 46;
	    send_cmd(fd,&length,1);
	    //send each char of number keyed in by user
	    int i = 0;
	    char * c = ptr;
	    for (i = 0; i < len - 2; i++){
	     send_cmd(fd, &c[i], 1); 
	    }
	    printf("Okay ready to go!");
        }
        //compare input buffer string to "exit"
        else if (strcmp(buffer, "exit\n") == 0) { 
            //just frees input buffer
	    free(buffer);
	    printf("Exiting...\n");
            return 0;
        }
	else{
	    printf("Invalid Input\n");
	    printf("%s", buffer);
	}
    }

}

int
send_cmd(int fd, char *cmd, size_t len) {
    int count = 0;
    char buf[32];
    
    // TODO: write command to serial
    count = write(fd, cmd, len);
    if (count == -1) {
	perror("write");
	close(fd);
	return -1;
    }
    else if (count == 0) {
        fprintf(stderr, "No data writen\n");
	close(fd);
	return -1;
    }
    // Give the data time to transmit
    // Serial is slow...
    sleep(1);
    count = read(fd,&buf,32);
    if (count == -1) {
	perror("read");
	close(fd);
	return -1;
    }
    else if (count == 0) {
        fprintf(stderr, "No data returned\n");
    }
    if(*cmd == 50  && count !=0){
        printf("ARDUINO RESPONSE: %s\n", buf); 
    }
    // Read response from serial
    // Note: tcflush(fd,TCIOFLUSH); 
    tcflush(fd,TCIOFLUSH);
    return count;
}

int
init_tty(int fd) {
    struct termios tty;
    /*
     * Configure the serial port.
     * First, get a reference to options for the tty
     * Then, set the baud rate to 9600 (same as on Arduino)
     */
    memset(&tty, 0, sizeof(tty));
    if (tcgetattr(fd, &tty) == -1) {
        perror("tcgetattr");
        return -1;
    }

    if (cfsetospeed(&tty, (speed_t)B9600) == -1) {
        perror("ctsetospeed");
        return -1;
    }
    if (cfsetispeed(&tty, (speed_t)B9600) == -1) {
        perror("ctsetispeed");
        return -1;
    }

    // 8 bits, no parity, no stop bits
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;

    // No flow control
    tty.c_cflag &= ~CRTSCTS;

    // Set local mode and enable the receiver
    tty.c_cflag |= (CLOCAL | CREAD);

    // Disable software flow control
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);

    // Make raw
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_oflag &= ~OPOST;

    // Infinite timeout and return from read() with >1 byte available
    tty.c_cc[VMIN]  = 0;
    tty.c_cc[VTIME] = 0;

    // Update options for the port
    if (tcsetattr(fd, TCSANOW, &tty) == -1) {
        perror("tcsetattr");
        return -1;
    }

    return 0;
}
