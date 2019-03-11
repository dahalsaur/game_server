//Saurav Dahal
//s.dahal@jacobs-university.de

/*
References:
http://man7.org/linux/man-pages/man2/ (socket documentation)
https://www.monkey.org/~provos/libevent/doxygen-2.0.1/ (libevent documentation)
https://www.ibm.com/developerworks/aix/library/au-libev/index.html (libevent implementation)
*/

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <errno.h>
#include <time.h>
#include <stdbool.h>
#include <fcntl.h>
#include <event2/event.h>
#include <event2/event.h>
#include <event2/event_struct.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>

#define BUF_SIZE 1024
#define LISTEN_BACKLOG 50
#define PORT 1234

static struct event_base *base;

        //struct that stores client's file descriptor and
        //      buffer event(provides input and output buffers that get filled and drained automatically.)
struct client {
    int fd;
    struct bufferevent *buf_ev;
};


        //set a socket to non-blocking mode
int setnonblock(int fd) {
	int flags;
	flags = fcntl(fd, F_GETFL);
	if (flags < 0) {
        return flags;
    }
	flags |= O_NONBLOCK;
	if (fcntl(fd, F_SETFL, flags) < 0) {
        return -1;
    }
	return 0;
}

char* read_phrase_callback(FILE *fp) {
    char *phrase = (char*) malloc(sizeof(char)*32);
    if (fgets(phrase, 32, fp) == NULL) {
        perror("fgets error");
        exit(EXIT_FAILURE);
    }
    return phrase;
}

void accept_callback(int sfd, short ev, void *arg) {
    int client_fd;
    socklen_t client_addr_len;
    struct sockaddr_storage client_addr;
    client_addr_len = sizeof(struct sockaddr_storage);
    struct client* client;

            //accept extracts the connection request and creates a new connected socket, client_fd
    client_fd = accept(sfd, (struct sockaddr *) &client_addr, &client_addr_len);
    if (client_fd == -1) {
        fprintf(stderr, "Could not accept\n");
        exit(EXIT_FAILURE);
    }

        //setting a client socket to non-blocking mode
    if (setnonblock(client_fd) < 0) {
    perror("setnonblock error");
    exit(EXIT_FAILURE);
    }

        //dynamically allocating client
    client = calloc(1, sizeof(*client));
    if (client == NULL) {
        perror("client calloc error");
    }
    // client->fd = client_fd;
    // client->buf_ev = bufferevent_new(base, client_fd, 0);


    FILE *fp;
            //popen opens a process by creating a pipe, forking, and invoking the shell to run fortune
    fp = popen("fortune -n 32 -s", "r");
    if (fp == NULL) {
        perror("popen");
        exit(EXIT_FAILURE);
    }

    char *phrase;
        //callback function to get phrase from the pipe
    phrase = read_phrase_callback(fp);

    printf("%c\n", phrase[0]);

}

int main(int argc, char *argv[]) {
    int socket_fd;
    struct sockaddr_in my_addr;
    struct event *event_accept;
    int reuse = 1;

    base = event_base_new();   //libevent intialization
    if (base == NULL) {
        perror("event_base_new error");
        exit(EXIT_FAILURE);
    }

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);    //opening a socket
    if (socket_fd == -1) {
        perror("Error opening socket");
        exit(EXIT_FAILURE);
    }
             //clearing sockaddr before using it
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    my_addr.sin_port = htons(PORT); //Assigning socket to port 1234
            //bind assigns the address to the socket, socket_fd
    if (bind(socket_fd, (struct sockaddr *) &my_addr, sizeof(struct sockaddr_in)) == -1) {  //Bind socket
        perror("bind error");
        exit(EXIT_FAILURE);
    }
            //listen marks the socket as passive socket, waiting to accept connection requests
    if (listen(socket_fd, LISTEN_BACKLOG) == -1) {  //
        perror("listen error");
        exit(EXIT_FAILURE);
    }
            //setting options on sockets
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1) {
        perror("setsockopt error");
        exit(EXIT_FAILURE);
    }
                //setting a socket to non-blocking mode
    if (setnonblock(socket_fd) < 0) {
        perror("setnonblock error");
        exit(EXIT_FAILURE);
    }
            //creating a read event to be notified when a client connects
    event_accept = event_new(base, socket_fd, EV_READ|EV_PERSIST, accept_callback, NULL);

            //add an event to the set of monitered events
    event_add(event_accept, NULL);

            ////loop to process events
    if (event_base_loop(base, 0) == -1) {
        perror("event_base_loop error");
        event_base_free(base);
        exit(EXIT_FAILURE);
    }

    event_base_free(base);

    close(socket_fd);

    return(EXIT_SUCCESS);
}
