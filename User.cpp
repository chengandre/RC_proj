// handle signal child
// remove exit(1)
// while to send and receive messages through socket, so that \sum n = n_total;
#include "User.hpp"
using namespace std;

int fd_tcp, fd_udp, errcode;
ssize_t n_tcp, n_udp;
socklen_t addrlen;
struct addrinfo hints, *res;
struct sockaddr_in addr;
string hostname, port, ip, input;
char buffer[BUFFERSIZE];
vector<string> inputs, userInfo;
bool loggedIn = false;
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

int checkName(string &name) {
    return all_of(name.begin(), name.end(), ::isalnum) && name.length() <= 10;
}

int checkPrice(string &price) {
    return all_of(price.begin(), price.end(), ::isdigit);
}

int checkTime(string &time) {
    return all_of(time.begin(), time.end(), ::isdigit);
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

                vector<string> response;
                string tmp_buffer(buffer);
                parseInput(tmp_buffer, response);
                if (response[1] == "OK") {
                    loggedIn = true;
                    userInfo.push_back(arguments[1]);
                    userInfo.push_back(arguments[2]);
                }
                // check if logged in successfully, if so change bool
                return 0; // dunno what else to return
            } else {
                cout << "Syntax error" << endl;
            }
            break;
        case LOGOUT:
            if (!loggedIn) {
                cout << "User not logged in\n";
                return 0;
            }
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
    fd_tcp = socket(AF_INET, SOCK_STREAM, 0);
    if (fd_tcp == -1) {
        return fd_tcp;
    }

    n_tcp = connect(fd_tcp, res->ai_addr, res->ai_addrlen);
    if (n_tcp == -1) {
        return fd_tcp;
    }

    n_tcp = write(fd_tcp, message.c_str(), size);
    if (n_tcp == -1) exit(1);

    // ssize_t total = 0;
    // ssize_t sent;
    // while (sent < size) {
    //     sent = write(fd_tcp, message.c_str() + total, size - total);
    //     if (sent == -1) {
    //         return sent; // error
    //     }

    //     total += sent;
    // }

    int n = read(fd_tcp, buffer, BUFFERSIZE);
    
    return n_tcp;
}

string openJPG(string fname) {
    ifstream fin(fname, ios::binary);
    ostringstream oss;
    oss << fin.rdbuf();
    return oss.str();
}

int handleTCPRequest(int request, vector<string> inputs) {
    cout << "Handling TCP Request" << endl;
    int n;
    string message, tmp;
    switch (request) {
        case OPEN:
            // open name asset_fname start_value timeactive
            userInfo.push_back("123456");
            userInfo.push_back("12345678");
            message = "OPA " + userInfo[0] + " " + userInfo[1] + " " + inputs[1] + " ";
            message += inputs[3] + " " + inputs[4] + " " + inputs[2] + " ";
            tmp = openJPG(inputs[2]);
            message += to_string(tmp.size()) + " ";
            message += openJPG(inputs[2]) + "\n";
            n = sendReceiveTCPRequest(message, message.length());
            break;
        case CLOSE:
            message = "CLS" + userInfo[0] + " " + userInfo[1] + " " + inputs[1] + "\n";
            n = sendReceiveTCPRequest(message, message.length());
            break;
        case SHOW_ASSET:
            message =  "SAS " + inputs[1] + "\n";
            n = sendReceiveTCPRequest(message, message.length());
            // save the image 
            break;
        case BID:
            message = "BID " + userInfo[0] + " " + userInfo[1] + " " + inputs[1] + " " + inputs[2] + "\n";
            n = sendReceiveTCPRequest(message, message.length());
            break;
        default:
            cout << "Not possible" << endl;
            break;
    }
    return n;
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