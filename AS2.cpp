#include "AS.hpp"

using namespace std;

string hostname, port;
int fd_tcp, fd_udp, newfd, errcode;
ssize_t n_tcp, n_udp;
socklen_t addrlen;
string buffer;

int fd_tcp, fd_udp, errcode;
ssize_t n_tcp, n_udp;
socklen_t addrlen;
struct addrinfo hints, *res;
struct sockaddr_in addr;
string hostname, port, ip, input;
char buffer[BUFFERSIZE];
vector<string> inputs;

int main(int argc, char *argv[]) {
  

}