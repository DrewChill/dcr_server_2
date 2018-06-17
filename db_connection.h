/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   db_connection.h
 * Author: drew
 *
 * Created on June 17, 2018, 9:33 AM
 */

#ifndef DB_CONNECTION_H
#define DB_CONNECTION_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "byte_msg_parser.h"
//#include <mysql56/my_global.h>
//#include <mysql.h>

typedef struct{
	uint32_t room_id;
} dj_room_t;

//takes the creation info and populates the dj room object. returns success state
int create_new_dj_room(msg_header_t header, create_room_msg_t create_info, dj_room_t *dj_room);


#ifdef __cplusplus
}
#endif

#endif /* DB_CONNECTION_H */