/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   byte_msg_parser.h
 * Author: drew
 *
 * Created on June 17, 2018, 9:32 AM
 */

#ifndef BYTE_MSG_PARSER_H
#define BYTE_MSG_PARSER_H

#ifdef __cplusplus
extern "C" {
#endif
    
#include <stdlib.h>
#include <stdint.h>
#include <netinet/in.h>

#define CREATE_ROOM_TYPE 1 //UDP
#define CONNECT_TO_CONTAINER_TYPE 2 //TCP
#define JOIN_ROOM_REQUEST 3 //UDP
#define PLAYER_CMND_TYPE 4 //UDP
#define QUEUE_UPDATE_TYPE 5	//UDP
#define CHAT_MESSAGE_TYPE 6 //UDP


#define REQUEST_STATUS_TYPE 50
#define CREATE_ROOM_RESPONSE_TYPE 51

#define HEADER_LENGTH 14

typedef uint16_t msg_type_t;

typedef struct{
	msg_type_t msg_type; //2 bytes
	uint32_t user_id; // 4 byte
	uint32_t room_id; //0 when creating room. 4 bytes
	size_t msg_length; //4 bytes
	//maybe some other stuff in the future
} msg_header_t;

typedef struct{
	uint8_t is_private;
	//uint32_t user_id;
	//initial player state info probably
} create_room_msg_t;

typedef struct{
	//uint32_t room_id;
	//uint32_t user_id;
	//maybe some other stuff in the future
} connect_to_container_msg_t;

typedef struct{
	msg_header_t header;
	uint8_t status;
} request_status_msg_t;

typedef struct{
	msg_header_t header;
	uint8_t status;
	struct sockaddr_in container_addr;
} create_room_response_t;

typedef struct{
	
} player_cmnd_msg_t;

//will construct a create room response
void construct_create_room_response_msg(create_room_response_t *msg, uint8_t status, struct sockaddr_in container_addr, uint32_t user_id, uint32_t room_id);

//will construct a request response message
void construct_request_response_msg(request_status_msg_t *msg, uint8_t status, uint32_t user_id, uint32_t room_id);

//will populate the message header info. returns success status
int parse_header_info(char *full_msg, msg_header_t *header);

//will populate the create room msg struct from the byte msg. returns parse success status
int parse_create_room_msg(char *byte_msg, create_room_msg_t *new_msg);

//will populate the connect msg from byte msg. returns the parse success status
int parse_connect_to_container_msg(char *byte_msg, connect_to_container_msg_t *new_msg);


#ifdef __cplusplus
}
#endif

#endif /* BYTE_MSG_PARSER_H */

