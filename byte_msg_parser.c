/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "byte_msg_parser.h"

//will construct a request response message
void construct_request_response_msg(request_status_msg_t *msg, uint8_t status){
    
}

//will populate the message header info. returns success status
int parse_header_info(char *full_msg, msg_header_t *header){
    return 1;
}

//will populate the create room msg struct from the byte msg. returns parse success status
int parse_create_room_msg(char *byte_msg, create_room_msg_t *new_msg){
    return 1;
}

//will populate the connect msg from byte msg. returns the parse success status
int parse_connect_to_container_msg(char *byte_msg, connect_to_container_msg_t *new_msg){
    return 1;
}