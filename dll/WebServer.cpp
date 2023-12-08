#include <string>
//#include <iostream>
//#include <thread>
//#include "Globals.hpp"
#include <iostream>
#include <windows.h>
#include "WebServer.hpp"

#ifndef UNICODE
#define UNICODE
#endif 

static SERVER_DLL_START p_server_start;

using namespace std;

char _callback_error_code;
#define RESPONSE_BUFFER 100000
char _callback_response[RESPONSE_BUFFER+1];

int callback ( ServerData *sd ) {

  if( sd->message_id == SERVER_NOTIFICATION_MESSAGE ) { // Simply to inform SP about smth.
    printf("********\n%s\n", sd->message);
    return 0;
  } else {
	printf("********\nA MESSAGE FROM SERVER TO SP:\n");
    printf("ID: %d\n", sd->message_id);
	if( sd->user != nullptr ) {	
    	printf("USER: %s\n", sd->user);
	} else {
    	printf("USER: nullptr\n");
	}
	if( sd->message != nullptr ) {	
    	printf("MESSAGE: %s\n", sd->message);
	}
  }

  _callback_error_code = 0;
  sd->sp_response_buf = _callback_response;
  sd->sp_free_response_buf = false;
  sd->sp_response_is_file = false;

  if( sd->message_id == SERVER_LOGIN ) {
    strcpy( sd->sp_response_buf, "1" );
    sd->sp_response_buf_size = strlen(sd->sp_response_buf);
    _callback_error_code = 0;
  }
  else if ( sd->message_id == SERVER_GET_CONTENTS ) {
    strcpy( sd->sp_response_buf,
        "{ \"Lang\" : \"ru\", \"Projects\" : [ \"$СП_Лосево-Павловск.210.sprj\", \"$ЦКАД4_УЧ9_СУ920.176.sprj\", \"000000116.153.раб.001.sprj\", \"002020.душ.баз.001.sprj\"], \"Actions\" : [ { \"Id\": \"OpenGanttOper\", \"Name\": \"Открыть Гантт Работ\" }, { \"Id\": \"OpenDashboard\",  \"Name\": \"Открыть Дэшборд\" } ] }" );
    sd->sp_response_buf_size = strlen(sd->sp_response_buf);
    _callback_error_code = 0;
  }
  else if( sd->message_id == SERVER_CHECK_GANTT_SYNCHRO ) {
    strcpy( sd->sp_response_buf, "1" );   // "1" - synchronized, "0" - not sync., must reload
    sd->sp_response_buf_size = strlen(sd->sp_response_buf);
    _callback_error_code = 0;
  }
  else if( sd->message_id == SERVER_GET_GANTT ) {
    strcpy( sd->sp_response_buf, "html/tempdata/gantt.json" );
    sd->sp_response_buf_size = strlen(sd->sp_response_buf);
	sd->sp_response_is_file = true;
    _callback_error_code = 0;
  }
  else if( sd->message_id == SERVER_SAVE_GANTT ) {
    // "0" - saved ok, no error, "1" - saved ok, must reload, "10", "11", "12" etc - not saved
    cerr << "SAVE GANTT: " << endl << sd->message << endl;
    strcpy( sd->sp_response_buf, "{\"errorCode\":0, \"errorMessage\":\"Ok\"}" );
    sd->sp_response_buf_size = strlen(sd->sp_response_buf);
    _callback_error_code = 0;
  }
  else if( sd->message_id == SERVER_CHECK_INPUT_SYNCHRO ) {
    strcpy( sd->sp_response_buf, "1" );   // "1" - synchronized, "0" - not sync., must reload
    sd->sp_response_buf_size = strlen(sd->sp_response_buf);
    _callback_error_code = 0;
  }
  else if( sd->message_id == SERVER_GET_INPUT ) {
    strcpy( sd->sp_response_buf, "html/tempdata/input.json" );
    sd->sp_response_buf_size = strlen(sd->sp_response_buf);
	sd->sp_response_is_file = true;
    _callback_error_code = 0;
  }
  else if( sd->message_id == SERVER_SAVE_INPUT ) {
    // "0" - saved ok, no error, "1" - saved ok, must reload, "10", "11", "12" etc - not saved
    cerr << "SAVE INPUT: " << endl << sd->message << endl;
    strcpy( sd->sp_response_buf, "{\"errorCode\":0, \"errorMessage\":\"Ok\"}" );
    sd->sp_response_buf_size = strlen(sd->sp_response_buf);
    _callback_error_code = 0;
  }
  else if( sd->message_id == SERVER_SAVE_IMAGE ) {
    strcpy( sd->sp_response_buf, "{\"errorCode\":0, \"errorMessage\":\"Ok\"}" );
    sd->sp_response_buf_size = strlen(sd->sp_response_buf);
    _callback_error_code = 0;
  }
  else if( sd->message_id == SERVER_GET_IMAGE ) {
    // "0" - saved ok, no error, "1" - saved ok, must reload, "10", "11", "12" etc - not saved
    cerr << "SERVING A FILE: " << endl << sd->message << endl;

    FILE *fileptr = fopen("main\\image.jpg", "rb");
    size_t filesize=0;
    size_t blocksread=0;
    if( fileptr ) {
      fseek(fileptr, 0L, SEEK_END);
      filesize = ftell(fileptr);
      if( filesize <= RESPONSE_BUFFER ) {
        fseek(fileptr, 0L, SEEK_SET);
        blocksread = fread( sd->sp_response_buf, filesize, 1, fileptr);
      }
      fclose(fileptr);
    }
    if( blocksread == 1 ) {
      sd->sp_response_buf_size = filesize;
      _callback_error_code = 0;
    } else {
      sd->sp_response_buf[0] = '\x0';
      sd->sp_response_buf_size = 0;
      _callback_error_code = -1;
    }
  }
  else {
    sd->sp_response_buf[0] = '\x0';
    sd->sp_response_buf_size = 0;
    _callback_error_code = -1;
  }
  return _callback_error_code;
}

static StartServerData Data;

//int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR pCmdLine, int nCmdShow)
int main( void )
{
    HINSTANCE hServerDLL;

    Data.IP = "127.0.0.1";
    Data.Port = "8000";
    Data.ExePath = nullptr;
    Data.HtmlPath = "html\\";

    hServerDLL = LoadLibrary("serverweb");
    if (hServerDLL != NULL)
    {
        std::cout << "Starting!" << std::endl;
        Data.Message = ssd_Start;
        p_server_start = (SERVER_DLL_START) GetProcAddress (hServerDLL, "start");

        if (p_server_start != NULL) {
            //int (*callback_ptr)(ServerData *) = callback;
            //cerr << "Server is about to start!" << endl;

            p_server_start (&Data, callback);
            //MessageBoxW( NULL, L"Press \"STOP\" to stop the server...", L"SP-Server", MB_OK );
            cerr << "The server has started! Press <ENTER> to stop the server..."  << endl;
            cin.get();
            Data.Message = ssd_Stop;
            p_server_start (&Data, callback);
            cerr << "The server is stopped! Press <ENTER> to exit the program..."  << endl;
            cin.get();
    } else {
      cerr << "The server has not started!" << endl;
    }
    FreeLibrary(hServerDLL);
  }
   //return 0;
}
