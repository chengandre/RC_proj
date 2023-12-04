#include "AS.hpp"
// handle signal child
// verify if ifstream ofstream have opened correctly
// use remove_all to delete everything in a folder
using namespace std;

int fd_tcp, fd_udp, tcp_child, errcode;
ssize_t n_tcp, n_udp;
socklen_t addrlen;
struct addrinfo hints, *res;
struct sockaddr_in addr;
string hostname, port, ip, input;
char buffer[BUFFERSIZE];
vector<string> inputs;
bool verbose = false;
int auction_number = 0;
time_t fulltime;
struct tm *current_time;
string current_time_str;



int parseCommand(string &command) {
    if (command == "LIN") {
        return LOGIN;
    } else if (command == "LOU") {
        return LOGOUT;
    } else if (command == "UNR") {
        return UNREGISTER;
    } else if (command == "OPA") {
        return OPEN;
    } else if (command == "CLS") {
        return CLOSE;
    } else if (command == "LMA") {
        return MYAUCTIONS;
    } else if (command == "LMB") {
        return MYBIDS; 
    } else if (command == "LST") {
        return LIST;
    } else if (command == "SAS") {
        return SHOW_ASSET;
    } else if (command == "BID") {
        return BID;
    } else if (command == "SRC") {
        return SHOW_RECORD;
    } else {
        return -1;
    }
}

void parseInput(string &input, vector<string> &inputs) {
    inputs.clear();

    istringstream stream(input);
    string tmp;

    while (stream >> tmp) {
        inputs.push_back(tmp);
    }
}

void parseInput(char *input, vector<string> &inputs) {
    inputs.clear();

    istringstream stream(input);
    string tmp;

    while (stream >> tmp) {
        inputs.push_back(tmp);
    }
}

int createLogin(string &uid) {
    string loginName;

    if (uid.size() != 6) {
        cout << "[LOG]: Invalid UID on login" << endl;
        return -1;
    }

    loginName = "USERS/" + uid + "/" + uid + "_login.txt";

    ofstream fout(loginName, ios::out);
    if (!fout) {
        cout << "[LOG]: Couldn't create login file" << endl;
        return -1;
    }
    cout << "User " + uid << " logged in" << endl;

    return 0;
}

int Register(string &uid, string &pass) {
    string userDir, userPass;
    int ret;

    userDir = "USERS/" + uid;
    ret = mkdir(userDir.c_str(), 0700);
    if (ret == -1) {
        cout << "[LOG]: Couldn't create user directory" << endl;
        return -1;
    }

    userPass = "USERS/" + uid + "/" + uid + "_pass.txt";
    ofstream fout(userPass, ios::out);
    if (!fout) {
        cout << "[LOG]: Couldn't create user password file" << endl;
        remove_all(userDir);
        return -1;
    }
    fout << pass;
    
    return 0;
}

void startUDP(void) {
    fd_udp = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd_udp == -1) {
        exit(1);
    }

    n_udp = bind(fd_udp, res->ai_addr, res->ai_addrlen);
    if (n_udp == -1) {
        cout << "bind" << strerror(errno);
        exit(1);
    }

    cout << "[LOG]: UDP starting to read from socket" << endl;
    while (1) {
        addrlen = sizeof(addr);
        n_udp = recvfrom(fd_udp, buffer, 128, 0, (struct sockaddr*) &addr, &addrlen);
        if (n_udp == -1) {
            exit(1);
        }

        string message = "Server received: ";
        message += buffer;
        cout << message;

        // handleUDPRequest(message, pid);
        // write(1, message.c_str(), message.length());
        n_udp = sendto(fd_udp, message.c_str(), message.length(), 0, (struct sockaddr*) &addr, addrlen);
        if (n_udp == -1) {
            exit(1);
        }
    }
}

void concatenateString(string &target, char item[], int size) {
    for (int i = 0; i < size; i++) {
        target.push_back(item[i]);
    }
}

int receiveTCPsize(int fd, int size, string &response, pid_t pid) {
    int total_received = 0;
    int n;
    char tmp[128];
    response.clear();
    cout << "[LOG]: " << pid << " Receiving TCP request" << endl;
    while (total_received < size) {
        n = read(fd, tmp, 1);
        if (n == -1) {
            cout << "TCP receive error" << endl;
            break;
        }
        concatenateString(response, tmp, n);
        total_received += n;
    }
    cout << "[LOG]: " << pid << " Received response of size " << total_received << endl;

    return total_received;
}

int receiveTCPspace(int fd, int size, string &response, pid_t pid) {
    int total_received = 0;
    int total_spaces = 0;
    int n;
    char tmp[128];
    response.clear();
    cout << "[LOG]: " << pid << " Receiving TCP request by spaces" << endl;
    while (total_spaces < size) {
        n = read(fd, tmp, 1);
        if (n == -1) {
            cout << "TCP receive error" << endl;
            break;
        }
        concatenateString(response, tmp, n);
        total_received += n;
        if (tmp[0] == ' ') {
            total_spaces++;
        }
    }
    cout << "[LOG]: " << pid << " Received response of size " << total_received << endl;

    return total_received;
}

int receiveTCPimage(int fd, int size, string &fname, string &aid, pid_t pid) {
    int total_received = 0;
    int n;
    char tmp[128];
    string dir = "AUCTIONS/" + aid + "/" + fname;
    ofstream fout(dir, ios::binary);

    while (total_received < size) {
        n = read(fd, tmp, 128);
        if (n == -1) {
            cout << "TCP image receive error" << endl;
            fout.close();
            remove(fname.c_str());
            return -1;
        }
        fout.write(tmp, n);
        total_received += n;
    }
    cout << "[LOG]: " << pid << " Received file of size " << total_received << endl;
    fout.close();
    return total_received;
    // receive image directly into file
}

int sendTCPresponse(int fd, string &message, int size, pid_t &pid) {
    cout << "[LOG]: " << pid << " Sending TCP responde" << endl;
    int total_sent = 0;
    int n;
    while (total_sent < size) {
        n = write(fd_tcp, message.c_str() + total_sent, size - total_sent);
        if (n == -1) {
            cout << "TCP send error" << endl;
            return n;
        }
        total_sent += n;
    }
    cout << "[LOG]: Sent TCP request" << endl;
    return total_sent;
}

bool checkLogin (string &uid) {
    // checks if user is logged in
    struct stat buffer;
    string tmp = "USERS/" + uid + "/" + uid + "_login.txt";
    return (stat (tmp.c_str(), &buffer) == 0); 
}

bool checkPassword(string &uid, string &pw) {
    string fname = "USERS/" + uid + "/" + uid + "_password.txt";

    ifstream pw_file;
    pw_file.open(fname, ios::in);
    
    ostringstream oss;
    oss << pw_file.rdbuf();
    return oss.str() == pw;
}

int createAuctionDir(string &aid) {
    string AID_dirname = "AUCTIONS/" + aid;
    string BIDS_dirname = "AUCTIONS/" + aid + "/BIDS";
    int ret;

    ret = mkdir(AID_dirname.c_str(), 0700);
    if (ret == -1) {
        cout << "Error mkdir" << endl;
        return -1;
    }

    ret = mkdir(BIDS_dirname.c_str(), 0700);
    if (ret == -1) {
        rmdir(AID_dirname.c_str());
        return -1;
    }

    return 1;
}

void deleteAuctionDir(string &aid) {
    string AID_dirname = "AUCTIONS/" + aid;
    string BIDS_dirname = "AUCTIONS/" + aid + "/BIDS";

    rmdir(AID_dirname.c_str());
    rmdir(BIDS_dirname.c_str());
}

string getNextAID() {
    char tmp[4];
    snprintf(tmp, 4, "%03d", auction_number);
    string aid(tmp);
    auction_number++;
    return aid;
}

int createStartAuctionText(vector<string> &arguments, string &aid) {
    string name, tmp;
    char time_str[20]; // date and time

    tmp = arguments[0] + " "; // UID
    tmp += arguments[2] + " "; // Name
    tmp += arguments[5] + " "; // fname
    tmp += arguments[3] + " "; // start_value
    tmp += arguments[4] + " "; // time active

    time(&fulltime); // update time in seconds
    current_time = gmtime(&fulltime);
    snprintf(time_str, 20, "%4d−%02d−%02d %02d:%02d:%02d",
            current_time->tm_year + 1900, current_time->tm_mon + 1,current_time->tm_mday, 
            current_time->tm_hour , current_time->tm_min , current_time->tm_sec);
    tmp += time_str;
    tmp += " ";
    
    stringstream ss;
    ss << fulltime;
    string ts = ss.str();
    tmp += ts;

    name = "AUCTIONS/" + aid + "/START_" + aid + ".txt";
    ofstream fout(name, ios::out);
    if (!fout) {
        cout << "Error creating start auction text" << endl;
        return -1;
    }
    fout.write(name.c_str(), name.size());
    fout.close();

    return 0;
}

void handleTCPRequest(int &fd, pid_t &pid) {
    cout << "[LOG]: Got one request child " << pid << " handling it" << endl;
    string request, tmp;
    vector<string> request_arguments;
    char buffer[BUFFERSIZE];
    int n, request_type;

    receiveTCPsize(fd, 3, tmp, pid);
    request_type = parseCommand(tmp);
    receiveTCPsize(fd, 1, tmp, pid); // clear space
    switch (request_type) {
        case OPEN:
            {
            receiveTCPspace(fd, 7, request, pid);
            parseInput(request, request_arguments);
            ssize_t fsize;
            stringstream stream(request_arguments[6]);
            stream >> fsize;

            bool ok = true;
            if (!checkLogin(request_arguments[0])) {
                // user not logged in
                ok = false;
                return; // maybe we can just return here
            } else if (!checkPassword(request_arguments[0], request_arguments[1])) {
                // incorrect password
                ok = false;
            } else {
                // check if an auction has the same name?
                // check name and other things' length
                
            }

            if (!ok) {
                tmp = "ROA NOK\n";
                sendTCPresponse(fd, tmp, tmp.size(), pid);
            } else {
                // talvez pode-se remover a pasta com tds os ficheiros dentro dela
                int status;
                string aid = getNextAID();
                status = createAuctionDir(aid);
                if (status == -1) {
                    break;
                }
                status = receiveTCPimage(fd, fsize, request_arguments[5], aid, pid);
                if (status == -1) {
                    deleteAuctionDir(aid); // adicionar loop ate confirmar sucesso?
                    break;
                }
                status = createStartAuctionText(request_arguments, aid);
                if (status == -1) {
                    deleteAuctionDir(aid);
                    string dir = "AUCTIONS/" + aid + "/" + request_arguments[5];
                    remove(dir.c_str());
                    break;
                }
                tmp = "USERS/" + request_arguments[0] + "/HOSTED/" + aid + ".txt"; // create hosted in user folder
                ofstream fout(tmp);
                if (!fout) {
                    deleteAuctionDir(aid);
                    string dir = "AUCTIONS/" + aid + "/" + request_arguments[5];
                    remove(dir.c_str());
                    dir = "AUCTIONS/" + aid + "/START_" + aid + ".txt";
                    remove(dir.c_str());
                    break;
                }
                fout.close();
                
                tmp = "ROA OK " + aid + "\n";
                sendTCPresponse(fd, tmp, tmp.size(), pid);
            }

            // check user
            // and check arguments
            
            // open auction
            // respond to client
            break;
        }
        case CLOSE:
            break;
        case SHOW_ASSET:
            break;
        case BID:
            break;
        default:
            cout << "Syntax error" << endl;
            break;
    }

    close(fd);
}

void startTCP(void) {
    fd_tcp = socket(AF_INET, SOCK_STREAM, 0);
    if (fd_tcp == -1) {
        cout << "TCP socket error" << endl;
        exit(EXIT_FAILURE);
    }

    n_tcp = bind(fd_tcp, res->ai_addr, res->ai_addrlen);
    if (n_tcp == -1) {
        cout << "TCP bind error" << endl;
        exit(EXIT_FAILURE);
    }

    if (listen(fd_tcp, 50) == -1) {
        cout << "TCP listen error" << endl;
        exit(EXIT_FAILURE);
    }

    while (1) {
        addrlen = sizeof(addr);
        cout << "[LOG]: TCP parent starting to accept requests" << endl;
        if ( (tcp_child = accept(fd_tcp, (struct sockaddr*) &addr, &addrlen)) == -1) {
            cout << "TCP accept error" << endl;
            exit(EXIT_FAILURE);
        }

        pid_t pid = fork();
        if (pid == -1) {
            cout << "TCP fork error" << endl;
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            handleTCPRequest(tcp_child, pid);
        }
    }
}

int main(int argc, char *argv[]) {

    switch(argc) {
        case 1:
            port = DEFAULT_PORT;
            break;
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

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    errcode = getaddrinfo(NULL, port.c_str(), &hints, &res);
    if (errcode != 0) {
        cout << gai_strerror(errcode);
        exit(1);
    }

    pid_t pid = fork();
    if (pid == -1) {
        cout << "Fork error" << endl;
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        startTCP();
    } else {
        startUDP();
    }
    
}