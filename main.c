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
void *listen_for_create_requests(){

	int sock;
        struct sockaddr_in addr;
        int addrLength;

        printf("creating room create socket...\n");
        fflush(stdout);
        //create the socket
        if((sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0){
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
        if(bind(sock, (struct sockaddr *)&addr, sizeof(addr))<0){
                printf("couldn't bind socket\n");
                exit(1);
        }

	int remoteLen, numBytesRecved;
        char data[2048];
        struct sockaddr_in remote;

        //get remote addr length
        remoteLen = sizeof(remote);


	//Request structure:
	//[0-31] = user id
	//[32-?] = initial player state


        while(1){
                //read from socket
                printf("waiting for room create requests...\n");
                fflush(stdout);
                numBytesRecved = recvfrom(sock, data, 2048, 0, (struct sockaddr *)&remote, &remoteLen);
                printf("Bytes received:%d\n",numBytesRecved);
                fflush(stdout);
                //get remote connection info
                char *addr = inet_ntoa(remote.sin_addr);
                int port = ntohs(remote.sin_port);

                //print sender addr
                printf("Got request from %s:%d\n", addr, port);
                fflush(stdout);

		//handle the request
		//1. sql cmnd to add room to db. return room id
		
		msg_header_t header;
		parse_header_info(data, &header);
		
		//create_room_msg_t create_room_msg = malloc(sizeof(create_room_msg_t));
//		if(parse_create_room_msg(data, &create_room_msg)<0){
//			//error handler
//		}

		dj_room_t new_dj_room = malloc(sizeof(new_dj_room));
		if(create_new_dj_room(NULL, &new_dj_room)<0){
			//error handler
		}
	
		//2. use room id to create distribution container. return connection info for container
		container_connection_info_t container_connection;
		if(handle_new_connection_request(new_dj_room.room_id, header.user_id, &container_connection)<0){
			//error handler
		}

		//3. send connection info back to user
        }
}

//will listen for join requests and assign them to a distribution container
//void *listen_for_join_requests(){
//
//}

int main(int argc, char *argv[]){

	//create worker threads

}