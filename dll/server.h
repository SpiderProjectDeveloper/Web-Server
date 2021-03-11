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

#include "auth.h"
#include "helpers.h"

#define SRV_MAX_EXE_PATH 400
#define SRV_HTML_ROOT_DIR ""
#define SRV_MAX_HTML_ROOT_PATH (SRV_MAX_EXE_PATH + 1 + sizeof(SRV_HTML_ROOT_DIR))

#define SRV_SESSIONF_FILE_NAME "server.sess"

void server_response( int client_socket, char *socket_request_buf, int socket_request_buf_size, 
	char *html_source_dir, callback_ptr _callback );
