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
#include <string.h>
#include <sstream>
#include <algorithm>
#include <fstream>

#define DEFAULT_HOSTNAME "loscalhost"
#define DEFAULT_PORT "58123" //don't know...
#define BUFFERSIZE 128

enum {
    EXIT = 0,
    LOGIN = 1, 
    LOGOUT = 2, 
    UNREGISTER = 3,
    MYAUCTIONS = 4,
    MYBIDS = 5,
    LIST = 6,
    SHOW_RECORD = 7,
    OPEN = 8,
    CLOSE = 9,
    SHOW_ASSET = 10,
    BID = 11
};