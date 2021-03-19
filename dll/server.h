#pragma once

// #define __DEV__ 1

#include <thread>
#include <mutex>
#include <condition_variable>
#include <string>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <vector>

#define _WIN32_WINNT 0x501
#include <WinSock2.h>
#include <WS2tcpip.h>

#define SERVER_DLL_EXPORT
#include "WebServer.hpp"

#include "helpers.h"

#define SRV_MAX_EXE_PATH 400
#define SRV_HTML_ROOT_DIR ""
#define SRV_MAX_HTML_ROOT_PATH (SRV_MAX_EXE_PATH + 1 + sizeof(SRV_HTML_ROOT_DIR))

#define SRV_SESS_ID_LEN 80
#define SRV_USER_MAX_LEN 80

#define _max_response_size 99999999
#define _max_response_size_digits 8

#define _http_header_template "HTTP/1.1 200 OK\r\nVersion: HTTP/1.1\r\nContent-Type:%s\r\nContent-Length:%lu\r\n\r\n"
#define _http_header_buf_size  (sizeof(_http_header_template) + MIME_BUF_SIZE + _max_response_size_digits) 


class ResponseWrapper {
	public:
	char header[_http_header_buf_size+1];
	char *body;
	char *body_allocated;
	int body_len;

	ResponseWrapper(): body(nullptr), body_allocated(nullptr), body_len(0) {
		header[0] = '\x0';
	}	

	~ResponseWrapper() {
		if( body_allocated != nullptr ) {
			delete [] body;
		}
	}	
};

class ServerDataWrapper {
	public:

	ServerData sd;
	
	ServerDataWrapper() {
		sd.user = nullptr;
		sd.sess_id = nullptr;
		sd.message = nullptr;
		sd.sp_response_buf = nullptr;
		sd.sp_free_response_buf = false;
  	sd.sp_response_is_file = false;
	}

	~ServerDataWrapper() {
		if( sd.sp_free_response_buf == true ) {
			delete [] sd.sp_response_buf;
		}
	}
};


void server_response( int client_socket, char *socket_request_buf, int socket_request_buf_size, 
	char *html_source_dir, callback_ptr _callback );

char* server_login( ServerDataWrapper& sdw, char *user, char *pass, callback_ptr callback );
bool server_is_logged( ServerDataWrapper& sdw, char *sess_id, callback_ptr callback, bool is_update_session=false );
bool server_logout( ServerDataWrapper& sdw, char *sess_id, callback_ptr callback );