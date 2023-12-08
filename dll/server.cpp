#include "server.h"

WSADATA _wsaData; //  use Ws2_32.dll
sockaddr_in _sock_addr;
static int _listen_socket = INVALID_SOCKET;
static const int _socket_request_buf_size = 1024*5000;
static char _socket_request_buf[_socket_request_buf_size + 1];
static char _html_root_path[SRV_MAX_HTML_ROOT_PATH+1]; 			// Root directory for html applications

std::condition_variable _th_manager_cond;
std::mutex _th_manager_mtx;
bool _th_manager_on = false;

void thread_manager_start() {
    std::unique_lock<std::mutex> L{_th_manager_mtx};
    _th_manager_cond.wait( L, [&]()
    {
        return !_th_manager_on;
    });
    if( _listen_socket != INVALID_SOCKET ) {
        closesocket( _listen_socket );
    }
    _listen_socket = INVALID_SOCKET;
}

void thread_manager_stop() {
    std::lock_guard<std::mutex> L{_th_manager_mtx};
    if( !_th_manager_on ) {
        return;
    }
    _th_manager_on = false;
    _th_manager_cond.notify_one();
}


static int server( StartServerData *ssd, callback_ptr callback );

int start( StartServerData *ssd, callback_ptr callback ) {
    if( ssd->Message == ssd_Stop ) {       // The server must be shutdown.
        thread_manager_stop();
        return 0;
    }

	if( _th_manager_on == true ) {
		return -1;
	}
    if( ssd->HtmlPath != nullptr ) {
		error_message( std::string("start(): ssd->HtmlPath = ") + ssd->HtmlPath);
	    if( strlen(ssd->HtmlPath) >= SRV_MAX_EXE_PATH ) {
    		return -1;
        }
      strcpy( _html_root_path, ssd->HtmlPath);
	} else {
		error_message("start(): ssd->HtmlPath not spicified!");
		_html_root_path[0] = '\x0';
	}
	strcat( _html_root_path, SRV_HTML_ROOT_DIR );

	int port;
	try {
		port = std::stoi( ssd->Port );
	} catch (const std::invalid_argument& ia) {
		thread_manager_stop();
		return -1;
	}

	_sock_addr.sin_family = AF_INET;
	/*
	if( ssd->IP == nullptr ) {
		_sock_addr.sin_addr.s_addr = INADDR_ANY;
	} else if( strlen(ssd->IP) == 0 ) {
		_sock_addr.sin_addr.s_addr = INADDR_ANY; 
	} else {
		_sock_addr.sin_addr.s_addr = inet_addr(ssd->IP);
	}
	*/
	_sock_addr.sin_addr.s_addr = INADDR_ANY;
	_sock_addr.sin_port = htons(port);

	size_t result;

	result = WSAStartup(MAKEWORD(2, 2), &_wsaData);
	if (result != 0) {
		error_message( std::string("WSAStartup failed: ") + std::to_string( result ) );
		return -1;
	}

	_listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (_listen_socket == INVALID_SOCKET) {
	error_message( std::string("getaddrinfo failed: ") + std::to_string(result) );
	WSACleanup(); // unloading  Ws2_32.dll
	return -1;
	}

	result = bind(_listen_socket, (SOCKADDR *) &_sock_addr, sizeof (_sock_addr));
	if (result == SOCKET_ERROR) { 		// If failed to bind...
		error_message( std::string("bind failed with error: ") + std::to_string(WSAGetLastError()) );
		closesocket(_listen_socket);
		WSACleanup();
		return -1;
	}

	// Init listening...
	if (listen(_listen_socket, SOMAXCONN) == SOCKET_ERROR) {
		error_message( std::string("listen failed with error: ") + std::to_string(WSAGetLastError()) );
		closesocket(_listen_socket);
		WSACleanup();
		return -1;
	}

	_th_manager_on = true;	
	std::thread thm(thread_manager_start);	
	thm.detach();

	std::thread th(server, ssd, callback);	
	th.detach();

	return 0;
} 


// ******** THE SERVER
static int server( StartServerData *ssd, callback_ptr callback )
{
  size_t result;    
	int client_socket = INVALID_SOCKET;

	for (;;) {
		// Accepting an incoming connection...
		error_message( "accepting..." );
		client_socket = accept(_listen_socket, NULL, NULL);
		if (client_socket == INVALID_SOCKET) {
    		error_message( std::string("accept failed with error: ") + std::to_string(WSAGetLastError()) );
			break;
		}

		int bytes_read = 0;
		int bytes_in_buffer_available = _socket_request_buf_size;
		int content_read = -1;
		int content_length = -1;
		do { 
			result = recv(client_socket, &_socket_request_buf[bytes_read], bytes_in_buffer_available, 0);
			if( result == SOCKET_ERROR )
				break;			
			bytes_read += result;
			bytes_in_buffer_available -= result;
			error_message( std::string("bytes_read: ") + std::to_string(bytes_read) );

			// Trying to get the "Content-Length" value. If failed - breaking, since it's either a GET request or possibly not a valid POST request
			if( content_length == -1 ) {
				content_length = get_content_length( _socket_request_buf, bytes_read );
			}
			if( content_length == -1 ) {
				break;
			}			

			content_read = get_content_read( _socket_request_buf, bytes_read );
			if( content_read == -1 ) { 	// If failed to get the num. of bytes after the header - breaking
				break;								// it's something wrong with the request
			}
			if( content_read == 0 ) { 	// 
				if( content_length == 0 ) {         // If a zero length request - breaking...
					break;
				}					
			} else if( content_read > 0 ) { 		// Some content has been read...
				if( content_read == content_length ) {	// If all bytes sent are read...
					break; 								// ... breaking
				}										
			}
		} while( result > 0 && bytes_in_buffer_available > 0 );

		if (result == SOCKET_ERROR) { 	// Error receiving data
			error_message("server: recv failed...");
			closesocket(client_socket);
		} else if( !(bytes_in_buffer_available > 0) ) {
			error_message("server: the message is too long...");
			closesocket(client_socket);
		} else if( bytes_read == 0 ) { 	
			error_message("server: the connection was closed by the client...");
			closesocket(client_socket);
		} else {
			_socket_request_buf[bytes_read] = '\0';
			error_message( "server [request]:\n" + std::string(_socket_request_buf) + "\nlength=" + std::to_string(bytes_read) );
			server_response( client_socket, _socket_request_buf, bytes_read, _html_root_path, callback );
			closesocket(client_socket);
		}
	}

	// Closing everything...
	error_message("Closing everything!");    
	if( _listen_socket != INVALID_SOCKET ) {
		closesocket(_listen_socket);
		_listen_socket = INVALID_SOCKET;
	}
	WSACleanup();
  thread_manager_stop();
	return 0;
}
