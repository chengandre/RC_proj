// handle signal child
// remove exit(1)
#include "User.hpp"

using namespace std;

int fd_tcp, fd_udp, errcode;
ssize_t n_tcp, n_udp;
socklen_t addrlen;
struct addrinfo hints, *res;
struct sockaddr_in addr;
string hostname, port, ip, input;
char buffer[BUFFERSIZE];
vector<string> inputs;
// vector<string> linUIDs; // keep record of the logged in users

void parseInput(string &input, vector<string> &inputs) {
    inputs.clear();

    istringstream stream(input);
    string tmp;

    while (stream >> tmp) {
        inputs.push_back(tmp);
    }
}

int checkUID(string &uid) {
    // 6 digitos
    return all_of(uid.begin(), uid.end(), ::isdigit) && uid.length() == 6;
}

int checkPassword(string &pw) {
    // 8 numeros ou letras
    return all_of(pw.begin(), pw.end(), ::isalnum) && pw.length() == 8;
}

int checkAID(string &aid) {
    return all_of(aid.begin(), aid.end(), ::isdigit) && aid.length() == 3;
}

int parseCommand(string &command) {
    if (command == "login") {
        return LOGIN;
    } else if (command == "logout") {
        return LOGOUT;
    } else if (command == "unregister") {
        return UNREGISTER;
    } else if (command == "exit") {
        return EXIT;
    } else if (command == "open") {
        return OPEN;
    } else if (command == "close") {
        return CLOSE;
    } else if (command == "myauctions" || command == "ma") {
        return MYAUCTIONS;
    } else if (command == "mybids" || command == "mb") {
        return MYBIDS; 
    } else if (command == "list" || command == "l") {
        return LIST;
    } else if (command == "show_asset" || command == "sa") {
        return SHOW_ASSET;
    } else if (command == "bid" || command == "b") {
        return BID;
    } else if (command == "show_record" || command == "sr") {
        return SHOW_RECORD;
    } else {
        return -1;
    }
}

int sendReceiveUDPRequest(string message, int size) {
    // change to read more from server
    int n;
    n = sendto(fd_udp, message.c_str(), size, 0, res->ai_addr, res->ai_addrlen);
    if (n == -1) {
        return n;
    }

    addrlen = sizeof(addrlen);
    n = recvfrom(fd_udp, buffer, BUFFERSIZE, 0, (struct sockaddr*) &addr, &addrlen);
    return n;
}

int handleUDPRequest(int request, vector<string> arguments) {
    int n;
    string message;
    switch (request) {
        case LOGIN:
            if (checkUID(arguments[1]) && checkPassword(arguments[2])) {
                message = "LIN " + arguments[1] + " " + arguments[2] + "\n";
                n = sendReceiveUDPRequest(message, message.length());
                cout << buffer;
                // check if logged in successfully, if so change bool
            } else {
                cout << "Syntax error" << endl;
            }
            break;
        case LOGOUT:
            if (checkUID(arguments[1]) && checkPassword(arguments[2])) {
                message = "LOU " + arguments[1] + " " + arguments[2] + "\n";
                n = sendReceiveUDPRequest(message, message.length());
                cout << buffer;
            } else {
                cout << "Syntax error" << endl;
            }
            break;
        case UNREGISTER:
            if (checkUID(arguments[1]) && checkPassword(arguments[2])) {
                message = "UNR " + arguments[1] + " " + arguments[2] + "\n";
                n = sendReceiveUDPRequest(message, message.length());
                cout << buffer;
            } else {
                cout << "Syntax error" << endl;
            }
            break;
        case EXIT:
            break;
        case MYAUCTIONS:
            if (checkUID(arguments[1])) {
                message = "LMA " + arguments[1] + "\n";
                n = sendReceiveUDPRequest(message, message.length());
                cout << buffer;
            } else {
                cout << "Syntax error" << endl;
            }
            break;
        case MYBIDS:
            if (checkUID(arguments[1])) {
                message = "LMB " + arguments[1] + "\n";
                n = sendReceiveUDPRequest(message, message.length());
                cout << buffer;
            } else {
                cout << "Syntax error" << endl;
            }
            break;
        case LIST:
            message = "LST\n";
            n = sendReceiveUDPRequest(message, message.length());
            cout << buffer;
            break;
        case SHOW_RECORD:
            if (checkAID(arguments[1])) {
                message = "SRC " + arguments[1] + "\n";
                n = sendReceiveUDPRequest(message, message.length());
                cout << buffer;
            } else {
                cout << "Syntax error" << endl;
            }
            break;
        default:
            cout << "Not possible from UDP" << endl;
            break;
    }
    return -1;
}

int sendReceiveTCPRequest(string message, int size) {
    
}

int handleTCPRequest(int request, vector<string> inputs) {
    int n;
    string message;
    switch (request) {
        case OPEN:
            break;
        case CLOSE:
            break;
        case SHOW_ASSET:
            break;
        case BID:
            break;
        default:
            cout << "Not possible" << endl;
            break;
    }
}

int handleTCPRequest(int request, vector<string> arguments) {
    return 0;
}

int main(int argc, char *argv[]) {
    switch(argc){
        case(1):{
            hostname=DEFAULT_HOSTNAME;
            port = DEFAULT_PORT;
            break;
        }
        case(3):{
            if (!strcmp(argv[1],"-p")){
                hostname=DEFAULT_HOSTNAME;
                port= argv[2];

            }
            else{
                hostname= argv[2];
                port = DEFAULT_PORT;
            }
            break;
        }
        default:{
            hostname = argv[2];
            port = argv[4];

        }
    }

    
    fd_tcp = socket(AF_INET, SOCK_STREAM, 0);
    if (fd_tcp == -1) {
        exit(1);
    }

    fd_udp = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd_udp == -1) {
        exit(1);
    }
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    cout << hostname << '\n';
    cout << port << '\n';
    errcode = getaddrinfo(hostname.c_str(), port.c_str(), &hints, &res);
    if (errcode != 0) {
        cout << gai_strerror(errcode);
        exit(1);
    }
    
    // loop to receive commands
    while (1) {
        cout << "> ";
        getline(cin, input);
        parseInput(input, inputs);
        int request = parseCommand(inputs[0]);
        if (request > 0 && request < 8) {
            handleUDPRequest(request, inputs);
        } else if (request >= 8) {
            handleTCPRequest(request, inputs);
        } else if (request == 0) {
            // exit request; check if logged in, logout if so
        } else {
            cout << "No such request\n";
        }


        // // have udp < target, request = parseCommand() with elseif
        // switch(parseCommand(inputs[0])) {
        //     case LOGIN:
        //         handleUDPRequest(LOGIN, inputs);
        //         break;
        //     case LOGOUT:
        //         handleUDPRequest(LOGOUT, inputs);
        //         break;
        //     case UNREGISTER:
        //         handleUDPRequest(UNREGISTER, inputs);
        //     default:
        //         printf("No such command\n");
        //         break;
        // }
    }

    return 0;
}