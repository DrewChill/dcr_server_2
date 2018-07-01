/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "db_connection.h"
#include "string.h"

void init_db_connection(){
	printf("connecting to database...\n");fflush(stdout);
	conn = mysql_init(NULL);
	if(conn == NULL){
		printf("could not connect to database...exiting");fflush(stdout);
		exit(1);
	}

	if(mysql_real_connect(conn, "localhost", "root", "dirtydangles45", "Vybes", 0, NULL, 0)==NULL){
                fprintf(stderr, "%s\n", mysql_error(conn));
                mysql_close(conn);
                exit(1);
        }
	
	printf("connected to db...\n");fflush(stdout);
}

int create_new_dj_room(msg_header_t header, create_room_msg_t create_info, dj_room_t *dj_room){

	char query_buffer[1000];
	snprintf(query_buffer, sizeof(query_buffer), "INSERT INTO `DJ_Room` (`DJ`,`isPrivate`) VALUES ('%lu', '%u') ON DUPLICATE KEY UPDATE `isPrivate`='%u'", header.user_id, 0, 0);

	//TODO: uhh this doesn't seem right
	char query_string[strlen(query_buffer)+1];
	memset(query_string, '\0', sizeof(query_string));
	strcpy(query_string, query_buffer);

	printf("dj:%lu ; query:%s", header.user_id, query_string);fflush(stdout);
	
	if(mysql_query(conn, query_string)){
		//query failed
		return 0;
	}else{
		//success
		
		//construct dj_room
		unsigned long room_id = mysql_insert_id(conn);
		dj_room->room_id = room_id;
		dj_room->user_id= header.user_id;
		dj_room->is_private = create_info.is_private;

		return 1;
	}
}
