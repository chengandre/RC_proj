#include "AS.hpp"

using namespace std;

int fd_tcp, fd_udp, errcode;
ssize_t n_tcp, n_udp;
socklen_t addrlen;
struct addrinfo hints, *res;
struct sockaddr_in addr;
string hostname, port, ip, input;
char buffer[BUFFERSIZE];
vector<string> inputs;
bool verbose = false;

int main(int argc, char *argv[]) {

    switch(argc) {
        case 3:
            port = argv[2];
            break;
        case 4:
            if (strcmp(argv[1], "-p") == 0) {
                port = argv[2];
            }
            else {
                port = argv[3];
            }
            verbose = true;
            break;
        default:
            port = argv[2];
            break;
    }

    cout << port << '\n';

    fd_udp = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd_udp == -1) {
        exit(1);
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    errcode = getaddrinfo(NULL, port.c_str(), &hints, &res);
    if (errcode != 0) {
        cout << gai_strerror(errcode);
        exit(1);
    }

    n_udp = bind(fd_udp, res->ai_addr, res->ai_addrlen);
    if (n_udp == -1) {
        cout << "bind" << strerror(errno);
        exit(1);
    }
    
    while (1) {
        addrlen = sizeof(addr);
        n_udp = recvfrom(fd_udp, buffer, 128, 0, (struct sockaddr*) &addr, &addrlen);
        if (n_udp == -1) {
            exit(1);
        }

        string message = "Server received: ";
        message += buffer;
        cout << message;
        // write(1, message.c_str(), message.length());
        n_udp = sendto(fd_udp, message.c_str(), message.length(), 0, (struct sockaddr*) &addr, addrlen);
        if (n_udp == -1) {
            exit(1);
        }
    }
}