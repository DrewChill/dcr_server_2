#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
//#include <stdint.h>
#include <pthread.h>
#include "distribution_container_manager.h"

#define MAX_MSG_SIZE 256
#define on_error(...) { fprintf(stderr, __VA_ARGS__); fflush(stderr); exit(1); }

//TODO:better method for finding a free port when creating new container. for now, increment this after new creation
int next_free_port = 20000;

//interace addr to assign to container connection info
unsigned long if_addr;

//family to use for connections
short container_family;

//map of distribution containers for room id
struct hashtable *distribution_containers_for_room_id;//keys: room id
                                                      //values: array of container states

/************************************CONTAINER STATES FOR ROOM NUMBER HASHTABLE FUNCTION*****************************************/
static unsigned int
csfrn_hashfromkey(void *ky) {
    uint32_t *iky = (uint32_t *) ky;
    uint32_t key = *iky;

    return (key * 2654435761) % (2 << 30); //this might be dumb
}

static int
csfrn_equalkeys(void *k1, void *k2) {
    return (0 == memcmp(k1, k2, sizeof(uint32_t)));
}

/*****************************************************************************/

/************************************ROUTE MAP HASHTABLE FUNCTIONS*****************************************/
static unsigned int
route_map_hashfromkey(void *ky) {
    uint32_t *iky = (uint32_t *) ky;
    uint32_t key = *iky;

    return (key * 2654435761) % (2 << 30); //this might be dumb
}

//
static int
route_map_equalkeys(void *k1, void *k2) {
    uint32_t *val = (uint32_t *)k1;
    uint32_t *key = (uint32_t *)k2;
    //printf("key/val %u/%u",*key,*val);fflush(stdout);
    return (0 == memcmp(k1, k2, sizeof(uint32_t)));
}

void init_distribution_container_manager() {
    //get interface ip addr
    //distribution_containers_for_room_id = malloc(sizeof(struct hashtable));

    //initialize hashtable
    //struct hashtable *h;
    distribution_containers_for_room_id = create_hashtable(5, csfrn_hashfromkey, csfrn_equalkeys);

    //memcpy(distribution_containers_for_room_id, h, sizeof(struct hashtable));
}

/*****************************************************************************/

static void create_new_distribution_container(distribution_container *new_container) {
    //create socket for container
    int sock;
    struct sockaddr_in addr;

    /* Create the socket. */
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    /* Give the socket a name. */
    addr.sin_family = container_family;
    addr.sin_port = htons(next_free_port);
    //addr.sin_addr.s_addr = htonl (if_addr);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    next_free_port++;

    //create the route hashtable for the container
    //struct hashtable *h;
    //h = create_hashtable(5, route_map_hashfromkey, route_map_equalkeys);

    //populate container
    new_container->sock = sock;
    new_container->active_connection_count = 0;
    new_container->connection_info.container_addr = addr; //use memcpy here?
    new_container->route_map = create_hashtable(5, route_map_hashfromkey, route_map_equalkeys);
    //memcpy(new_container->route_map, h, sizeof(struct hashtable));
}

//TODO:update this function with new byte message layout
request_status_msg_t
handle_received_data_at_container(distribution_container *container, char *data, struct sockaddr_in remote,
                                  int connected_fd) {

    //parse the received message
    msg_header_t msg_header;
    //char body[MAX_MSG_SIZE - sizeof(msg_header)];
    parse_header_info(data, &msg_header);

    //char response_buffer[MAX_MSG_SIZE];

    //check msg type
    if (msg_header.msg_type == CONNECT_TO_CONTAINER_TYPE) {

        printf("got right message type...\n");
        fflush(stdout);

        //parse container connection ack
        //connect_to_container_msg_t connect_to_container_msg;

        //memcpy(body, data + sizeof(msg_header), sizeof(connect_to_container_msg_t));

        //parse_connect_to_container_msg(body, &connect_to_container_msg);

        //remote_connection_info_t *user_buffer;
        printf("table count: %d\n",msg_header.room_id);fflush(stdout);
        //uint32_t fuck = msg_header.room_id;
        void *found;
        if (NULL == (found = hashtable_search(container->route_map, &msg_header.room_id))) {
            //error handling
            printf("route table fucked up...\n");
            fflush(stdout);
        }

        remote_connection_data_t *remote_data;
        remote_data = (remote_connection_data_t *) found;

        uint8_t connected_to_container = 0;
        int i;
        for (i = 0; i < MAX_CONNECTIONS; i++) {
            if ((remote_data->connections + i)->user_id == msg_header.user_id) {
                printf("added user to distro...\n");
                fflush(stdout);
                (remote_data->connections + i)->remote_addr = remote;
                (remote_data->connections + i)->connected_fd = connected_fd;
                //remote_data->connection_count = remote_data->connection_count+1;
                connected_to_container = 1;
            }
        }

        request_status_msg_t response;
        construct_request_response_msg(&response, connected_to_container, msg_header.user_id, msg_header.room_id);

        return response;
    } else {
        request_status_msg_t response;
        construct_request_response_msg(&response, 0, msg_header.user_id, msg_header.room_id);

        return response;
    }
}

void *container_connection_listener(void *dc) {

    distribution_container *container;
    container = (distribution_container *) dc;

    int sock = container->sock;

    if (listen(sock, 5) < 0) {
        printf("fuuuuck");
        fflush(stdout);
    }

    fd_set active_fd_set, read_fd_set;

    //initialize active fd set w/ container socket
    FD_ZERO(&active_fd_set);
    FD_SET(sock, &active_fd_set);

    printf("listening for joiners...\n");
    fflush(stdout);

    while (1) {
        //block until request is received
        read_fd_set = active_fd_set;
        //TODO:FD_SETSIZE?? ya this is lazy as shit
        if (select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL)) {
            //error handling
        }

        //handle the sockets with input
        int i;
        for (i = 0; i < FD_SETSIZE; i++) {
            if (FD_ISSET(i, &read_fd_set)) {
                if (i == sock) {
                    int new_conn;
                    struct sockaddr_in remote;
                    size_t size = sizeof(remote);
                    //accept the new connection
                    new_conn = accept(sock, (struct sockaddr *) &remote, &size);
                    if (new_conn < 0) {
                        //error handling
                    }
                    //TODO: probably verify that it is from an expected ip address. (port would be unknown)
                    if (remote.sin_port != 0)
                        printf("connected to %s:%hu\n", inet_ntoa(remote.sin_addr), ntohs(remote.sin_port));
                    fflush(stdout);
                    FD_SET(new_conn, &active_fd_set);
                } else {
                    //got something from an already connected socket
                    char msg_buffer[MAX_MSG_SIZE];
                    int bytesRead;
                    struct sockaddr_in remote;

                    socklen_t length;
                    length = (socklen_t)sizeof(remote);
                    //read data/parse
                    bytesRead = recvfrom(i, msg_buffer, MAX_MSG_SIZE, 0, (struct sockaddr *) &remote, &length);
                    if (bytesRead < 0) {
                        //error handling
                    } else if (bytesRead == 0) {
                        //wut? don't anything I guess
                    } else {
                        //handle_received
                        printf("handling join request...\n");
                        fflush(stdout);
                        request_status_msg_t response = handle_received_data_at_container(container, msg_buffer, remote,
                                                                                          i);
                        char buffer[HEADER_LENGTH+1];
                        memcpy(buffer, &response.header.msg_type, 2);
                        memcpy(buffer+2, &response.header.user_id, 4);
                        memcpy(buffer+6, &response.header.room_id, 4);
                        memcpy(buffer+10, &response.header.msg_length, 4);
                        memcpy(buffer+14, &response.status, 1);
                        send(i, buffer, HEADER_LENGTH+1, 0);
                    }
                }
            }
        }

    }
}

void *container_distribute_recv_data(void *dc) {

    distribution_container *container;
    container = (distribution_container *) dc;

    pthread_mutex_lock(&container->recv_data.recv_lock);

    while (1) {
        //wait until buffer has data to distribute
        pthread_cond_wait(&container->recv_data.buffer_has_data, &container->recv_data.recv_lock);
        //printf("attempting to distribute data...\n");fflush(stdout);
        //distribute all available data
        while (container->recv_data.tail != container->recv_data.head) {
            //get the next header from the buffer
            msg_header_t *next_header = malloc(sizeof(msg_header_t));
            int wrap_around_offset = 0;
            if(container->recv_data.tail > container->recv_data.head){
                int space_left = 2047-container->recv_data.tail;
                if(space_left >= HEADER_LENGTH){
                    parse_header_info(container->recv_data.recv_buffer + container->recv_data.tail, next_header);
                }else{
                    char temp_header_buffer[HEADER_LENGTH];

                    memcpy(temp_header_buffer, container->recv_data.recv_buffer + container->recv_data.tail, space_left);
                    memcpy(temp_header_buffer+space_left, container->recv_data.recv_buffer, HEADER_LENGTH-space_left);
                    container->recv_data.tail = HEADER_LENGTH-space_left;
                    wrap_around_offset = HEADER_LENGTH;

                    parse_header_info(container->recv_data.recv_buffer + container->recv_data.tail, next_header);
                }
            }else{
                parse_header_info(container->recv_data.recv_buffer + container->recv_data.tail, next_header);
            }
            //grab message body from the buffer. (some messages may just have the header info)

            //parse and distribute message...actually is parsing even necessary? probably not
            void *found;
            if (NULL == (found = hashtable_search(container->route_map, &next_header->room_id))) {
                //error handling
                printf("route map not found for room %d..\n", next_header->room_id);fflush(stdout);
            } else {
                remote_connection_data_t *remote_data;
                remote_data = (remote_connection_data_t *) found;
                printf("remote connection count: %d\n", remote_data->connection_count);
                int i;
                for (i = 0; i < remote_data->connection_count; i++) {
                    //send the data to each connected socket
                    int connected_socket = (remote_data->connections + i)->connected_fd;
                    printf("checking if client %u is connected...\n", (remote_data->connections + i)->user_id);fflush(stdout);
                    //check if it's actually connected yet
                    if (connected_socket > 0) {
                        printf("should be sending %d bytes...\n", next_header->msg_length);fflush(stdout);
                        int len = next_header->msg_length;
                        if(container->recv_data.tail+len > 2047){
                            int space_left = 2047-container->recv_data.tail;
                            char temp_buffer[len];
                            memcpy(temp_buffer, container->recv_data.recv_buffer + container->recv_data.tail, space_left);
                            memcpy(temp_buffer+space_left, container->recv_data.recv_buffer, len-space_left);
                            container->recv_data.tail = len-space_left;
                            wrap_around_offset = len;

                            int bytes_sent = send(connected_socket,
                                                  temp_buffer,
                                                  len, 0);
                            printf("sent client %d bytes...\n", bytes_sent);printf(stdout);
                            if(bytes_sent<0){
                                on_error("Client write failed\n");
                            }
                        }else if(wrap_around_offset!=0){
                            int space_left = 2047-container->recv_data.tail;
                            char temp_buffer[len];
                            memcpy(temp_buffer, container->recv_data.recv_buffer + container->recv_data.tail, space_left);
                            memcpy(temp_buffer+space_left, container->recv_data.recv_buffer, len-space_left);

                            int bytes_sent = send(connected_socket,
                                                  temp_buffer,
                                                  len, 0);
                            printf("sent client %d bytes...\n", bytes_sent);printf(stdout);
                            if(bytes_sent<0){
                                on_error("Client write failed\n");
                            }
                        }else {
                            int bytes_sent = send(connected_socket,
                                                  container->recv_data.recv_buffer + container->recv_data.tail,
                                                  len, 0);
                            printf("sent client %d bytes...\n", bytes_sent);printf(stdout);
                            if(bytes_sent<0){
                                on_error("Client write failed\n");
                            }
                        }
                    } else {
                        //it's probably waiting to get the connection request or there was an error
                    }
                }
            }
            free(next_header);
            //move tail up. TODO: circular buffer
            container->recv_data.tail += (next_header->msg_length-wrap_around_offset);
        }

        //finished sending all the data. buffer can be filled again
        pthread_mutex_unlock(&container->recv_data.recv_lock);
    }
}

void activate_new_container(uint32_t room_id, uint32_t user_id,
                            container_connection_info_t *container_connection_info) {
    int ret;
    //create new distribution container and add to the hashtable
    distribution_container *new_container = malloc(sizeof(distribution_container));
    create_new_distribution_container(new_container);

    new_container->recv_data.recv_buffer = calloc(2048, sizeof(char));

    //add user that created it to the intial route map
    remote_connection_data_t *remote_connection_data = calloc(1, sizeof(remote_connection_data));
    remote_connection_data->connections = calloc(MAX_CONNECTIONS, sizeof(remote_connection_info_t));

    (remote_connection_data->connections + 0)->user_id = user_id;
    (remote_connection_data->connections + 0)->connected_fd = -1; //not connected
    remote_connection_data->connection_count = 1;

    uint32_t *rm_key = malloc(sizeof(uint32_t));
    memcpy(rm_key, &room_id, sizeof(uint32_t));

    if (!hashtable_insert(new_container->route_map, rm_key, remote_connection_data)) {
        //error handling
        printf("couldn't add conneciton info");fflush(stdout);
    }

    //start worker threads for container
    //set as detached
    pthread_attr_t attr;
    if ((ret = pthread_attr_init(&attr)) != 0) {
        //error handling
    }

    if ((ret = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)) != 0) {
        //error handling
    }

    //init recv mutex
    pthread_mutex_init(&new_container->recv_data.recv_lock, 0);

    //init thread conds
    pthread_cond_init(&new_container->recv_data.buffer_has_data, NULL);
    pthread_cond_init(&new_container->recv_data.buffer_full, NULL);

    //add container to container state and start its worker threads
    container_state *new_state = calloc(1,sizeof(container_state));
    new_state->container = new_container;

    //start threads
    if ((ret = pthread_create(&new_state->worker_threads[0], &attr, container_connection_listener,
                              (void *) new_container)) != 0) {
        //error handling
    }

    if ((ret = pthread_create(&new_state->worker_threads[1], &attr, container_distribute_recv_data,
                              (void *) new_container)) != 0) {
        //error handling
    }


    uint32_t *rm_key_2 = malloc(sizeof(uint32_t));
    memcpy(rm_key_2, &room_id, sizeof(uint32_t));

    //insert new state into hashtable.
    if (!hashtable_insert(distribution_containers_for_room_id, rm_key_2, new_state)) {
        ret = -1;
        printf("didnt add state");fflush(stdout);
        //goto(EXIT);
    }

    //populate container connection info before returning
    memcpy(container_connection_info, &new_state->container->connection_info, sizeof(container_connection_info_t));
}

int handle_new_connection_request(uint32_t room_id, uint32_t user_id,
                                  container_connection_info_t *container_connection_info) {
    int ret = 1;
    void *found;
    if (NULL == (found = hashtable_search(distribution_containers_for_room_id, &room_id))) {
        activate_new_container(room_id, user_id, container_connection_info);
    } else {
        //check if existing distribution containers have space, create new if not
        container_state *existing_state;
        existing_state = (container_state *) found;
        distribution_container *container = existing_state->container;

        if(container->active_connection_count < MAX_CONNECTIONS){
            void *found2;
            if (NULL == (found2 = hashtable_search(container->route_map, &room_id))) {
                //shouldn't get here
                printf("join didn't work");fflush(stdout);
            } else {
                //add the user to the container

                //TODO: adding connections needs to be thread safe
                remote_connection_data_t *remote_connection_data;
                remote_connection_data = (remote_connection_data_t *) found2;
                if (remote_connection_data->connection_count < MAX_CONNECTIONS) {
                    int next_connection = remote_connection_data->connection_count;
                    (remote_connection_data->connections + next_connection)->user_id = user_id;
                    (remote_connection_data->connections + next_connection)->connected_fd = -1;
                    //printf("why is this here?");fflush(stdout);
                    remote_connection_data->connection_count++;
                    container->active_connection_count++;
                } else {
                    //create a new container to add it to. (or add to existing w/ space)
                    activate_new_container(room_id, user_id, container_connection_info);
                }
            }

            memcpy(container_connection_info, &container->connection_info, sizeof(container_connection_info_t));
        }else{

        }
    }

    EXIT:
    return ret;
}

void handle_incoming_data(msg_header_t header, char *data){
    void *found;
    if(NULL == (found = hashtable_search(distribution_containers_for_room_id, &header.room_id))){
        //error handling
        printf("failed to find container for data distibution\n");fflush(stdout);
        //return 0;
    }else{
        //pass data to appropriate container to distribute
        container_state *existing_state;
        existing_state = (container_state *) found;
        distribution_container *container = existing_state->container;

        //acquire recv lock
        pthread_mutex_lock(&container->recv_data.recv_lock);

        int len = header.msg_length;
        printf("filling ditribution data buffer...\n");fflush(stdout);
        if(len+container->recv_data.head > 2047){
            int space_left = (2047-container->recv_data.head);
            memcpy(container->recv_data.recv_buffer+(2047-container->recv_data.head), data, space_left);
            //
            memcpy(container->recv_data.recv_buffer, data+space_left, len-space_left);

            container->recv_data.head = len-space_left;
        }else {
            memcpy(container->recv_data.recv_buffer+container->recv_data.head, data, len);
            //printf("wait...\n");fflush(stdout);
            container->recv_data.head += len;
        }

        pthread_cond_signal(&container->recv_data.buffer_has_data);
        pthread_mutex_unlock(&container->recv_data.recv_lock);
        //return 1;
    }
}
