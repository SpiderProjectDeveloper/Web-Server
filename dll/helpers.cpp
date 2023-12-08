#include <string>
#include <iostream>
#include <fstream>
#include <windows.h>
#include <shlwapi.h>
#include <string.h>
#include "helpers.h"                                                        

void set_mime_type(char *fn, char *mime_buf, int mime_buf_size) {
	int l = strlen(fn);
	if( l > 4 && 
		(tolower(fn[l-1]) == 's' && tolower(fn[l-2]) == 's' && tolower(fn[l-3]) == 'c' && fn[l-4] == '.')) 
	{
		strcpy(mime_buf, "text/css; charset=utf-8"); 	// .css
	} 
	else if( l > 5 && 
		(tolower(fn[l-1]) == 'n' && tolower(fn[l-2]) == 'o' && tolower(fn[l-3]) == 's' && 
		tolower(fn[l-4]) == 'j' && fn[l-5] == '.') ) 
	{
		strcpy(mime_buf, "text/json; charset=utf-8"); 	// .json
	} 
	else if( l > 5 && 
		(tolower(fn[l-1]) == 'g' && tolower(fn[l-2]) == 'e' && tolower(fn[l-3]) == 'p' && 
		tolower(fn[l-4]) == 'j' && fn[l-5] == '.') ) 
	{
		strcpy(mime_buf, "image/jpeg"); 	// .jpeg
	} 
	else if( l > 4 && 
		(tolower(fn[l-1]) == 'g' && tolower(fn[l-2]) == 'p' && tolower(fn[l-3]) == 'j' && fn[l-4] == '.') ) 
	{
		strcpy(mime_buf, "image/jpeg"); 	// .jpg
	} 
	else if( l > 4 && 
		(tolower(fn[l-1]) == 'g' && tolower(fn[l-2]) == 'n' && tolower(fn[l-3]) == 'p' && fn[l-4] == '.') ) 
	{
		strcpy(mime_buf, "image/png"); 	// .jpg
	} 
	else if( l > 4 && 
		(tolower(fn[l-1]) == 'f' && tolower(fn[l-2]) == 'i' && tolower(fn[l-3]) == 'g' && fn[l-4] == '.') ) 
	{
		strcpy(mime_buf, "image/gif"); 	// .gif
	} 
	else if( l > 5 && 
		(tolower(fn[l-1]) == 'f' && tolower(fn[l-2]) == 'f' && tolower(fn[l-3]) == 'i' && 
		tolower(fn[l-4]) == 't' && fn[l-5] == '.') ) 
	{
		strcpy(mime_buf, "image/tiff"); 	// .tiff
	} 
	else if( l > 4 && 
		(tolower(fn[l-1]) == 'f' && tolower(fn[l-2]) == 'i' && tolower(fn[l-3]) == 't' && fn[l-4] == '.') ) 
	{
		strcpy(mime_buf, "image/tiff"); 	// .tiff
	} 
	else if( l > 4 && 
		(tolower(fn[l-1]) == 'o' && tolower(fn[l-2]) == 'c' && tolower(fn[l-3]) == 'i'  && fn[l-4] == '.') ) 
	{
		strcpy(mime_buf, "image/x-icon"); 	// .ico
	} else if( 
		l > 4 && (tolower(fn[l-1]) == 'm' && tolower(fn[l-2]) == 't' && tolower(fn[l-3]) == 'h' && fn[l-4] == '.') ) 
	{
		strcpy(mime_buf, "text/html; charset=utf-8"); 	// .html
	} 
	else if( l > 4 && 
		(tolower(fn[l-1]) == 'l' && tolower(fn[l-2]) == 'm' && tolower(fn[l-3]) == 't' && 
		tolower(fn[l-4]) == 'h' && fn[l-5] == '.' ) ) 
	{
		strcpy(mime_buf, "text/html; charset=utf-8"); 	// .html
	} 
	else if( l > 4 && 
		(tolower(fn[l-1]) == 't' && tolower(fn[l-2]) == 'x' && tolower(fn[l-3]) == 't' && fn[l-4] == '.') ) 
	{
		strcpy(mime_buf, "text/plain; charset=utf-8"); 	// .txt
	}
	else if( l > 5 && 
		(tolower(fn[l-1]) == 'm' && tolower(fn[l-2]) == 's' && tolower(fn[l-3]) == 'a' && 
		tolower(fn[l-4]) == 'w' && fn[l-5] == '.') ) 
	{
		strcpy(mime_buf, "application/wasm"); 	// .json
	}  
	else 
	{
		strcpy(mime_buf, "text/html; charset=utf-8");
	}
}

void create_cookie( char *sessId, char *user, char *cookie_buf, unsigned int cookie_buf_size ) {
	if (sessId == nullptr) {
		sprintf(cookie_buf, "Set-Cookie: sessid=deleted; path=/; expires=Thu, 01 Jan 1970 00:00:00 GMT\r\nSet-Cookie: user=Not Authorized; path=/; expires=Thu, 01 Jan 1970 00:00:00 GMT\r\n");
	}
	else {
		if( strlen(user) == 0 ) {
			sprintf(cookie_buf, "Set-Cookie: sessid=%s; path=/;\r\n", sessId);
		} else {
			sprintf(cookie_buf, "Set-Cookie: sessid=%s; path=/;\r\nSet-Cookie: user=%s; path=/;\r\n", sessId, user);
		}
	}
}


int get_uri_to_serve(char *b, char *uri_buf, int uri_buf_size, bool *is_get, char *get_buf, int get_buf_size, char **post, bool *is_options) {
	int b_len = strlen(b);

	*is_get = false;
	*post = nullptr;
	*is_options = false;

	// Searching for "GET", "POST" or "OPTIONS"
	int uri_index = -1;

	int i = 0; 	// To skip leading spaces if exist...
	for( ; i < b_len ; i++) {
		if( b[i] != ' ' ) {
			break;
		}
	}

	if( i < b_len - 4 ) {	// Is it a GET?
		if (tolower(b[i]) == 'g' && tolower(b[i + 1]) == 'e' && tolower(b[i + 2]) == 't') {
			uri_index = i+3;
			*is_get = true;
			get_buf[0] = '\x0';
		}
	}
	if( *is_get == false && i < b_len - 4 ) { 	// Is it a POST?
		if (tolower(b[i]) == 'p' && tolower(b[i + 1]) == 'o' && tolower(b[i + 2]) == 's' && tolower(b[i + 3]) == 't') {
			uri_index = i+4;
			int post_index = -1;
			for (int j = i+4; j < b_len - 4; j++) {
				if (tolower(b[j]) == '\r' && tolower(b[j+1]) == '\n' && tolower(b[j+2]) == '\r' && tolower(b[j+3]) == '\n') {
					post_index = j + 4;
					*post = &b[post_index];
					break;
				}
			}
			if( post_index == -1 ) { 	// A POST request, but no post data...
				return -1;
			}
		}
	}
	if( *is_get == false && *post == nullptr && i < b_len - 7 ) { 	// Is it an OPTIONS?
		if (tolower(b[i]) == 'o' && tolower(b[i+1]) == 'p' && tolower(b[i+2]) == 't' && tolower(b[i+3]) == 'i' &&
			tolower(b[i+4]) == 'o' && tolower(b[i+5]) == 'n' && tolower(b[i+6]) == 's' ) {
			uri_index = i+7;
			*is_options = true;
		}
	}
	if( uri_index == -1 ) {
		return -1;
	}

	int first_uri_index = -1;
	int last_uri_index = -1;
	for (int i = uri_index; i < b_len - 1; i++) {
		if (b[i] == ' ') {
			continue;
		}
		if (b[i] == '/') {
			first_uri_index = i;
			break;
		}
	}
	if (first_uri_index != -1) {
		int last_uri_index = first_uri_index + 1;
		for (; last_uri_index < b_len; last_uri_index++) {
			if( b[last_uri_index] == '?' ) { 	// A uri with get request				
				for( int j = last_uri_index+1 ; j < b_len ; j++ ) {
					if( b[j] == ' ' || b[j] == '\r' || b[j] == '\n' ) {
						int get_request_len = j-last_uri_index-1;
						if( get_request_len > 0 && get_request_len < get_buf_size ) {
							strncpy( get_buf, &b[last_uri_index+1], get_request_len );
							get_buf[get_request_len] = '\x0';
							break;
						}
					}
				}
				break;			
			}
			if (b[last_uri_index] == ' ' || b[last_uri_index] == '\r' || b[last_uri_index] == '\n' ) {
				break;
			}
		}
		int uri_len = last_uri_index - first_uri_index;
		if (uri_len < uri_buf_size) {
			strncpy_s(uri_buf, uri_buf_size, &b[first_uri_index], uri_len);
			uri_buf[uri_len] = '\x0';
			return 0;
		}
	}
	return -1;
}


bool is_html_request( char *uri ) {

	int l = strlen(uri);
	if (l > 4 && (tolower(uri[l-1]) == 'm' && tolower(uri[l-2]) == 't' && tolower(uri[l-3]) == 'h' && uri[l-4] == '.')) {
		return true;
	}
	if( (l > 5 && tolower(uri[l-1]) == 'l' && tolower(uri[l-2]) == 'm' && tolower(uri[l-3]) == 't' && tolower(uri[l-4]) == 'h' && uri[l-5] == '.') ) {
		return true;
	}

	return false;
}


int get_user_and_session_from_cookie( char *b, char *user_buf, int user_buf_size, char *sess_id_buf, int sess_id_buf_size ) 
{
	int r = 0; 
	int b_len = strlen(b);

	sess_id_buf[0] = '\x0';
	user_buf[0] = '\x0';
	
	int cookie_index = -1;
	for (int i = 0; i < b_len - 8; i++) {
		if (tolower(b[i]) == 'c' && tolower(b[i + 1]) == 'o' && tolower(b[i + 2]) == 'o' &&
			tolower(b[i + 3]) == 'k' && tolower(b[i + 4]) == 'i' && tolower(b[i + 5]) == 'e' && b[i + 6] == ':') {
			cookie_index = i + 7;
			break;
		}
	}
	int session_index = -1;
	if (cookie_index != -1) { 	// If "Cookie:" found...
		for (int i = cookie_index; i < b_len - 9; i++) {
			if (tolower(b[i]) == 's' && tolower(b[i + 1]) == 'e' && tolower(b[i + 2]) == 's' && tolower(b[i + 3]) == 's' && 
				tolower(b[i + 4]) == '_' && tolower(b[i + 5]) == 'i' && tolower(b[i + 6]) == 'd' && b[i + 7] == '=') {
				session_index = i + 8;
				break;
			}
		}
	}
	int session_ending_index = -1;
	if (session_index != -1) {
		for (int i = session_index; i < b_len; i++) {
			if (b[i] == ' ' || b[i] == ';' || b[i] == ',' || b[i] == '\r' || b[i] == '\n') {
				session_ending_index = i - 1;
				break;
			}
		}
	}

	int l = session_ending_index - session_index + 1;
	if (l > 0) {
		if( l > sess_id_buf_size ) {
			l = sess_id_buf_size;
		}
		for ( int i = 0 ; i < l ; i++) {
			sess_id_buf[i] = b[session_index + i];
		}
		r++;
		sess_id_buf[l] = '\x0';
	}

	int user_index = -1;
	if (cookie_index != -1) { 	// If "Cookie:" found...
		for (int i = cookie_index; i < b_len - 6; i++) {
			if (tolower(b[i]) == 'u' && tolower(b[i + 1]) == 's' && tolower(b[i + 2]) == 'e' && tolower(b[i + 3]) == 'r' && tolower(b[i + 4]) == '=' ) {
				user_index = i + 5;
				break;
			}
		}
	}
	int user_ending_index = -1;
	if (user_index != -1) {
		for (int i = user_index; i < b_len; i++) {
			if (b[i] == ' ' || b[i] == ';' || b[i] == ',' || b[i] == '\r' || b[i] == '\n') {
				user_ending_index = i - 1;
				break;
			}
		}
	}

	l = user_ending_index - user_index + 1;
	if (l > 0) {
		if( l > user_buf_size ) {
			l = user_buf_size;
		}
		for ( int i = 0 ; i < l ; i++) {
			user_buf[i] = b[user_index + i];
		}
		r++;
		user_buf[l] = '\x0';
	}
		
	return r;
}


int get_user_and_pass_from_post( char *b, char *user_buf, int user_buf_size, char *pass_buf, int pass_buf_size ) 
{
	int r = 0;

	int b_len = strlen(b);
	
	user_buf[0] = '\x0';
	int user_index = -1;
	for( int i = 0 ; i < b_len ; i++ ) {
		if( tolower(b[i]) == 'u' && tolower(b[i+1]) == 's' && tolower(b[i+2]) == 'e' && 
			tolower(b[i+3]) == 'r' && tolower(b[i+4]) == '=' ) {
			user_index = i+5;
			break;
		}
	}		
	if( user_index != -1 ) {
		int user_i = 0;
		for( int i = user_index ; i < b_len && user_i < user_buf_size ; i++ ) {
			if( b[i] == '&' || b[i] == ' ' ) {
				break;
			}
			user_buf[user_i] = b[i];
			user_i++;
		}
		user_buf[user_i]='\x0';
		r++;
	}
	pass_buf[0] = '\x0';
	int pass_index = -1;
	for( int i = 0 ; i < b_len ; i++ ) {
		if( tolower(b[i]) == 'p' && tolower(b[i+1]) == 'a' && tolower(b[i+2]) == 's' && 
			tolower(b[i+3]) == 's' && tolower(b[i+4]) == '=' ) {
			pass_index = i+5;
			break;
		}
	}		
	if( pass_index != -1 ) {
		int pass_i = 0;
		for( int i = pass_index ; i < b_len && pass_i < pass_buf_size ; i++ ) {
			if( b[i] == '&' || b[i] == ' ' ) {
				break;
			}
			pass_buf[pass_i] = b[i];
			pass_i++;
		}
		pass_buf[pass_i]='\x0';
		r++;
	}
	return r;
}


bool get_session_from_uri( char *b, char *sess_id_buf, int sess_id_buf_size ) 
{
	int r = 0; 
	int b_len = strlen(b);

	sess_id_buf[0] = '\x0';
	
	int session_index = -1;
	for (int i = 0; i < b_len - 8; i++) 
	{
		bool isSessId = 
			tolower(b[i]) == 's' && tolower(b[i + 1]) == 'e' && tolower(b[i + 2]) == 's' && 
			tolower(b[i + 3]) == 's' && tolower(b[i + 4]) == 'i' && 
			tolower(b[i + 5]) == 'd' && tolower(b[i + 6]) == '=';
		if( isSessId ) {
			session_index = i + 7;
			break;
		}
	}
	//error_message( "session_index: ", session_index );
	if( session_index == -1 ) return false;

	int session_ending_index = session_index;
	for( int i = session_index+1; i < b_len; i++ ) {
		if( b[i] == '&' || b[i] == ' ' ) break;
		session_ending_index++;
	}
	//error_message( "session_ending_index: ", session_ending_index );

	int l = session_ending_index - session_index + 1;
	if( l <= 0 ) return false;
	if( l > sess_id_buf_size ) {
		l = sess_id_buf_size;
	}
	for( int i = 0 ; i < l ; i++ ) {
		sess_id_buf[i] = b[session_index + i];
	}
	sess_id_buf[l] = '\x0';

	return true;
}


bool get_session_from_referer( char *b, char *sess_id_buf, int sess_id_buf_size ) 
{
	int r = 0; 
	int b_len = strlen(b);

	sess_id_buf[0] = '\x0';
	
	int referer_index = -1;
	for (int i = 0; i < b_len - 9; i++) {
		bool isReferer = tolower(b[i]) == 'r' && tolower(b[i + 1]) == 'e' && tolower(b[i + 2]) == 'f' &&
			tolower(b[i + 3]) == 'e' && tolower(b[i + 4]) == 'r' && tolower(b[i + 5]) == 'e' && 
			tolower(b[i + 6]) == 'r' && b[i + 7] == ':';
		if( isReferer ) {
			referer_index = i + 8;
			break;
		}
	}
	if( referer_index == -1 ) return false;

	int session_index = -1;	
	for (int i = referer_index; i < b_len - 8; i++) 
	{
		bool isSessId = tolower(b[i]) == 's' && tolower(b[i + 1]) == 'e' && 
			tolower(b[i + 2]) == 's' && tolower(b[i + 3]) == 's' && 
			tolower(b[i + 4]) == 'i' && tolower(b[i + 5]) == 'd' && b[i + 6] == '=';
		if( isSessId  ) {
			session_index = i + 7;
			break;
		}
	}
	if( session_index == -1 ) return false;

	int session_ending_index = session_index;
	for( int i = session_index+1; i < b_len; i++ ) {
		if( b[i] == '&' || b[i] == ' ' || b[i] == '\r' || b[i] == '\n') break;
		session_ending_index++;
	}

	int l = session_ending_index - session_index + 1;
	if( l <= 0 ) return false;
	if( l > sess_id_buf_size ) {
		l = sess_id_buf_size;
	}
	for( int i = 0 ; i < l ; i++ ) {
		sess_id_buf[i] = b[session_index + i];
	}
	sess_id_buf[l] = '\x0';

	return true;
}



int decode_uri( char *uri_encoded, char *uri, int buf_size ) {
	DWORD size = buf_size;
	HRESULT hres = UrlUnescapeA( uri_encoded, uri, &size, 0 );
	if( hres != S_OK ) {
		return -1;
	}
	return 0;
} 


int get_content_read( char *b, int b_len ) {
	for( int i = 0; i < b_len - 3 ; i++ ) {
		if( (b[i] == '\r' || b[i] == '\n') && (b[i+1] == '\r' || b[i+1] == '\n') &&
	        (b[i+2] == '\r' || b[i+2] == '\n') && (b[i+3] == '\r' || b[i+3] == '\n') )
		{
			return b_len-i-4;
		}
	}
	return -1;
}


int get_content_length( char *b, int b_len ) {
	if( b_len < 15 ) {
		return -1;
	}
	int b_max_index_for_content_length = b_len-15;
	int i = 0;
	for( ; i < b_max_index_for_content_length ; i++ ) {
		if( (b[i] == 'C' || b[i] == 'c') &&  (b[i+1] == 'O' || b[i+1] == 'o') &&  (b[i+2] == 'N' || b[i+2] == 'n') && 
			(b[i+3] == 'T' || b[i+3] == 't') && (b[i+4] == 'E' || b[i+4] == 'e') &&  (b[i+5] == 'N' || b[i+5] == 'n') && 
			(b[i+6] == 'T' || b[i+6] == 't') && (b[i+7] == '-') &&  
			(b[i+8] == 'L' || b[i+8] == 'l') && (b[i+9] == 'E' || b[i+9] == 'e') &&  (b[i+10] == 'N' || b[i+10] == 'n') && 
			(b[i+11] == 'G' || b[i+11] == 'g') && (b[i+12] == 'T' || b[i+12] == 't') && (b[i+13] == 'H' || b[i+13] == 'h') && 
			(b[i+14] == ':' ) ) 
		{
			break;
		}
	}
	if( !(i < b_max_index_for_content_length) ) {
		return -1;
	}
	
	int content_length=0;
	for( i = i+15 ; i < b_len ; i++ ) { 	// Skipping space(s) if exists
		if( b[i] == '\r' || b[i] == '\n' )
			break;
		int digit;
		switch( b[i] ) {
			case '0': digit=0; break;
			case '1': digit=1; break;
			case '2': digit=2; break;
			case '3': digit=3; break;
			case '4': digit=4; break;
			case '5': digit=5; break;
			case '6': digit=6; break;
			case '7': digit=7; break;
			case '8': digit=8; break;
			case '9': digit=9; break;
			default: digit=-1;
		}
		if( digit >= 0 ) {
			content_length = content_length*10 + digit;
		}
	}	
	return content_length;
}
