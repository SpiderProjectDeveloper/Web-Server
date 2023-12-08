#include "server.h"

char* server_login( ServerDataWrapper& sdw, char *user, char *pass, callback_ptr callback ) {
	try {
			sdw.sd.user = user;
			sdw.sd.message_id = SERVER_LOGIN;
			sdw.sd.message = pass;
			int callback_return = callback( &sdw.sd );
			if( sdw.sd.sp_response_buf != nullptr && callback_return >= 0 && sdw.sd.sp_response_buf_size > 0 ) {
				if( sdw.sd.sp_response_buf[0] == '0' && strlen(sdw.sd.sp_response_buf) < 4 ) {
					return nullptr;
				}
				return sdw.sd.sp_response_buf;
			}
	} catch (...) {
	}
	return nullptr;
}

bool server_is_logged( 
	ServerDataWrapper& sdw, 
	char *sess_id, callback_ptr callback, 
	bool is_update_session ) 
{
	try {
			sdw.sd.message_id = SERVER_IS_LOGGED;
			sdw.sd.sess_id = sess_id;
			// error_message( "server_auth sess_id: ", sess_id );
			int callback_return = callback( &sdw.sd );
			// if( sdw.sd.sp_response_buf != nullptr ) error_message( "sdw.sd.sp_response_buf: ", sdw.sd.sp_response_buf );
			if( sdw.sd.sp_response_buf != nullptr && callback_return >= 0 && sdw.sd.sp_response_buf_size > 0 ) 
			{
				if( sdw.sd.sp_response_buf[0] == '0' ) {
					return false;
				}
				return true;
			}
	} catch (...) {
	}
	return false;
}

bool server_logout( ServerDataWrapper& sdw, char *sess_id, callback_ptr callback ) {
	try {
			sdw.sd.message_id = SERVER_LOGOUT;
			sdw.sd.sess_id = sess_id;
			int callback_return = callback( &sdw.sd );
			if( sdw.sd.sp_response_buf != nullptr && callback_return >= 0 && sdw.sd.sp_response_buf_size > 0 ) {
				if( sdw.sd.sp_response_buf[0] == '0' ) {
					return false;
				}
				return true;
			}
	} catch (...) {;}
	return false;
}