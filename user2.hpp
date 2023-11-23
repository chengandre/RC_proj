
#include <csignal>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string>
#include <iostream>
#include <vector>

#define DEFAULT_HOSTNAME "localhost"
#define DEFAULT_PORT "58123" //don't know...

enum {
    LOGIN = 1, 
    LOGOUT = 2, 
    UNREGISTER = 3,
    EXIT = 4,
    MYAUCTIONS = 5,
    MYBIDS = 6,
    LIST = 7,
    SHOW_RECORD = 8,
    OPEN = 9,
    CLOSE = 10,
    SHOW_ASSET = 11,
    BID = 12
};

class UserChars {
 public:
  char* program_path;
  std::string host = DEFAULT_HOSTNAME;
  std::string port = DEFAULT_PORT;
  UserChars(int argc, char* argv[]);
};