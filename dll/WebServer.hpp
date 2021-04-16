#ifndef WEBSERVER_HPP
#define WEBSERVER_HPP

// message_id
#define SERVER_NOTIFICATION_MESSAGE -1

#define SERVER_LOGIN 1          // returns sess id
#define SERVER_IS_LOGGED 2      // returns "0" or "1"
#define SERVER_LOGOUT 3         // returns "0" or "1"
#define SERVER_CLOSE_PROJECT 4

#define SERVER_GET_CONTENTS 10  // get list of actions and list of projects
#define SERVER_GET_DASHBOARD 20
#define SERVER_GET_MODEL 30
#define SERVER_GET_WEXBIM 40
#define SERVER_GET_PROJECT_PROPS 50

#define SERVER_GET_GANTT 100
#define SERVER_CHECK_GANTT_SYNCHRO 110 // question whether gantt changed (bool)
#define SERVER_SAVE_GANTT 150          // comments received from WEB

#define SERVER_GET_INPUT 200
#define SERVER_CHECK_INPUT_SYNCHRO 210
#define SERVER_SAVE_INPUT 250

#define SERVER_SAVE_IMAGE 300
#define SERVER_GET_IMAGE 310
// end message_id

struct ServerData {
  char* user;           // user code (login)
	char *sess_id;
  int message_id;       // id of the message sent to spider
  char* message;        // text of the message sent to spider
  int message_size;     // size of message_size

  char* sp_response_buf;         // spider response
  unsigned long sp_response_buf_size;   // size of the sp_response_buf
  bool sp_free_response_buf;     // server must free memory allocated for sp_response_buf
  bool sp_response_is_file;      // whether response is the full name of a file
};

struct StartServerData {
  char* IP;
  char* Port;
  char* ExePath;  // path to sp.exe
  char* HtmlPath; // path to the root of server JS files (with subfolders "dashboard", "gantt", "input")
  int Message;    // ssd_Start - start server, ssd_Stop - shutdown server and terminate thread
};

// StartServerData.Message
#define ssd_Start 0
#define ssd_Stop  100

typedef int (*callback_ptr) (ServerData*);
typedef int (*SERVER_DLL_START) (StartServerData* data, callback_ptr callback);

#ifdef SERVER_DLL_EXPORT
  extern "C" __declspec(dllexport) int start (StartServerData* data, callback_ptr callback);
#else
  extern "C" __declspec(dllimport) int start (StartServerData* data, callback_ptr callback);
#endif

void StartWebServer ();
void StopWebServer ();

#endif
