#include "server.h"

static int _listen_socket = INVALID_SOCKET;

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
	    if( strlen(ssd->HtmlPath) >= SRV_MAX_EXE_PATH ) {
    		return -1;
        }
	}

  _th_manager_on = true;	
	std::thread thm(thread_manager_start);	
	thm.detach();

	std::thread th(server, ssd, callback);	
	th.detach();

	return 0;
} 

// ******** THE SERVER

static const int _socket_request_buf_size = 1024*5000;
static char _socket_request_buf[_socket_request_buf_size + 1];

static char _html_root_path[SRV_MAX_HTML_ROOT_PATH+1]; 			// Root directory for html applications

static int server( StartServerData *ssd, callback_ptr callback )
{
	WSADATA wsaData; //  use Ws2_32.dll
	size_t result;

	result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (result != 0) {
		error_message( "WSAStartup failed: ", result );
        thread_manager_stop();
		return result;
	}

	struct addrinfo* addr = NULL; // holds socket ip etc

	// To be initialized with constants and values...
	struct addrinfo hints;
	ZeroMemory(&hints, sizeof(hints));

	hints.ai_family = AF_INET; // AF_INET - using net to work with socket
	hints.ai_socktype = SOCK_STREAM; // A socket of a "stream" type
	hints.ai_protocol = IPPROTO_TCP; // TCP protocol
	hints.ai_flags = AI_PASSIVE; // The socket should take incoming connections

	result = getaddrinfo(ssd->IP, ssd->Port, &hints, &addr); // Port 8000 is used
	if (result != 0) { 		// If failed...
		error_message( "getaddrinfo failed: ", result );
		WSACleanup(); // unloading  Ws2_32.dll
        thread_manager_stop();
		return 1;
	}
	// Creating a socket
	_listen_socket = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
	if (_listen_socket == INVALID_SOCKET) { 		// If failed to create a socket...
		error_message( "Error at socket: ", WSAGetLastError() );
		freeaddrinfo(addr);
		WSACleanup();
        thread_manager_stop();
		return 1;
	}

	// Binsding the socket to ip-address
	result = bind(_listen_socket, addr->ai_addr, (int)addr->ai_addrlen);
	if (result == SOCKET_ERROR) { 		// If failed to bind...
		error_message( "bind failed with error: ", WSAGetLastError() );
		freeaddrinfo(addr);
		closesocket(_listen_socket);
		WSACleanup();
        thread_manager_stop();
		return 1;
	}

	// Init listening...
	if (listen(_listen_socket, SOMAXCONN) == SOCKET_ERROR) {
		error_message( "listen failed with error: ", WSAGetLastError() );
		freeaddrinfo(addr);
		closesocket(_listen_socket);
		WSACleanup();
        thread_manager_stop();
		return 1;
	}

	int client_socket = INVALID_SOCKET;

	if( ssd->HtmlPath != nullptr ) { 
		strcpy( _html_root_path, ssd->HtmlPath);
		//strcat( _html_root_path, "\\" );
	} else {
		_html_root_path[0] = '\x0';
	}
	strcat( _html_root_path, SRV_HTML_ROOT_DIR );

	for (;;) {
		// Accepting an incoming connection...
		error_message( "accepting..." );
		client_socket = accept(_listen_socket, NULL, NULL);
		if (client_socket == INVALID_SOCKET) {
    		error_message( "accept failed with error: ", WSAGetLastError() );
			break;
		}

		result = recv(client_socket, _socket_request_buf, _socket_request_buf_size, 0);

		if (result == SOCKET_ERROR) { 	// Error receiving data
			error_message("server: recv failed...");
			closesocket(client_socket);
		}
		else if (result == 0) { 		// The connection was closed by the client...
			error_message("server: connection closed...");
		} 
		else if( result >= _socket_request_buf_size ) {
			error_message("server: too longrequest: ", result );
			closesocket(client_socket);
		}
		else if (result > 0) { 		// Everything is ok and "result" stores data length
			_socket_request_buf[result] = '\0';
			error_message( "server [request]:\n", _socket_request_buf, "\nlength=", result );
			server_response( client_socket, _socket_request_buf, result, _html_root_path, callback );
			closesocket(client_socket);
		}
	}

	// Closing everything...
    error_message("Closing everything!");    
    if( _listen_socket != INVALID_SOCKET ) {
	    closesocket(_listen_socket);
        _listen_socket = INVALID_SOCKET;
    }
	freeaddrinfo(addr);
	WSACleanup();
    thread_manager_stop();
	return 0;
}
