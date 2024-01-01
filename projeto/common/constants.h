#define MAX_RESERVATION_SIZE 256
#define STATE_ACCESS_DELAY_US 500000  // 500ms
#define MAX_JOB_FILE_NAME_SIZE 256
#define MAX_SESSION_COUNT 8

// Begin of the changes

#define MAX_PIPE_NAME 40

#define SETUP_CODE '1'
#define QUIT_CODE '2'
#define CREATE_CODE '3'
#define RESERVE_CODE '4'
#define SHOW_CODE '5'
#define LIST_CODE '6'

#define SERVER_PIPE_MODE 0777
#define REQ_PIPE_MODE 0777
#define RESP_PIPE_MODE 0777