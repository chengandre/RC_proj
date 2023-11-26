
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
#include <string.h>
#include <sstream>

#define PORT "58030" //number of group:30
#define BUFFERSIZE 65535