#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
//#include <stdint.h>
#include <pthread.h>
#include <time.h>
#include "distribution_container_manager.h"

#define BUFFER_LENGTH 204700

#define MAX_CONTAINERS 10000
#define ROOMS_PER_CONTAINER 5
#define ROOM_MEMBERS_PER_CONTAINER 50

#define MAX_MSG_SIZE 256
#define on_error(...) { fprintf(stderr, __VA_ARGS__); fflush(stderr); exit(1); }

//TODO:better method for finding a free port when creating new container. for now, increment this after new creation
int next_free_port = 20000;

int container_count = 0;

int message_number = 0;

int users_joined = 0;

int connections_made = 0;

int number_of_table_entries = 0;

//interace addr to assign to container connection info
unsigned long if_addr;

//family to use for connections
short container_family;

unsigned long first = 0;

//map of distribution containers for room id
struct hashtable *containers_for_room_id;//keys: room id
                                                      //values: array of container states

container_list_node *head = NULL;
container_list_node *next_open_container = NULL;

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

    //initialize hashtable
    containers_for_room_id = create_hashtable(5, csfrn_hashfromkey, csfrn_equalkeys);
}


void print_time(){
    time_t my_time;
    struct tm * timeinfo;
    time (&my_time);
    timeinfo = localtime (&my_time);

    printf("%d:%d:%d:%d:%d:%d :: ", timeinfo->tm_year+1900, timeinfo->tm_mon+1, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);fflush(stdout);
    struct timeval start;
    gettimeofday(&start, NULL);
    if(first==0){
        first = start.tv_usec;
    }
    printf("%lu-->%lu", start.tv_usec, first);
}

/*****************************************************************************/

static void create_new_distribution_container_info(distribution_container_info_t *new_container_info) {
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

    //populate container
    new_container_info->sock = sock;
    new_container_info->active_connection_count = 0;
    new_container_info->connection_info.container_addr = addr;
    new_container_info->route_map = create_hashtable(5, route_map_hashfromkey, route_map_equalkeys);
}

//*********************Join Listener Thread*********************************

//TODO:update this function with new byte message layout
request_status_msg_t
handle_received_data_at_container(distribution_container_info_t *container_info, char *data, struct sockaddr_in remote,
                                  int connected_fd) {

    //parse the received message
    msg_header_t msg_header;
    parse_header_info(data, &msg_header);

    //check msg type
    if (msg_header.msg_type == CONNECT_TO_CONTAINER_TYPE) {
        //printf("table count: %d\n",msg_header.room_id);fflush(stdout);
        void *found;
        if (NULL == (found = hashtable_search(container_info->route_map, &msg_header.room_id))) {
            //error handling
            printf("route table fucked up...\n");
            fflush(stdout);
        }

        remote_connection_data_t *remote_data;
        remote_data = (remote_connection_data_t *) found;

        int connected_to_container = 0;
        int i = 0;
        while (i < ROOM_MEMBERS_PER_CONTAINER && !connected_to_container) {
            if ((remote_data->connections + i)->user_id == msg_header.user_id) {
                users_joined++;
                printf("user %d subscribed to room...\n", users_joined);fflush(stdout);
                (remote_data->connections + i)->remote_addr = remote;
                (remote_data->connections + i)->connected_fd = connected_fd;
                connected_to_container = 1;
            }
            i++;
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

    distribution_container_info_t *container_info;
    container_info = (distribution_container_info_t *) dc;

    int sock = container_info->sock;

    if (listen(sock, ROOM_MEMBERS_PER_CONTAINER) < 0) {
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
        //TODO:FD_SETSIZE??
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
                        //printf("connected to %s:%hu -- %d\n", inet_ntoa(remote.sin_addr), ntohs(remote.sin_port), connections_made);
                    fflush(stdout);
                    connections_made++;
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
                        //printf("handling join request...\n");
                        fflush(stdout);
                        request_status_msg_t response = handle_received_data_at_container(container_info, msg_buffer, remote,
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
//*********************Join Listener Thread (END)*********************************

//*********************Distribute Data Thread*********************************

void *container_distribute_recv_data(void *dc) {

    distribution_container_info_t *container_info;
    container_info = (distribution_container_info_t *) dc;

    pthread_mutex_lock(&container_info->recv_data.recv_lock);

    while (1) {
        //wait until buffer has data to distribute
        pthread_cond_wait(&container_info->recv_data.buffer_has_data, &container_info->recv_data.recv_lock);
        //printf("attempting to distribute data...%d/%d\n", container_info->recv_data.tail, container_info->recv_data.head);fflush(stdout);
        //distribute all available data
        while (container_info->recv_data.tail != container_info->recv_data.head) {
            //get the next header from the buffer
            msg_header_t *next_header = malloc(sizeof(msg_header_t));
            int wrap_around_offset = 0;
            if(container_info->recv_data.tail > container_info->recv_data.head){
                int space_left = BUFFER_LENGTH-container_info->recv_data.tail;
                if(space_left >= HEADER_LENGTH){
                    parse_header_info(container_info->recv_data.recv_buffer + container_info->recv_data.tail, next_header);
                }else{
                    char temp_header_buffer[HEADER_LENGTH];

                    memcpy(temp_header_buffer, container_info->recv_data.recv_buffer + container_info->recv_data.tail, space_left);
                    memcpy(temp_header_buffer+space_left, container_info->recv_data.recv_buffer, HEADER_LENGTH-space_left);
                    container_info->recv_data.tail = HEADER_LENGTH-space_left;
                    wrap_around_offset = HEADER_LENGTH;

                    parse_header_info(temp_header_buffer, next_header);
                }
            }else{
                parse_header_info(container_info->recv_data.recv_buffer + container_info->recv_data.tail, next_header);
            }

            void *found;
            int len = next_header->msg_length;
            if (NULL == (found = hashtable_search(container_info->route_map, &next_header->room_id))) {
                //error handling
                printf("route map not found for room %d..\n", next_header->room_id);fflush(stdout);
            } else {
                remote_connection_data_t *remote_data;
                remote_data = (remote_connection_data_t *) found;
                //printf("remote connection count: %d\n", remote_data->connection_count);fflush(stdout);

                char send_buffer[len];
                if(container_info->recv_data.tail+len > BUFFER_LENGTH){
                    int space_left = BUFFER_LENGTH-container_info->recv_data.tail;
                    memcpy(send_buffer, container_info->recv_data.recv_buffer + container_info->recv_data.tail, space_left);
                    memcpy(send_buffer+space_left, container_info->recv_data.recv_buffer, len-space_left);
                    container_info->recv_data.tail = len-space_left;
                    wrap_around_offset = len;
                }else if(wrap_around_offset!=0){
                    memcpy(send_buffer, next_header, HEADER_LENGTH);
                    memcpy(send_buffer+HEADER_LENGTH, container_info->recv_data.recv_buffer + container_info->recv_data.tail, len-HEADER_LENGTH);
                }else {
                    memcpy(send_buffer, container_info->recv_data.recv_buffer + container_info->recv_data.tail, len);
                }
                message_number += remote_data->connection_count;
                int i;
                for (i = 0; i < remote_data->connection_count; i++) {
                    //send the data to each connected socket
                    int connected_socket = (remote_data->connections + i)->connected_fd;
                    //printf("checking if client %u is connected...\n", (remote_data->connections + i)->user_id);fflush(stdout);
                    //check if it's actually connected yet
                    if (connected_socket > 0) {
                        //printf("should be sending %d bytes...\n", next_header->msg_length);fflush(stdout);
                        //printf("tail:%d head:%d...\n", container_info->recv_data.tail, container_info->recv_data.head);fflush(stdout);
                        int bytes_sent = send(connected_socket,
                                              send_buffer,
                                              len, 0);
                        //printf("sent client %d bytes...\n", bytes_sent);printf(stdout);
                        if(bytes_sent<0){
                            on_error("Client write failed\n");
                        }
                    } else {
                        //it's probably waiting to get the connection request or there was an error
                    }
                }
            }
            free(next_header);
            //move tail up. TODO: circular buffer
            container_info->recv_data.tail += (len-wrap_around_offset);
        }

        printf("---------------- finished sent messages #%d @ ", number_of_table_entries);fflush(stdout);
        print_time();
        printf("---------------- \n");fflush(stdout);
        //finished sending all the data. buffer can be filled again
        //pthread_mutex_unlock(&container_info->recv_data.recv_lock);
    }
}

//*********************Distribute Data Thread (END)*********************************

//*********************New User Management******************************************

void activate_new_container(uint32_t room_id, uint32_t user_id,
                            container_connection_info_t *container_connection_info, container_table_entry *container_entry) {
    int ret;
    //create new distribution container and add to the hashtable
    distribution_container_info_t *new_container_info = malloc(sizeof(distribution_container_info_t));
    create_new_distribution_container_info(new_container_info);

    new_container_info->recv_data.recv_buffer = calloc(BUFFER_LENGTH+1, sizeof(char));

    //add user that created it to the intial route map
    remote_connection_data_t *remote_connection_data = calloc(1, sizeof(remote_connection_data));
    remote_connection_data->connections = calloc(ROOM_MEMBERS_PER_CONTAINER, sizeof(remote_connection_info_t));

    (remote_connection_data->connections + 0)->user_id = user_id;
    (remote_connection_data->connections + 0)->connected_fd = -1; //not connected
    remote_connection_data->connection_count = 1;

    uint32_t *rm_key = malloc(sizeof(uint32_t));
    memcpy(rm_key, &room_id, sizeof(uint32_t));

    if (!hashtable_insert(new_container_info->route_map, rm_key, remote_connection_data)) {
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
    pthread_mutex_init(&new_container_info->recv_data.recv_lock, 0);

    //init thread conds
    pthread_cond_init(&new_container_info->recv_data.buffer_has_data, NULL);
    pthread_cond_init(&new_container_info->recv_data.buffer_full, NULL);

    //add container to container state and start its worker threads
    distribution_container *new_container = calloc(1,sizeof(distribution_container));
    new_container->container_info = new_container_info;

    //start threads
    if ((ret = pthread_create(&new_container->worker_threads[0], &attr, container_connection_listener,
                              (void *) new_container_info)) != 0) {
        //error handling
    }

    if ((ret = pthread_create(&new_container->worker_threads[1], &attr, container_distribute_recv_data,
                              (void *) new_container_info)) != 0) {
        //error handling
    }


    uint32_t *rm_key_2 = malloc(sizeof(uint32_t));
    memcpy(rm_key_2, &room_id, sizeof(uint32_t));

    container_entry->containers[container_entry->count] = new_container;
    container_entry->count++;

    //populate container connection info before returning
    memcpy(container_connection_info, &new_container->container_info->connection_info, sizeof(container_connection_info_t));

    //add to linked list
    if(head == NULL){
        head = malloc(sizeof(container_list_node));
        head->container = new_container;
        head->next = NULL;
        next_open_container = head;
    }else{
        container_list_node *new_node = malloc(sizeof(container_list_node));
        new_node->container = new_container;
        new_node->next = NULL;
        next_open_container->next = new_node;
        next_open_container = new_node;
    }

    container_count++;
}

void findOpenContainer(){

}

int handle_new_connection_request(uint32_t room_id, uint32_t user_id,
                                  container_connection_info_t *container_connection_info) {
    uint32_t *temp = malloc(sizeof(uint32_t));
    memcpy(temp, &room_id, sizeof(uint32_t));
    int ret = 1;
    void *found;
    if (NULL == (found = hashtable_search(containers_for_room_id, temp))) {
        free(temp);
        printf("---adding new entry---\n");fflush(stdout);

        number_of_table_entries++;

        uint32_t *rm_key_2 = malloc(sizeof(uint32_t));
        memcpy(rm_key_2, &room_id, sizeof(uint32_t));

        //new container entry
        container_table_entry *container_entry = malloc(sizeof(container_table_entry));
        //distribution_container **containers = malloc(20 * sizeof(*distribution_container));
        container_entry->containers = malloc(51 * sizeof(distribution_container*));
        container_entry->count = 0;

        //insert new state into hashtable.
        if (!hashtable_insert(containers_for_room_id, rm_key_2, container_entry)) {
            ret = -1;
            printf("didnt add state");fflush(stdout);
            //goto(EXIT);
        }

        if(container_count < MAX_CONTAINERS){
            printf("---creating new container---\n");fflush(stdout);
            activate_new_container(room_id, user_id, container_connection_info, container_entry);
        }else{
            printf("---using existing container---\n");fflush(stdout);
            int found_open_container = 0;
            int i = 0;
            while(!found_open_container && i < MAX_CONTAINERS){

                //check if next open container actually has space
                if(hashtable_count(next_open_container->container->container_info->route_map) < ROOMS_PER_CONTAINER){
                    //add connections to this container

                    //add user that created it to the intial route map
                    remote_connection_data_t *remote_connection_data = calloc(1, sizeof(remote_connection_data));
                    remote_connection_data->connections = calloc(ROOM_MEMBERS_PER_CONTAINER, sizeof(remote_connection_info_t));

                    (remote_connection_data->connections + 0)->user_id = user_id;
                    (remote_connection_data->connections + 0)->connected_fd = -1; //not connected
                    remote_connection_data->connection_count = 1;

                    uint32_t *rm_key = malloc(sizeof(uint32_t));
                    memcpy(rm_key, &room_id, sizeof(uint32_t));

                    if (!hashtable_insert(next_open_container->container->container_info->route_map, rm_key, remote_connection_data)) {
                        //error handling
                        printf("couldn't add conneciton info");fflush(stdout);
                    }else{
                        found_open_container = 1;
                        //memcpy((container_entry->containers + container_entry->count), &next_open_container->container,
                        //       sizeof(*distribution_container));
                        container_entry->containers[container_entry->count] = next_open_container->container;
                        container_entry->count++;

                        uint32_t *rm_key_2 = malloc(sizeof(uint32_t));
                        memcpy(rm_key_2, &room_id, sizeof(uint32_t));

                        memcpy(container_connection_info, &next_open_container->container->container_info->connection_info, sizeof(container_connection_info_t));
                    }
                }

                //check if you need to loop around to head
                if(next_open_container->next == NULL){
                    next_open_container = head;
                }else{
                    next_open_container = next_open_container->next;
                }

                i++;
            }
            ret = found_open_container;
        }
    } else {
        free(temp);
        printf("---found existing entry---\n");fflush(stdout);
        //check if existing distribution containers have space, create new if not
        container_table_entry *container_entry;
        container_entry = (container_table_entry *) found;

        int was_added = 0;
        int i=0;
        while(i < container_entry->count && !was_added){
            //start from the end, most likely to be open
            distribution_container *container = *(container_entry->containers + (container_entry->count-1-i));
            void *found2;
            if (NULL == (found2 = hashtable_search(container->container_info->route_map, &room_id))) {
               //TODO?
                printf("---didnt find route map---\n");fflush(stdout);
            }else{
                //TODO: adding connections needs to be thread safe
                printf("---checking if container has space---\n");fflush(stdout);
                remote_connection_data_t *remote_connection_data;
                remote_connection_data = (remote_connection_data_t *) found2;
                if (remote_connection_data->connection_count < ROOM_MEMBERS_PER_CONTAINER) {
                    int next_connection = remote_connection_data->connection_count;
                    (remote_connection_data->connections + next_connection)->user_id = user_id;
                    (remote_connection_data->connections + next_connection)->connected_fd = -1;
                    //printf("why is this here?");fflush(stdout);
                    remote_connection_data->connection_count++;
                    container->container_info->active_connection_count++;

                    was_added = 1;

                    memcpy(container_connection_info, &container->container_info->connection_info, sizeof(container_connection_info_t));
                }
            }

            i++;
        }

        if(!was_added && container_count < MAX_CONTAINERS){
            printf("---no space in existing containers. creating new---\n");fflush(stdout);
            activate_new_container(room_id, user_id, container_connection_info, container_entry);
        }
    }

    EXIT:
    return ret;
}
//*********************New User Management(END)*********************************


//*********************Receiving Room Data*********************************
void handle_incoming_data(msg_header_t header, char *data){

    printf("---------------- Got message @ ");fflush(stdout);
    print_time();
    printf("---------------- \n");

    void *found;
    if(NULL == (found = hashtable_search(containers_for_room_id, &header.room_id))){
        //error handling
        printf("failed to find container for data distibution\n");fflush(stdout);
        //return 0;
    }else{
        //pass data to appropriate container to distribute
        container_table_entry *container_entry;
        container_entry = (container_table_entry *) found;
        int i=0;
        while(i < container_entry->count){
            distribution_container *container = *(container_entry->containers + i);

            //acquire recv lock
            pthread_mutex_lock(&container->container_info->recv_data.recv_lock);

            int len = header.msg_length;
            printf("filling ditribution data buffer @ %d...\n", container->container_info->recv_data.head);fflush(stdout);
            if(len+container->container_info->recv_data.head > BUFFER_LENGTH){
                int space_left = (BUFFER_LENGTH-container->container_info->recv_data.head);
                memcpy(container->container_info->recv_data.recv_buffer + container->container_info->recv_data.head, data, space_left);
                memcpy(container->container_info->recv_data.recv_buffer, data+space_left, len-space_left);
                container->container_info->recv_data.head = len-space_left;
            }else {
                memcpy(container->container_info->recv_data.recv_buffer + container->container_info->recv_data.head, data, len);
                container->container_info->recv_data.head += len;
            }

            pthread_cond_signal(&container->container_info->recv_data.buffer_has_data);
            pthread_mutex_unlock(&container->container_info->recv_data.recv_lock);
            i++;
        }
        //return 1;
    }
}

//*********************Receiving Room Data(END)*********************************


//*********************Shutdown*************************************************

void s_handler(int signal){
    printf("1\n");fflush(stdout);
    if(signal == SIGINT){
        printf("2\n");fflush(stdout);
        struct hashtable_itr *iterator;
        iterator = hashtable_iterator(containers_for_room_id);

        int closed = 0;
        int i=0;
        if(hashtable_count(containers_for_room_id) > 0){
            do{
                container_table_entry *te;
                te = hashtable_iterator_value(iterator);

                int j=0;
                while(j<te->count){
                    close(te->containers[j]->container_info->sock);
                    closed++;
                    j++;
                }

                i++;
            }while(hashtable_iterator_advance(iterator));
        }
        printf("closed %d sockets...exiting\n", closed);fflush(stdout);
        exit(0);
    }
}

//*********************Shutdown(END)********************************************
