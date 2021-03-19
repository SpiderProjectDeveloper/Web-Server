#pragma once

#define MIME_BUF_SIZE 80
void set_mime_type(char *fn, char *mime_buf, int mime_buf_size);

int get_uri_to_serve(char *b, char *uri_buf, int uri_buf_size, bool *is_get, char *get_buf, int get_buf_size, char **post, bool *is_options);

int get_user_and_session_from_cookie( char *b, char *user_buf, int user_buf_size, char *sess_id_buf, int sess_id_buf_size  );

int get_user_and_pass_from_post( char *b, char *user_buf, int user_buf_size, char *pass_buf, int pass_buf_size );

bool is_html_request( char *uri );

int decode_uri( char *uri_encoded, char *uri, int buf_size );

template<class... Args>
void error_message( Args... args ) {
  return;
	///*
  //#ifndef __DEV__
	//	return;
	//#endif
	//(std::cout << ... << args) << std::endl;
	std::fstream log_file("c:\\Users\\1395262\\Desktop\\sava\\spider\\serverweb\\log.txt", std::fstream::out | std::fstream::app);
	if( log_file ) {
			(log_file << ... << args) << std::endl;
		log_file.close();
	}
	//*/
}
