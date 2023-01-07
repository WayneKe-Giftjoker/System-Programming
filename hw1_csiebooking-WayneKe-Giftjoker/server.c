//ref: https://github.com/chriscyh2000/NTU-Courses/blob/master/%5BCSIE2210%5DSystems%20Programming/Programming%20hw1/server.c#L199
//Discussed with the author of the above github repository B08902149 徐晨祐 & B08502041 李芸芳 & B09902129 黃柏鈞
#include <unistd.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdbool.h>
#include <ctype.h>

#define ERR_EXIT(a) do { perror(a); exit(1); } while(0)

#define OBJ_NUM 3

#define FOOD_INDEX 0
#define CONCERT_INDEX 1
#define ELECS_INDEX 2
#define RECORD_PATH "./bookingRecord"

#define STU_NUM 20

static char* obj_names[OBJ_NUM] = {"Food", "Concert", "Electronics"};
static int obj_idx[OBJ_NUM] = {FOOD_INDEX, CONCERT_INDEX, ELECS_INDEX};

typedef struct {
    char hostname[512];  // server's hostname
    unsigned short port;  // port to listen
    int listen_fd;  // fd to wait for a new connection
} server;

typedef struct {
    char host[512];  // client's host
    int conn_fd;  // fd to talk with client
    char buf[512];  // data sent by/to client
    size_t buf_len;  // bytes used by buf
    int id;
    int wait_for_write;  // used by handle_read to know if the header is read or not.
} request;

server svr;  // server
request* requestP = NULL;  // point to a list of requests
int maxfd;  // size of open file descriptor table, size of request list

const char* accept_read_header = "ACCEPT_FROM_READ";
const char* accept_write_header = "ACCEPT_FROM_WRITE";
const unsigned char IAC_IP[3] = "\xff\xf4";

static void init_server(unsigned short port);
// initailize a server, exit for error

static void init_request(request* reqP);
// initailize a request instance

static void free_request(request* reqP);
// free resources used by a request instance

typedef struct {
    int id;          // 902001-902020
    int bookingState[OBJ_NUM]; // For example, [0, 1, 2] means 0 FOOD, 1 CONCERT and 2 ELECTRONICS have been booked.
}record;

int handle_read(request* reqP) {
    /*  Return value:
     *      1: read successfully
     *      0: read EOF (client down)
     *     -1: read failed
     */
    int r;
    char buf[512];

    // Read in request from client
    r = read(reqP->conn_fd, buf, sizeof(buf));
    if (r < 0) return -1;
    if (r == 0) return 0;
    char* p1 = strstr(buf, "\015\012");
    int newline_len = 2;
    if (p1 == NULL) {
       p1 = strstr(buf, "\012");
        if (p1 == NULL) {
            if (!strncmp(buf, IAC_IP, 2)) {
                // Client presses ctrl+C, regard as disconnection
                fprintf(stderr, "Client presses ctrl+C....\n");
                return 0;
            }
            ERR_EXIT("this really should not happen...");
        }
    }
    size_t len = p1 - buf + 1;
    memmove(reqP->buf, buf, len);
    reqP->buf[len - 1] = '\0';
    reqP->buf_len = len-1;
    return 1;
}

int getNum(char* s, bool* valid){
    int len = strlen(s);
    bool neg = false;
    int i = 0;
    if(s[0] == '-' && len >= 2){
        neg = true;
        ++i;
    }
    int res = 0;
    for(; i < len; ++i){
        if(isdigit(s[i])){
            res = res*10 + s[i]-'0';
        }
        else{
            *valid = false;
            return 0;
        }
    }
    return (neg)? -res : res;
}

int main(int argc, char** argv) {

    // Parse args.
    if (argc != 2) {
        fprintf(stderr, "usage: %s [port]\n", argv[0]);
        exit(1);
    }

    struct sockaddr_in cliaddr;  // used by accept()
    int clilen;

    int conn_fd;  // fd for a new connection with client
    int file_fd = open(RECORD_PATH, O_RDWR);  // fd for file that we open for reading
    char buf[512];
    int buf_len;

    

    // Initialize server
    init_server((unsigned short) atoi(argv[1]));

    // Loop for handling connections
    fprintf(stderr, "\nstarting on %.80s, port %d, fd %d, maxconn %d...\n", svr.hostname, svr.port, svr.listen_fd, maxfd);

    struct pollfd fdarr[maxfd];
    bool exited[maxfd];
    int lockedFile[STU_NUM] = {0};// if lockedFIle[i] == 1 -> means RDLK || lockedFile[i] == 2 -> means WRLK
    
    fdarr[0].fd = svr.listen_fd; //record server's info at fdarr, so that every poll() can also check server's status. This makes maintaining easier.
    fdarr[0].events = POLLIN;
    unsigned int nfds = 1;

    const char delimiters[4] = " "; //for parsing write_server's input
    char *token;

    // //set the listener "svr.listen_fd" to be nonblocking
    // fcntl(svr.listen_fd, F_SETFL, O_NONBLOCK);

    while (1) {
        // TODO: Add IO multiplexing
        if(poll(fdarr, nfds, -1) < 0){ //because timeout == -1 -> blocked -> if revent no change -> 那我也要睡拉
            if(errno == EINTR) //errno is set to EINTR when some system calls are interrupted and the system is not in a position to resume the system call after interrupt handling.
                continue;
        }
        // Check new connection
        if(fdarr[0].revents & POLLIN){ //server ready 2 read
            clilen = sizeof(cliaddr);
            conn_fd = accept(svr.listen_fd, (struct sockaddr*)&cliaddr, (socklen_t*)&clilen);
            if (conn_fd < 0) {
                if (errno == EINTR || errno == EAGAIN) continue;  // try again
                if (errno == ENFILE) {
                    (void) fprintf(stderr, "out of file descriptor table ... (maxconn %d)\n", maxfd);
                    continue;
                }
                ERR_EXIT("accept");
            }
            requestP[conn_fd].conn_fd = conn_fd;
            strcpy(requestP[conn_fd].host, inet_ntoa(cliaddr.sin_addr));
            fprintf(stderr, "getting a new request... fd %d from %s\n", conn_fd, requestP[conn_fd].host);

            fdarr[nfds].fd = conn_fd;
            fdarr[nfds].events = POLLOUT;
            requestP[fdarr[nfds].fd].wait_for_write = 0;
            exited[nfds] = false;
            sprintf(requestP[fdarr[nfds].fd].buf, "Please enter your id (to check your booking state):\n");
            ++nfds;
        }

        for(int i = 1; i < nfds; ++i){
            struct flock lock;
            if(fdarr[i].revents & POLLOUT){ //user i ready 2 write
                

                write(fdarr[i].fd, requestP[fdarr[i].fd].buf, strlen(requestP[fdarr[i].fd].buf)); //write data to client
                
                fdarr[i].events = POLLIN;

                if(exited[i]){
                    fprintf(stderr, "fd %d exited, now nfds = %d\n", fdarr[i].fd, nfds);
                    close(fdarr[i].fd);
                    free_request(&requestP[fdarr[i].fd]);
                    // dont forgot to fill up the blank in fdarr!
                    --nfds;
                    fdarr[i] = fdarr[nfds];
                    exited[i] = exited[nfds];
                    --i; // do request originally at nfds-1, which is now swapped to i;
                    fprintf(stderr, "fdarr[%d] is now fd %d, nfds = %d\n", i, fdarr[i].fd, nfds);
                }
            }
            else if(fdarr[i].revents & POLLIN){ //user i ready 2 read
                fdarr[i].events = POLLOUT;
                
                int ret = handle_read(&requestP[fdarr[i].fd]); // parse data from client to requestP[conn_fd].buf
                fprintf(stderr, "ret = %d\n", ret);
                if (ret < 0) {
                    fprintf(stderr, "fdarr[%d].fd = %d is wierd, requestP[%d(conn_fd)].buf = %s || requestP[%d(fdarr.fd)] = %s\n", i, fdarr[i].fd, conn_fd, requestP[conn_fd].buf, fdarr[i].fd, requestP[fdarr[i].fd].buf);
                    fprintf(stderr, "bad request from %s\n", requestP[fdarr[i].fd].host);
                    continue;
                }
            

                // TODO: handle requests from clients
#ifdef READ_SERVER      
                if(requestP[fdarr[i].fd].wait_for_write == 1){
                    if(strcmp(requestP[fdarr[i].fd].buf, "Exit") == 0){
                        lock.l_type = F_UNLCK;
                        fcntl(file_fd, F_SETLK, &lock);
                        lockedFile[requestP[fdarr[i].fd].id] = 0;
                        requestP[fdarr[i].fd].wait_for_write = 0;
                        exited[i] = true;
                    }
                    requestP[fdarr[i].fd].buf[0] = '\0';
                    continue; // not so sure... <- was told that test cases won't happen this scenario?
                }
                int tarID;
                buf_len = strlen(requestP[fdarr[i].fd].buf);
                if(buf_len != 6){
                    tarID = -1;
                    exited[i] = true;
                }
                else if(buf_len == 6){
                    tarID = atoi(requestP[fdarr[i].fd].buf) - 902001;
                }
                
                if(tarID < 0 || tarID >= 20){
                    sprintf(requestP[fdarr[i].fd].buf, "[Error] Operation failed. Please try again.\n");
                    exited[i] = true;
                }
                else{
                    lock.l_type = F_RDLCK;
                    lock.l_start = sizeof(record)*tarID;
                    lock.l_whence = SEEK_SET;
                    lock.l_len = sizeof(record);
                    
                    if(fcntl(file_fd, F_SETLK, &lock) == -1 || lockedFile[tarID] == 2){
                        sprintf(requestP[fdarr[i].fd].buf, "Locked.\n");
                        exited[i] = true;
                    }
                    else{
                        lockedFile[requestP[fdarr[i].fd].id] = 1;
                        record data;
                        lseek(file_fd, sizeof(record) * tarID, SEEK_SET);
                        read(file_fd, &data, sizeof(record));
                        sprintf(requestP[fdarr[i].fd].buf, "Food: %d booked\nConcert: %d booked\nElectronics: %d booked\n\n(Type Exit to leave...)\n", data.bookingState[FOOD_INDEX], data.bookingState[CONCERT_INDEX], data.bookingState[ELECS_INDEX]);
                        requestP[fdarr[i].fd].wait_for_write = 1;
                    }
                }
                
                // fprintf(stderr, "%s", requestP[conn_fd].buf);
                // sprintf(buf,"%s : %s",accept_read_header,requestP[conn_fd].buf);
                // write(requestP[conn_fd].conn_fd, buf, strlen(buf));    
#elif defined WRITE_SERVER
                if(requestP[fdarr[i].fd].wait_for_write != 1){
                    int tarID;
                    buf_len = strlen(requestP[fdarr[i].fd].buf);
                    if(buf_len != 6){
                        tarID = -1;
                        exited[i] = true;
                    }
                    else if(buf_len == 6){
                        tarID = atoi(requestP[fdarr[i].fd].buf) - 902001;
                    }
                    
                    if(tarID < 0 || tarID >= 20){
                        sprintf(requestP[fdarr[i].fd].buf, "[Error] Operation failed. Please try again.\n");
                        exited[i] = true;
                    }
                    else{
                        requestP[fdarr[i].fd].id = tarID;
                        lock.l_type = F_WRLCK;
                        lock.l_start = sizeof(record)*tarID;
                        lock.l_whence = SEEK_SET;
                        lock.l_len = sizeof(record);
                        
                        if(fcntl(file_fd, F_SETLK, &lock) == -1 || lockedFile[tarID] == 1|| lockedFile[tarID] == 2){
                            sprintf(requestP[fdarr[i].fd].buf, "Locked.\n");
                            exited[i] = true;
                        }
                        else{
                            lockedFile[requestP[fdarr[i].fd].id] = 2;
                            record data;
                            lseek(file_fd, sizeof(record) * tarID, SEEK_SET);
                            read(file_fd, &data, sizeof(record));
                            sprintf(requestP[fdarr[i].fd].buf, "Food: %d booked\nConcert: %d booked\nElectronics: %d booked\n\nPlease input your booking command. (Food, Concert, Electronics. Positive/negative value increases/decreases the booking amount.):\n", data.bookingState[FOOD_INDEX], data.bookingState[CONCERT_INDEX], data.bookingState[ELECS_INDEX]);
                            requestP[fdarr[i].fd].wait_for_write = 1;
                        }
                    }
                }
                else if(requestP[fdarr[i].fd].wait_for_write == 1){
                    //how to parse input //this implementation(strtok) is not sure. -> will we jump to other requestP[].buf during data parsing?
                    record modify;
                    modify.id = requestP[fdarr[i].fd].id;
                    token = strtok(requestP[fdarr[i].fd].buf, delimiters);
                    int idx = 0;
                    while(token != NULL){
                        bool valid = true;
                        int data = getNum(token, &valid);
                        if(!valid || idx >= 3){
                            sprintf(requestP[fdarr[i].fd].buf, "[Error] Operation failed. Please try again.\n");
                            exited[i] = true;
                            break;
                        }
                        token = strtok(NULL, delimiters);
                        modify.bookingState[idx] = data;
                        ++idx;
                    }
                    if(idx != 3){
                        sprintf(requestP[fdarr[i].fd].buf, "[Error] Operation failed. Please try again.\n");
                        exited[i] = true;
                    }

                    if(!exited[i]){
                        record res;
                        lseek(file_fd, sizeof(record) * requestP[fdarr[i].fd].id, SEEK_SET);
                        read(file_fd, &res, sizeof(record));
                        res.bookingState[FOOD_INDEX] += modify.bookingState[FOOD_INDEX];
                        res.bookingState[CONCERT_INDEX] += modify.bookingState[CONCERT_INDEX];
                        res.bookingState[ELECS_INDEX] += modify.bookingState[ELECS_INDEX];
                        int totBook = res.bookingState[FOOD_INDEX] + res.bookingState[CONCERT_INDEX] + res.bookingState[ELECS_INDEX];
                        if(totBook > 15){
                            sprintf(requestP[fdarr[i].fd].buf, "[Error] Sorry, but you cannot book more than 15 items in total.\n");
                        }
                        else if(res.bookingState[FOOD_INDEX] < 0 || res.bookingState[CONCERT_INDEX] < 0 || res.bookingState[ELECS_INDEX] < 0){
                            sprintf(requestP[fdarr[i].fd].buf, "[Error] Sorry, but you cannot book less than 0 items.\n");
                        }
                        else{
                            lseek(file_fd, sizeof(record) * requestP[fdarr[i].fd].id, SEEK_SET);
                            write(file_fd, &res, sizeof(record));
                            sprintf(requestP[fdarr[i].fd].buf, "Bookings for user %d are updated, the new booking state is:\nFood: %d booked\nConcert: %d booked\nElectronics: %d booked\n", res.id, res.bookingState[FOOD_INDEX], res.bookingState[CONCERT_INDEX], res.bookingState[ELECS_INDEX]);
                        }
                        exited[i] = true;
                    }
                    lock.l_start = sizeof(record) * requestP[fdarr[i].fd].id;
                    lock.l_whence = SEEK_SET;
                    lock.l_type = F_UNLCK;
                    lock.l_len = sizeof(record);
                    lockedFile[requestP[fdarr[i].fd].id] = 0;
                    fcntl(file_fd, F_SETLK, &lock);
                }
                // fprintf(stderr, "%s", requestP[conn_fd].buf);
                // sprintf(buf,"%s : %s",accept_write_header,requestP[conn_fd].buf);
                // write(requestP[conn_fd].conn_fd, buf, strlen(buf));    
#endif
            }
            // close(requestP[conn_fd].conn_fd);
            // free_request(&requestP[conn_fd]);
        }
    }
    close(file_fd);
    free(requestP);
    return 0;
}

// ======================================================================================================
// You don't need to know how the following codes are working
#include <fcntl.h>

static void init_request(request* reqP) {
    reqP->conn_fd = -1;
    reqP->buf_len = 0;
    reqP->id = 0;
}

static void free_request(request* reqP) {
    /*if (reqP->filename != NULL) {
        free(reqP->filename);
        reqP->filename = NULL;
    }*/
    init_request(reqP);
}

static void init_server(unsigned short port) {
    struct sockaddr_in servaddr;
    int tmp;

    gethostname(svr.hostname, sizeof(svr.hostname));
    svr.port = port;

    svr.listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (svr.listen_fd < 0) ERR_EXIT("socket");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
    tmp = 1;
    if (setsockopt(svr.listen_fd, SOL_SOCKET, SO_REUSEADDR, (void*)&tmp, sizeof(tmp)) < 0) {
        ERR_EXIT("setsockopt");
    }
    if (bind(svr.listen_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        ERR_EXIT("bind");
    }
    if (listen(svr.listen_fd, 1024) < 0) {
        ERR_EXIT("listen");
    }

    // Get file descripter table size and initialize request table
    maxfd = getdtablesize();
    requestP = (request*) malloc(sizeof(request) * maxfd);
    if (requestP == NULL) {
        ERR_EXIT("out of memory allocating all requests");
    }
    for (int i = 0; i < maxfd; i++) {
        init_request(&requestP[i]);
    }
    requestP[svr.listen_fd].conn_fd = svr.listen_fd;
    strcpy(requestP[svr.listen_fd].host, svr.hostname);

    return;
}
