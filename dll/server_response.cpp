#include "server.h"

static constexpr int _uri_buf_size = 400;		// Buffer size for uri
static constexpr int _get_buf_size = 4000;		// Buffer size for get request
static constexpr int _html_file_path_buf_size = SRV_MAX_HTML_ROOT_PATH + 1 + _uri_buf_size + 1;

static char _image_file_name_buf[_html_file_path_buf_size+1];

static constexpr int _content_to_serve_buf_size = 10000;
static char _content_to_serve_buf[ _content_to_serve_buf_size+1];

//static constexpr int _max_response_size = 99999999;
//static constexpr int _max_response_size_digits = 8;

static char _mime_buf[MIME_BUF_SIZE + 1];

//static constexpr char _http_header_template[] = "HTTP/1.1 200 OK\r\nVersion: HTTP/1.1\r\nContent-Type:%s\r\nContent-Length:%lu\r\n\r\n";
//static constexpr int _http_header_buf_size = sizeof(_http_header_template) + MIME_BUF_SIZE + _max_response_size_digits; 

static constexpr char _http_redirect_template[] = "HTTP/1.1 302 Found\r\nLocation: %s\r\n\r\n";

static constexpr char _http_empty_message[] = "HTTP/1.1 200 OK\r\nContent-Length:0\r\n\r\n";
static constexpr char _http_authorized[] = "HTTP/1.1 200 OK\r\nContent-Length:1\r\n\r\n1";
static constexpr char _http_not_authorized[] = "HTTP/1.1 200 OK\r\nContent-Length:1\r\n\r\n0";
static constexpr char _http_synchro_not_authorized[] = "HTTP/1.1 200 OK\r\nContent-Length:2\r\n\r\n-1";
static constexpr char _http_logged_out[] = "HTTP/1.1 200 OK\r\nContent-Length:1\r\n\r\n1";
static constexpr char _http_header_bad_request[] = "HTTP/1.1 400 Bad Request\r\nVersion: HTTP/1.1\r\nContent-Length:0\r\n\r\n";
static constexpr char _http_header_not_found[] = "HTTP/1.1 404 Not Found\r\nVersion: HTTP/1.1\r\nContent-Length:0\r\n\r\n";
static constexpr char _http_header_failed_to_serve[] = "HTTP/1.1 501 Internal Server Error\r\nVersion: HTTP/1.1\r\nContent-Length:0\r\n\r\n";

static constexpr char _http_ok_options_header[] = 
		"HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Methods: POST, GET, OPTIONS\r\n"
		"Access-Control-Allow-Headers: Content-Type\r\n\r\n";

static void readHtmlFileAndPrepareResponse( char *file_name, char *html_source_dir, ResponseWrapper &response );
static void querySPAndPrepareResponse( callback_ptr callback, char *uri, char *sess_id, char *user, 
	bool is_get, char *get, char *post, ResponseWrapper &response, ServerDataWrapper &sdw, 
	char *html_root_path );
static void send_redirect( int client_socket, char *uri );

#define AUTH_URI_NUM 4
static constexpr char *_auth_uri[] = { "/gantt/", "/input/", "/dashboard/", "/ifc/" };


void server_response( 
	int client_socket, char *socket_request_buf, int socket_request_buf_size, 
	char *html_source_dir, callback_ptr callback ) 
{
	static char uri[_uri_buf_size+1];
	char *post;
	bool is_get = false;
	static char get_encoded[_get_buf_size+1];
	static char get[_get_buf_size+1];
	bool is_options = false;

	int uri_status = get_uri_to_serve(socket_request_buf, uri, _uri_buf_size, &is_get, get_encoded, _get_buf_size, &post, &is_options);
	error_message( "get_encoded:", get_encoded );	
	if (uri_status != 0) { 	// Failed to parse uri - closing socket...
		send(client_socket, _http_empty_message, strlen(_http_empty_message), 0);
		error_message("server: uri_status != 0" );
		return;
	}
	if( is_get ) {
		int decode_status = decode_uri( get_encoded, get, _get_buf_size );
		if( decode_status != 0 ) {
			send(client_socket, _http_empty_message, strlen(_http_empty_message), 0);
			error_message("server: decode_status != 0" );
			return;
		}
		error_message( "get:", get );	
	}

	error_message("server: requested uri=", uri );
	char *sess_id = nullptr;
	char *user = nullptr;

	if( is_options ) { // An OPTIONS request - allowing all
		send(client_socket, _http_ok_options_header, strlen(_http_ok_options_header), 0);
		return;
	}	

	if( strcmp(uri,"/.check_connection") == 0 ) { 	// A system message simply to check availability of the server.
		send(client_socket, _http_empty_message, strlen(_http_empty_message), 0);
		return;
	}				

	ResponseWrapper response;
	ServerDataWrapper sdw;

	static char cookie_sess_id[ SRV_SESS_ID_LEN + 1 ];
	static char uri_sess_id[ SRV_SESS_ID_LEN + 1 ];
	static char referer_sess_id[ SRV_SESS_ID_LEN + 1 ];
	static char cookie_user[ SRV_USER_MAX_LEN + 1 ];
	static char post_user[ SRV_USER_MAX_LEN + 1 ];
	static char post_pass[ SRV_USER_MAX_LEN + 1 ];
	
	if( strcmp(uri,"/.check_authorized") == 0 ) { 	// A system message to check if user is authorized or not.
		get_user_and_session_from_cookie( socket_request_buf, cookie_user, SRV_USER_MAX_LEN, cookie_sess_id, SRV_SESS_ID_LEN );
		if( server_is_logged( sdw, cookie_sess_id, callback ) ) {
			send(client_socket, _http_authorized, strlen(_http_authorized), 0);
		} else {
			send(client_socket, _http_not_authorized, strlen(_http_not_authorized), 0);
		}		
		return;
	}		

	if( strcmp(uri,"/.login") == 0 ) { 	// A login try?
    bool logged_in = false;
		if( post != nullptr ) {
			get_user_and_pass_from_post( post, post_user, SRV_USER_MAX_LEN, post_pass, SRV_USER_MAX_LEN );
			sess_id = server_login( sdw, post_user, post_pass, callback );
			if( sess_id != nullptr ) {
				logged_in = true;
			}
		}
		if( logged_in && sess_id != nullptr ) {  	
			sprintf_s( _content_to_serve_buf, _content_to_serve_buf_size, _http_header_template, "text/plain", strlen(sess_id) );
			strcat_s( _content_to_serve_buf, _content_to_serve_buf_size, sess_id );
			send(client_socket, _content_to_serve_buf, strlen(_content_to_serve_buf), 0);
		} else {
			sprintf_s( _content_to_serve_buf, _content_to_serve_buf_size, _http_header_template, "text/plain", 0 );
			send(client_socket, _content_to_serve_buf, strlen(_content_to_serve_buf), 0);
		}
		return;
	} 
	
	if( strcmp(uri,"/.logout") == 0 ) { 	// Logging out?
		get_user_and_session_from_cookie( socket_request_buf, cookie_user, SRV_USER_MAX_LEN, cookie_sess_id, SRV_SESS_ID_LEN );
		server_logout(sdw, cookie_sess_id, callback);
		//send_redirect(client_socket, "/?login");  // ... redirecting	
		send(client_socket, _http_not_authorized, strlen(_http_not_authorized), 0);
		return;
	}
			
	if( strcmp(uri, "/") == 0 ) {
		strcpy(uri, "/index.html");
	}
		
	bool is_query_sp = false; // All SP queries require an authentificated user
	if( strlen(uri) > 1 ) {	
		if( uri[1] == '.' ) {
			is_query_sp = true;
		}
	}			
	if( is_query_sp ) {
		bool is_update_session;
		if( strcmp(uri,"/.check_gantt_synchro") == 0 || strcmp(uri,"/.check_input_synchro") == 0 ) { 		
			is_update_session = false;
		} else {
			is_update_session = true;
		}
		get_user_and_session_from_cookie( socket_request_buf, cookie_user, SRV_USER_MAX_LEN, cookie_sess_id, SRV_SESS_ID_LEN );
		if( !server_is_logged( sdw, cookie_sess_id, callback, is_update_session ) ) 
		{
			if( !is_update_session ) { 	
				send(client_socket, _http_synchro_not_authorized, strlen(_http_synchro_not_authorized), 0);
			} else {
				send( client_socket, _http_header_bad_request, sizeof(_http_header_bad_request), 0 );
			}
			return; 
		}
		try {
			querySPAndPrepareResponse( callback, uri, cookie_sess_id, cookie_user, is_get, get, post, 
				response, sdw, html_source_dir );
		}
		catch (...) {
			error_message( "Failed to create response..." );
			send(client_socket, _http_header_failed_to_serve, strlen(_http_header_failed_to_serve), 0);
			//closesocket(client_socket);
			return;
		}
	} else 
	{ 	// Serving a file... 
		bool is_auth_required = false;
		for( int i = 0 ; i < AUTH_URI_NUM ; i++ ) {
			if( strncmp( uri, _auth_uri[i], strlen(_auth_uri[i]) ) == 0 ) { // Auth is required!
				is_auth_required = true;
				break;
			}
		}
		if( is_auth_required ) 
		{
			bool isLogged = false;
			get_user_and_session_from_cookie( socket_request_buf, cookie_user, SRV_USER_MAX_LEN, cookie_sess_id, SRV_SESS_ID_LEN );
			if( server_is_logged(sdw, cookie_sess_id, callback) ) isLogged =  true;
			if( !isLogged && is_get ) {
				get_session_from_uri( get, uri_sess_id, SRV_SESS_ID_LEN  );
				if( server_is_logged( sdw, uri_sess_id, callback ) ) isLogged = true;
			}
			if( !isLogged && is_get ) {
				get_session_from_referer( socket_request_buf, referer_sess_id, SRV_SESS_ID_LEN  );
				if( server_is_logged( sdw, referer_sess_id, callback ) ) isLogged = true;
				error_message( "referer sess id: ", referer_sess_id );
				error_message( "referer is logged: ", isLogged);
			}
			if( !isLogged )
			{
				if( is_html_request(uri) ) { 	// If it is an html request...
					send_redirect(client_socket, "/");  				// ... redirectingto login.html
				} else { // Other requests - sending "Bad Request"				
					send( client_socket, _http_header_bad_request, sizeof(_http_header_bad_request), 0 );
				}
				return; 
			}
		} 
		try {
			error_message( uri[1] );
			readHtmlFileAndPrepareResponse( &uri[1], html_source_dir, response );
		}
		catch (...) {
			// error_message( "Failed to create response..." );
			send(client_socket, _http_header_failed_to_serve, strlen(_http_header_failed_to_serve), 0);
			//closesocket(client_socket);
			return;
		}
	}

	int send_header_result = send(client_socket, response.header, strlen(response.header), 0);
	if (send_header_result == SOCKET_ERROR) { 	// If error...
		// error_message( "header send failed: ", WSAGetLastError() );
	} else {
		if (response.body != nullptr && response.body_len > 0 ) {
			int send_body_result = send(client_socket, response.body, response.body_len, 0);
			if (send_body_result == SOCKET_ERROR) { 	// If error...
				// error_message( "send failed: ",  WSAGetLastError() );
			}
		} else if (response.body_allocated != nullptr && response.body_len > 0 ) {
			int send_body_result = send(client_socket, response.body_allocated, response.body_len, 0);
			if (send_body_result == SOCKET_ERROR) { 	// If error...
				// error_message( "send failed: ", WSAGetLastError() );
			}
		}
	}
}


static void querySPAndPrepareResponse( 
	callback_ptr callback,
	char *uri, 	// A query serve 
	char *sess_id, 		// Session id, "null" value means an unauthorized user
	char *user, 		// User name 
	bool is_get,		// If true, it's a get request, might have essential info in *get
	char *get, 			// Points to get request 
	char *post, 		// Points to post request
	ResponseWrapper &response,
	ServerDataWrapper &sdw,
	char *html_root_path )
{
	// Must verify if SP should respond with not a file but with a data
	int callback_return; 	// 
	sdw.sd.user = user;
	bool binary_data_requested = false;

	if( strcmp( uri, "/.contents" ) == 0 ) {
		sdw.sd.message_id = SERVER_GET_CONTENTS;
		callback_return = callback( &sdw.sd );
	} 
	else if( strcmp( uri, "/.check_gantt_synchro" ) == 0 && is_get ) {
		sdw.sd.message_id = SERVER_CHECK_GANTT_SYNCHRO;			
		sdw.sd.message = get;
		callback_return = callback( &sdw.sd );
	} 
	else if( strcmp( uri, "/.check_input_synchro" ) == 0 && is_get ) {
		sdw.sd.message_id = SERVER_CHECK_INPUT_SYNCHRO;
		sdw.sd.message = get;
		callback_return = callback( &sdw.sd );
	} 
	else if( strcmp( uri, "/.check_sdoc_synchro" ) == 0 && is_get ) {
		sdw.sd.message_id = SERVER_CHECK_SDOC_SYNCHRO;
		sdw.sd.message = get;
		callback_return = callback( &sdw.sd );
	} 
	else if( strcmp( uri, "/.save_gantt" ) == 0 && post != nullptr ) {
		sdw.sd.message_id = SERVER_SAVE_GANTT;
		sdw.sd.message = post;
		callback_return = callback( &sdw.sd );
	} 
	else if( strcmp( uri, "/.save_input" ) == 0 && post != nullptr ) {
		sdw.sd.message_id = SERVER_SAVE_INPUT;
		sdw.sd.message = post;
		callback_return = callback( &sdw.sd );
	} 
	else if( strcmp( uri, "/.save_sdoc" ) == 0 && post != nullptr ) {
		sdw.sd.message_id = SERVER_SAVE_SDOC;
		sdw.sd.message = post;
		callback_return = callback( &sdw.sd );
	} 
	else if( strcmp( uri, "/.set_table" ) == 0 && post != nullptr ) {
		sdw.sd.message_id = SERVER_SET_TABLE;
		sdw.sd.message = post;
		callback_return = callback( &sdw.sd );
	} 
	else if( strcmp( uri, "/.set_ifc" ) == 0 && post != nullptr ) {
		sdw.sd.message_id = SERVER_SET_IFC;
		sdw.sd.message = post;
		callback_return = callback( &sdw.sd );
	} 	
	else if( strcmp( uri, "/.create_project" ) == 0 && post != nullptr ) {
		sdw.sd.message_id = SERVER_CREATE_PROJECT;
		sdw.sd.message = post;
		callback_return = callback( &sdw.sd );
	} 
	else if( strcmp( uri, "/.project_exists" ) == 0 && post != nullptr ) {
		sdw.sd.message_id = SERVER_PROJECT_EXISTS;
		sdw.sd.message = post;
		callback_return = callback( &sdw.sd );
	} 
	else if( strcmp( uri, "/.save_image" ) == 0 && post != nullptr ) {
		sdw.sd.message_id = SERVER_SAVE_IMAGE;
		sdw.sd.message = post;
		callback_return = callback( &sdw.sd );
	} 
	else if( strcmp( uri, "/.get_image" ) == 0 && is_get ) {
		// sdw.sd.message_id = SERVER_GET_IMAGE;
		// sdw.sd.message = get;
		// callback_return = callback( &sdw.sd );
		// A temp. code
		strcpy_s(_image_file_name_buf, _html_file_path_buf_size, html_root_path );
		strcat_s(_image_file_name_buf, _html_file_path_buf_size, get);
		sdw.sd.sp_response_is_file = true;
		sdw.sd.sp_response_buf = _image_file_name_buf;
		sdw.sd.sp_response_buf_size = strlen(_image_file_name_buf);
		sdw.sd.sp_free_response_buf = false;
		callback_return = 0;
		// error_message( "IMAGE: get=", get, ", r=", callback_return, ", buf_size=", sdw.sd.sp_response_buf_size, ", isfile=", (int)sdw.sd.sp_response_is_file );
		// End of the temp. code
		binary_data_requested = true;
	} 
	else if( strcmp( uri, "/.gantt_data" ) == 0 && is_get ) {
		error_message("GANTT DATA!!!!");
		sdw.sd.message_id = SERVER_GET_GANTT;
		sdw.sd.message = get;
		callback_return = callback( &sdw.sd );
		error_message("AFTER GANTT DATA!!!!");
	} 
	else if( strcmp( uri, "/.input_data" ) == 0 && is_get ) {
		sdw.sd.message_id = SERVER_GET_INPUT;
		sdw.sd.message = get;
		callback_return = callback( &sdw.sd );
	}
	else if( strcmp( uri, "/.dashboard_data" ) == 0 && is_get ) {
		sdw.sd.message_id = SERVER_GET_DASHBOARD;
		sdw.sd.message = get;
		callback_return = callback( &sdw.sd );
	}
	else if( strcmp( uri, "/.sdoc_data" ) == 0 && is_get ) {
		sdw.sd.message_id = SERVER_GET_SDOC;
		sdw.sd.message = get;
		callback_return = callback( &sdw.sd );
	} 
	else if( strcmp( uri, "/.model_data" ) == 0 && is_get ) {
		sdw.sd.message_id = SERVER_GET_MODEL;
		sdw.sd.message = get;
		callback_return = callback( &sdw.sd );
	}
	else if( strcmp( uri, "/.get_ifc" ) == 0 && is_get ) {
		sdw.sd.message_id = SERVER_GET_IFC;
		sdw.sd.message = get;
		callback_return = callback( &sdw.sd );
		binary_data_requested = true;
	} 
	else if( strcmp( uri, "/.get_wexbim" ) == 0 && is_get ) {
		sdw.sd.message_id = SERVER_GET_WEXBIM;
		sdw.sd.message = get;
		callback_return = callback( &sdw.sd );
		binary_data_requested = true;
	} 
	else if( strcmp( uri, "/.get_project_props" ) == 0 && is_get ) {
		sdw.sd.message_id = SERVER_GET_PROJECT_PROPS;
		sdw.sd.message = get;
		callback_return = callback( &sdw.sd );
	} 
	else if( strcmp( uri, "/.get_project_last_updates" ) == 0 && is_get ) {
		sdw.sd.message_id = SERVER_GET_PROJECT_LAST_UPDATES;
		sdw.sd.message = get;
		callback_return = callback( &sdw.sd );
	} 	
	else if( strcmp( uri, "/.close_project" ) == 0 && is_get ) {
		sdw.sd.message_id = SERVER_CLOSE_PROJECT;
		sdw.sd.message = get;
		callback_return = callback( &sdw.sd );
	} 
	else if( strcmp( uri, "/.get_gantt_structs" ) == 0 && is_get ) {
		sdw.sd.message_id = SERVER_GET_GANTT_STRUCTS;
		sdw.sd.message = get;
		callback_return = callback( &sdw.sd );
	} 	


	if( sdw.sd.sp_response_buf == nullptr || 	// Might happen if mistakenly left as nullptr in SP.
		callback_return < 0 || sdw.sd.sp_response_buf_size == 0 || // An error 
		sdw.sd.sp_response_buf_size > _max_response_size ) 	 // The response is too big
	{
		strcpy(response.header, _http_header_bad_request); 
		error_message( "Bad Request..." );
	} else if( sdw.sd.sp_response_is_file ) { 	// sd.sp_response_buf contains a file name...
		error_message("BEFORE readHtmlFileAndPrepareResponse");
		readHtmlFileAndPrepareResponse( sdw.sd.sp_response_buf, nullptr, response );
	} else {
		error_message("RESPONSE RECEIVED!!!!");
		if( binary_data_requested ) { 	// An image? (or other binary data for future use)
			set_mime_type(uri, _mime_buf, MIME_BUF_SIZE);
		} else { 	// 
			strcpy_s( _mime_buf, MIME_BUF_SIZE, "text/json; charset=utf-8" );
		}
		sprintf_s( response.header, _http_header_buf_size, 
			_http_header_template, _mime_buf, (unsigned long)sdw.sd.sp_response_buf_size );
		response.body = sdw.sd.sp_response_buf;
		response.body_len = sdw.sd.sp_response_buf_size;
		//error_message( "header:\n" + response.header + "body:\n" + response.body );
	}
	return;
}


static void readHtmlFileAndPrepareResponse( char *file_name, char *html_source_dir, ResponseWrapper &response )
{
	// If none of the above - serving a file
	char file_path[ _html_file_path_buf_size ];
	
	if( html_source_dir != nullptr ) {
		strcpy( file_path, html_source_dir );
	} else {
		file_path[0] = '\x0';
	}
	strcat( file_path, file_name );
	
	error_message( "Opening a file: ", file_path );
	
	std::ifstream fin(file_path, std::ios::in | std::ios::binary);
	if (fin) {
		// Reading http response body
		fin.seekg(0, std::ios::end);
		long int file_size = static_cast<long int>(fin.tellg());
		fin.seekg(0, std::ios::beg);

		if( file_size > 0 ) {
			set_mime_type(file_name, _mime_buf, MIME_BUF_SIZE);	//
			if( file_size <= _content_to_serve_buf_size ) { 	// The static buffer is big enough...
				fin.read(_content_to_serve_buf, file_size); 
				if( fin.gcount() == file_size ) {
					response.body = _content_to_serve_buf;
					response.body_len = file_size;
					sprintf_s(response.header, _http_header_buf_size, 
						_http_header_template, _mime_buf, (unsigned long)(file_size) );
				} else {
					strcpy_s(response.header, _http_header_buf_size, _http_header_failed_to_serve);
				}
			}
			else if( file_size < _max_response_size ) { 	// The static buffer is not enough...
				try { 	// Allocating...
					response.body_allocated = new char[file_size + 1];
				} catch(...) {;}				
				if( response.body_allocated != nullptr ) { 	// If allocated Ok...
					fin.read(response.body_allocated, file_size); 	// Adding the file to serve
					if( fin.gcount() == file_size ) {
						response.body_len = file_size;
						sprintf_s(response.header, _http_header_buf_size, 
							_http_header_template, _mime_buf, (unsigned long)(file_size) );
					} else {
						strcpy_s(response.header, _http_header_buf_size, _http_header_failed_to_serve);
					}
				} else {
					sprintf_s(response.header, _http_header_buf_size, _http_header_failed_to_serve);
				}
			} else {
				sprintf_s(response.header, _http_header_buf_size, _http_header_failed_to_serve);
			}
		} else {
			sprintf_s(response.header, _http_header_buf_size, _http_header_failed_to_serve);
		}
		fin.close();
	} else {
		strcpy_s(response.header, _http_header_buf_size, _http_header_not_found);
	}
}


static void send_redirect( int client_socket, char *uri ) {	
	int required_size = sizeof(_http_redirect_template) + strlen(uri);
	if(  required_size >= _content_to_serve_buf_size ) {
		send( client_socket, _http_header_bad_request, sizeof(_http_header_bad_request), 0 );
	} else {
		sprintf_s( _content_to_serve_buf, _content_to_serve_buf_size, _http_redirect_template, uri);		
		send( client_socket, _content_to_serve_buf, strlen(_content_to_serve_buf), 0 );
 	}
}

