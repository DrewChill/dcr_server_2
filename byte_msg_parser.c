/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "byte_msg_parser.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

//will construct a create room response message
void construct_create_room_response_msg(create_room_response_t *msg, uint8_t status, struct sockaddr_in container_addr, uint32_t user_id, uint32_t room_id){
    msg_header_t header;
    header.body_length=1+sizeof(struct sockaddr_in);
    header.msg_type = CREATE_ROOM_RESPONSE_TYPE;
    printf("user_id:%u\n",user_id);fflush(stdout);
    printf("room_id:%u\n",room_id);fflush(stdout);
    header.room_id = room_id;
    header.user_id = user_id;

    msg->header = header;
    msg->status = status;
    //printf("%d",sizeof(container_addr));fflush(stdout);
    msg->container_addr = container_addr;
}

//will construct a request response message
void construct_request_response_msg(request_status_msg_t *msg, uint8_t status, uint32_t user_id, uint32_t room_id){
    msg_header_t *header = malloc(sizeof(msg_header_t));
    header->body_length=1;
    header->msg_type = REQUEST_STATUS_TYPE;
    header->room_id = room_id;
    header->user_id = user_id;
    
    msg->header = *header;
    msg->status = status;
}

//will populate the message header info. returns success status
int parse_header_info(char *full_msg, msg_header_t *header){
    //memcpy(header, full_msg, 2); //msg type
    //memcpy(header+2, full_msg+2, 4); //user_id
    //memcpy(header+6, full_msg+6, 4); //room_id
    uint16_t msg_type;
    memcpy(&msg_type, full_msg, 2);

    uint32_t user_id;
    memcpy(&user_id, full_msg+2, 4);

    uint32_t room_id;
    memcpy(&room_id, full_msg+6, 4);

    header->msg_type = msg_type;
    header->user_id = user_id;
    header->room_id = room_id;
    header->body_length = 0; //this might just be derived from msg type or bytesread
    
    //always assume success for now
    return 1;
}

//will populate the create room msg struct from the byte msg. returns parse success status
int parse_create_room_msg(char *byte_msg, create_room_msg_t *new_msg){
    //only header info is necessary right now
    return 1;
}

//will populate the connect msg from byte msg. returns the parse success status
int parse_connect_to_container_msg(char *byte_msg, connect_to_container_msg_t *new_msg){
    //only header info is necessary right now
    return 1;
}
