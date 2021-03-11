#include <iostream>
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <string>
#include <filesystem>
#include <string.h>
#include "auth.h"

using namespace std;

static int read_sessions(fstream &f);
static void generate_session_id( char *user, char *sess_id, int sess_id_len );
static void sess_buf_reset();
static int auth_create(char *sess_user_name, const char *sess_id);
static int auth_delete(char *user, char *sess_id);
static int auth_confirm_and_update(char *user, char *sess_id, bool b_update_session, bool b_validate_sess_id_only);
static uint64_t timeSinceEpochMillisec();	

static const uint64_t _sess_inactivity = 30*60*1000;

#define AUTH_MAX_FILE_SESSION_PATH 420
static char _sess_file_path[AUTH_MAX_FILE_SESSION_PATH + 2];

const unsigned long _sess_id_size = AUTH_SESS_ID_LEN;
const unsigned long _sess_user_size = AUTH_USER_MAX_LEN;
static char *_sess_id = nullptr;
static char *_sess_user = nullptr;

struct Sess {
	char id[_sess_id_size+1];
	char user[_sess_user_size+1];
	uint64_t time;
};

static const unsigned long int _sess_buf_capacity = 40;
static Sess _sess_buf[_sess_buf_capacity];

// **** Public Section

int auth_set_session_path( char *path, char *name ) {
    if( strlen(path) + strlen(name) > AUTH_MAX_FILE_SESSION_PATH ) {
        return -1;
    }
    if( path != nullptr ) {
        if( strlen(path) > 0 ) {
            strcpy( _sess_file_path, path );
        }  
    }
    strcat( _sess_file_path, name ); 
    return 0;
}


char *auth_get_sess_id( void ) {
	return _sess_id;
}


char *auth_get_user( void ) {
	return _sess_user;
}


bool auth_logout(char *user, char *sess_id) {
	return (auth_delete(user, sess_id) >= 0);
}


bool auth_confirm(char *user, char *sess_id, bool b_update_session, bool b_validate_sess_id_only) {
	return (auth_confirm_and_update(user, sess_id, b_update_session, b_validate_sess_id_only) >= 0);
}


char *auth_create_session_id( char *user, char *pass, int *error_code ) {
	char *r = nullptr;
	
    char sess_id[_sess_id_size+1];
    generate_session_id( user, sess_id, _sess_id_size );
    int status = auth_create( user, sess_id );
    if (status >= 0) {
        r = _sess_id;
    } else if( error_code != nullptr ) {
        *error_code = status; 	// Too many sessions opened or failed to write session file
    }
	return r;
}

// **** Private Section

static const char sid_valid_chars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXWZ0123456789";
static const int sid_valid_chars_num = sizeof(sid_valid_chars)-1;

static void generate_session_id( char *user, char *sess_id, int sess_id_len ) 
{
	unsigned long int user_sum = 0;
	unsigned long int user_mult = 0;
	for( unsigned int i = 0 ; i < strlen(user) ; i++ ) {
		int n = static_cast<int>(user[i]);
		user_sum += n+(i+1);
		user_mult += n*(i+1);
	}
	unsigned long int seed = static_cast<unsigned long>(timeSinceEpochMillisec()) + user_mult + user_sum;
	std::srand(seed);

	for( int i = 0 ; i < sess_id_len ; i++ ) {
		int index = std::rand() % (sid_valid_chars_num-1);
		sess_id[i] = sid_valid_chars[index];
	}
	sess_id[sess_id_len] = '\x0';
}	


static void sess_buf_reset() {
	for( int i = 0 ; i < _sess_buf_capacity ; i++ ) {
		_sess_buf[i].id[0] = '\x0';				
		_sess_buf[i].user[0] = '\x0';				
		_sess_buf[i].time = 0;				
	}
}

static int read_sessions(fstream &f) {
	int r=-1;
	try {
		if (!std::filesystem::exists(_sess_file_path) ) {
			f.open(_sess_file_path, ios::out | ios::binary);
			sess_buf_reset();
			r = 0;
		} else {
			uintmax_t fsize = std::filesystem::file_size(_sess_file_path);
			if (fsize != sizeof(_sess_buf) ) { 	// The session file is distorted?
				f.open(_sess_file_path, ios::out | ios::binary | ios::trunc); 	// Truncating then
				sess_buf_reset();
				r = 0;
			} else {
				f.open(_sess_file_path, ios::in | ios::out | ios::binary);
				if(f) {
					f.read( (char*)&_sess_buf[0], sizeof(_sess_buf) );
					if(f) {
						long int bytes_read = static_cast<long int>(f.gcount());
						if( bytes_read == sizeof(_sess_buf) ) { 	// Bytes read equals session buffer capacity - Ok!
							r = bytes_read;
							for( int i = 0 ; i < _sess_buf_capacity ; i++ ) {
								_sess_buf[i].id[AUTH_SESS_ID_LEN] = '\x0';
								_sess_buf[i].user[AUTH_USER_MAX_LEN] = '\x0';
							}
						} 	
						else { 	// If not - it's an invalid session file
							sess_buf_reset();
							r = 0;
						}
					} 
					else { 	// Again it's an invalid session file or failed to read...
						sess_buf_reset();
						r = 0;
					}
					f.seekg(ios::beg);
				}
			}
		}
	} catch(...) {
		if(f) {		
			f.close();
		}
		r = -1;
	}
	return r;
}


static int auth_create(char *sess_user, const char *sess_id) {
	int r = AUTH_ERROR_TOO_MANY_SESSIONS;
	fstream f;
	_sess_id = nullptr;
	_sess_user = nullptr;

	int status = read_sessions(f);
	if (status >= 0) {
		uint64_t now = timeSinceEpochMillisec();
		int available_index = -1;
		for (int i = 0; i < _sess_buf_capacity; i++) {
			if( strcmp( _sess_buf[i].user, sess_user ) == 0 ) { // Reauth found
				available_index = i;
				break;
			}
		}			
		if( available_index == -1 ) {
			for (int i = 0; i < _sess_buf_capacity; i++) {
				if( (now - _sess_buf[i].time) >= _sess_inactivity ) { // An old session found
					available_index = i;
					break;
				}
			}
		}
		if( available_index != -1 ) {
			strncpy(_sess_buf[ available_index ].id, sess_id, _sess_id_size );
			_sess_buf[ available_index ].id[_sess_id_size] = '\x0';
			strncpy(_sess_buf[ available_index ].user, sess_user, _sess_user_size );
			_sess_buf[ available_index ].user[_sess_user_size] = '\x0';
			_sess_buf[ available_index ].time = now;
			try {
				f.write( (char*)&_sess_buf[0], sizeof(_sess_buf) );
			} catch(...) {
				r = AUTH_ERROR_FAILED_TO_WRITE_SESSION;
			}
			_sess_id = _sess_buf[ available_index ].id;
			_sess_user = _sess_buf[ available_index ].user;
			r = 0;
		}
		f.close();
	}
	return r;
}


static int auth_delete(char *user, char *sess_id) {
	int r = AUTH_UNKNOWN_ERROR;
	fstream f;
	_sess_id = nullptr;
	_sess_user = nullptr;

	int status = read_sessions(f);
	if (status >= 0) {
		if (status == sizeof(_sess_buf)) { 		// 
			for (int i = 0; i < _sess_buf_capacity; i++) {
				if( _sess_buf[i].time == 0 || _sess_buf[i].id[0] == '\x0' || _sess_buf[i].user[0] == '\x0' ) {
					continue;
				}
				if( strcmp( _sess_buf[i].user, user ) != 0 ) {
					continue; 
				}
				if( strcmp( _sess_buf[i].id, sess_id ) != 0 ) {
					continue;
				}
				_sess_buf[i].id[0] = '\x0';
				_sess_buf[i].user[0] = '\x0';
				_sess_buf[i].time = 0;
				try {
					f.write((char*)&_sess_buf[0], sizeof(_sess_buf));
					r = 0;
				} catch(...) {
					r = AUTH_ERROR_FAILED_TO_WRITE_SESSION;
				}
				break;
			}
		}
		f.close();
	}
	return r;
}


static int auth_confirm_and_update(char *user, char *sess_id, bool b_update_session, bool b_validate_sess_id_only) {
	int r = AUTH_UNKNOWN_ERROR;
	_sess_id = nullptr;
	_sess_user = nullptr;

	if( sess_id != nullptr ) {
		fstream f;	
		int status = read_sessions(f);
		if( status >= 0 ) {
			if (status == sizeof(_sess_buf)) { 		// 
				for (int i = 0; i < _sess_buf_capacity; i++) {
					if( _sess_buf[i].time == 0 || _sess_buf[i].id[0] == '\x0' || _sess_buf[i].user[0] == '\x0' ) {
						continue;
					}
					
					bool b_user_ok;
					if( !b_validate_sess_id_only ) {
						b_user_ok = (strcmp( _sess_buf[i].user, user ) == 0);
					} else {
						b_user_ok = true;
					}
					if( b_user_ok && strncmp( _sess_buf[i].id, sess_id, _sess_id_size ) == 0 ) { 
						uint64_t now = timeSinceEpochMillisec();
						if (now - _sess_buf[i].time < _sess_inactivity) {
							if( b_update_session ) {
								_sess_buf[i].time = now;
								try {
									f.write((char*)&_sess_buf[0], sizeof(_sess_buf));
									r = i;
								} catch(...) {
									;
								}
							} else {
								r = i;
							}
							_sess_id = _sess_buf[i].id;
							_sess_user = _sess_buf[i].user;
							break;
						}
					}
				}
			}
			f.close();
		}
	}
	return r;
}

static uint64_t timeSinceEpochMillisec() {
	using namespace std::chrono;
	return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

