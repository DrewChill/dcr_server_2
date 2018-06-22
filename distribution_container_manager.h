/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   distribution_container_manager.h
 * Author: drew
 *
 * Created on June 17, 2018, 9:28 AM
 */

#ifndef DISTRIBUTION_CONTAINER_MANAGER_H
#define DISTRIBUTION_CONTAINER_MANAGER_H

#include <stdint.h>
#include "hashtable/hashtable_private.h"
#include "byte_msg_parser.h"

#ifdef __cplusplus
extern "C" {
#endif  

#define MAX_CONNECTIONS 5

//note: maybe consolidate the two connection info types into just one thing. not sure if they will be different----
typedef struct{
        struct sockaddr_in container_addr;
} container_connection_info_t;

typedef struct{
        uint32_t user_id;
	int connected_fd;
        struct sockaddr_in remote_addr;
} remote_connection_info_t;
//-------

typedef struct{
	char recv_buffer[2048];
	int head;
	int tail;
	pthread_mutex_t recv_lock;
	pthread_cond_t buffer_has_data;
	pthread_cond_t buffer_full;
} container_recv_data_t;

typedef struct{
	remote_connection_info_t connections[MAX_CONNECTIONS];
	int connection_count;
} remote_connection_data_t;

typedef struct{
	int sock;
	int active_connection_count;
	container_connection_info_t connection_info;
	container_recv_data_t recv_data;
	struct hashtable route_map; //keys: room id
                                   //values: remote connection connection info
} distribution_container;

//couple the container and worker thread that runs for it
typedef struct{
	pthread_t worker_threads[2]; //for sending and receiving data
	distribution_container container;
} container_state;

//map of distribution containers for room id
struct hashtable distribution_containers_for_room_id;//keys: room id
						     //values: array of container states

void init_distribution_container_manager();

//handle a new connection request. fill continer connection info for container remote connection was added to
//pseudo code:
//  -check if room has a distribution container w/ available connection slots
//     -if yes, add user id to that container. if no, create new container, spawn worker thread for container
//  -returns a status
int handle_new_connection_request(uint32_t room_id, uint32_t user_id, container_connection_info_t *continer_connection_info);

//pass data to the appropriate distribution container
void handle_incoming_data(msg_header_t header, char *data);


#ifdef __cplusplus
}
#endif

#endif /* DISTRIBUTION_CONTAINER_MANAGER_H */

