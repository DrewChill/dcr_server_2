#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdint.h>
//#include <mysql56/my_global.h>
//#include <mysql.h>
#include "distribution_container_manager.h"
#include "db_connection.h"

/*****************************************************************************/
//static unsigned int
//hashfromkey(void *ky)
//{
//    session_info__t *k = (session_info__t *)ky;
//    return (((k->dst_addr.sin_addr.s_addr << 17) | (k->dst_addr.sin_addr.s_addr >> 15)) ^ k->src_addr.sin_addr.s_addr) +
//    (k->dst_port * 17) + (k->src_port * 13 * 29);
//}
//
//static int
//equalkeys(void *k1, void *k2)
//{
//    return (0 == memcmp(k1,k2,sizeof(session_info__t)));
//}

/*****************************************************************************/

//will listen for requests to create a DJ room and create initial distribution container
void *listen_for_create_requests() {

    int sock;
    struct sockaddr_in addr;
    int addrLength;

    printf("creating room create socket...\n");
    fflush(stdout);
    //create the socket
    if ((sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
        printf("couldn't create socket\n");
        exit(1);
    }

    //establish addr
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(40000);

    //printf("binding socket...\n");
    //fflush(stdout);
    //bind the socket
    if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        printf("couldn't bind socket\n");
        exit(1);
    }

    int remoteLen, numBytesRecved, numBytesReturned;
    char data[2048];
    struct sockaddr_in remote;

    //get remote addr length
    remoteLen = sizeof(remote);


    //Request structure:
    //[0-31] = user id
    //[32-?] = initial player state


    while (1) {
        //read from socket
        printf("waiting for room create requests...\n");
        fflush(stdout);
        numBytesRecved = recvfrom(sock, data, 2048, 0, (struct sockaddr *) &remote, &remoteLen);
        printf("Bytes received:%d\n", numBytesRecved);
        fflush(stdout);
        //get remote connection info
        char *addr = inet_ntoa(remote.sin_addr);
        int port = ntohs(remote.sin_port);

        //print sender addr
        printf("Got request from %s:%d\n", addr, port);
        fflush(stdout);

        //handle the request
        //1. sql cmnd to add room to db. return room id

        msg_header_t *header = malloc(sizeof(msg_header_t));
        parse_header_info(data, header);
        printf("parsed the header...\n");
        fflush(stdout);

        if (header->msg_type == CREATE_ROOM_TYPE) {
            create_room_msg_t *create_room_msg = malloc(sizeof(create_room_msg_t));
            //		if(parse_create_room_msg(data, &create_room_msg)<0){
            //			//error handler
            //		}
            create_room_msg->is_private = 0;

            dj_room_t *new_dj_room = malloc(sizeof(new_dj_room));
            if (create_new_dj_room(*header, *create_room_msg, new_dj_room) < 0) {
                //error handler
            }
            printf("\nadded to the db...\n");
            fflush(stdout);

            //2. use room id to create distribution container. return connection info for container
            container_connection_info_t *container_connection = malloc(sizeof(container_connection_info_t));
            if (handle_new_connection_request(new_dj_room->room_id, header->user_id, container_connection) < 0) {
                //error handler
            }

            printf("created distribution container...\n");

            //3. send connection info back to user
            create_room_response_t *response = malloc(sizeof(create_room_response_t));
            construct_create_room_response_msg(response, 1, container_connection->container_addr, header->user_id,
                                               new_dj_room->room_id);

            unsigned char buffer[21];
            memcpy(buffer, &response->header.msg_type, 2);
            memcpy(buffer + 2, &response->header.user_id, 4);
            memcpy(buffer + 6, &response->header.room_id, 4);
            memcpy(buffer + 10, &response->header.msg_length, 4);
            memcpy(buffer + 14, &response->status, 1);
            short host_order = ntohs(response->container_addr.sin_port);
            memcpy(buffer + 15, &host_order, 2);
            memcpy(buffer + 17, &response->container_addr.sin_addr.s_addr, 4);

            numBytesReturned = sendto(sock, buffer, 21, 0, (struct sockaddr *) &remote, sizeof(remote));

        } else if (header->msg_type == JOIN_ROOM_REQUEST) {
            //2. use room id to create distribution container. return connection info for container
            container_connection_info_t *container_connection = malloc(sizeof(container_connection_info_t));
            if (handle_new_connection_request(header->room_id, header->user_id, container_connection) < 0) {
                //error handler
            }

            create_room_response_t *response = malloc(sizeof(create_room_response_t));
            construct_create_room_response_msg(response, 1, container_connection->container_addr, header->user_id,
                                               header->room_id);

            unsigned char buffer[21];
            memcpy(buffer, &response->header.msg_type, 2);
            memcpy(buffer + 2, &response->header.user_id, 4);
            memcpy(buffer + 6, &response->header.room_id, 4);
            memcpy(buffer + 10, &response->header.msg_length, 4);
            memcpy(buffer + 14, &response->status, 1);
            short host_order = ntohs(response->container_addr.sin_port);
            memcpy(buffer + 15, &host_order, 2);
            memcpy(buffer + 17, &response->container_addr.sin_addr.s_addr, 4);

            numBytesReturned = sendto(sock, buffer, 21, 0, (struct sockaddr *) &remote, sizeof(remote));

        } else if(header->msg_type == PLAYER_CMND_TYPE){
            printf("attempting to distribute data...\n");fflush(stdout);
            handle_incoming_data(*header, data);
        }

    }
}

//will listen for join requests and assign them to a distribution container
//void *listen_for_join_requests(){
//
//}

int main(int argc, char *argv[]) {
    init_distribution_container_manager();
    init_db_connection();

    int ret = 0;

    //create worker threads
    pthread_attr_t attr;
    if ((ret = pthread_attr_init(&attr)) != 0) {
        //error handling
    }

    if ((ret = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE)) != 0) {
        //error handling
    }

    pthread_t create_listener;
    pthread_create(&create_listener, &attr, listen_for_create_requests, NULL);

    pthread_join(create_listener, NULL);
}
