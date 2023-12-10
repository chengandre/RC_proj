// handle signal child
// remove exit(1)
// check if read==0
// verify receicetcpsize
// cannot send more than 2 opens
#include "User.hpp"
#include "common.hpp"
using namespace std;

int fd_udp;
socklen_t addrlen;
struct addrinfo hints, *res;
struct sockaddr_in addr;
string hostname, port, input;
char buffer[BUFFERSIZE];
vector<string> inputs, userInfo;
bool loggedIn = false;

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

int indexSpace(int n_spaces, string &target){
    // finds the index of the nth space
    int count = 0;
    int i = 0;
    while (count < n_spaces && i < target.size()) {
        if (target[i] == ' ') {
            count++;
        }
        i++;
    }
    if (i == target.size()) {
        return -1;
    }
    return i-1;
}

void saveJPG(string &data, string &fname) {
    std::ofstream fout(fname, std::ios::binary);
    fout.write(data.c_str(), data.size());
    fout.close();
}

int sendReceiveUDPRequest(string &message, int size, string &response) {
    // change to read more from server
    int total_received = 0;
    int total_sent = 0;
    int n;
    cout << "[LOG]: Sending UDP request" << endl;
    while (total_sent < size) {
        n = sendto(fd_udp, message.c_str() + total_sent, size - total_sent, 0, res->ai_addr, res->ai_addrlen);
        if (n == -1) {
            cout << "UDP send error" << endl;
            return n;
        }
        total_sent += n;
    }
    cout << "[LOG]: Sent UDP request" << endl;

    addrlen = sizeof(addr);
    response.clear();
    n = BUFFERSIZE;
    cout << "[LOG]: Receiving UDP response" << endl;
    while (n == BUFFERSIZE) {
        n = recvfrom(fd_udp, buffer, BUFFERSIZE, 0, (struct sockaddr*) &addr, &addrlen);
        if (n == -1) {
            cout << "UDP receive error" << endl;
            break;
        }
        concatenateString(response, buffer, n);
        total_received += n;
    }
    cout << "[LOG]: Received UDP response of size " <<  response.size() << endl;

    return total_received;
    // return number of bytes read
}

void handleUDPRequest(int request, vector<string> arguments) {
    string response, message; // response/message to/from server
    vector<string> response_arguments; // split response from server

    switch (request) {
        case LOGIN: {
            string uid = arguments[1];
            string pass = arguments[2];
            try
            {
                if (checkUID(uid) && checkPasswordSyntax(pass)) {

                    message = "LIN " + uid + " " + pass + "\n";
                    sendReceiveUDPRequest(message, message.size(), response);

                    if (response.size() < 7 || response.size() > 8 || response.back() != '\n') throw -1;

                    parseInput(response, response_arguments);

                    if (response_arguments[0] != "RLI") throw -1;

                    if (response_arguments[1] == "OK") {
                        loggedIn = true;
                        userInfo.push_back(uid);
                        userInfo.push_back(pass);
                        cout << "Logged in successfully" << endl;
                    }
                    else if (response_arguments[1] == "NOK") {
                        cout << "Incorrect password" << endl;
                    }
                    else if (response_arguments[1] == "REG") {
                        loggedIn = true;
                        userInfo.push_back(uid);
                        userInfo.push_back(pass);
                        cout << "New User registered" << endl;
                    }
                    else {
                        throw -1;
                    }
                } else {
                    throw 0;
                }
            } catch(int n) {
                if (n == 0) {
                    cout << "Syntax error" << endl;
                } else {
                    cout << "Invalid response from server" << endl;
                }
            }
            break;
        }
        case LOGOUT: {
            try
            {
                if (!loggedIn) throw 1;

                string uid = userInfo[0];
                string pass = userInfo[1];

                if (checkUID(uid) && checkPasswordSyntax(pass)) {
                    message = "LOU " + uid + " " + pass + "\n";
                    sendReceiveUDPRequest(message, message.length(), response);
                    
                    if (response.size() < 7 || response.size() > 8 || response.back() != '\n') throw -1;
                    parseInput(response, response_arguments);
                    if (response_arguments[0] != "RLO") throw -1;

                    if (response_arguments[1] == "OK") {
                        userInfo.clear();
                        loggedIn = false;
                        cout << "Logged out successfully" << endl;
                    } else if (response_arguments[1] == "NOK") {
                        cout << "User not logged in" << endl;
                    } else if (response_arguments[1] == "UNR") {
                        cout << "User is not registered" << endl;
                    } else {
                        throw -1;
                    }
                } else {
                    throw 0;
                }
            }
            catch(int n)
            {
                if (n == 1) {
                    cout << "User not logged in" << endl;
                } else if (n == 0) {
                    cout << "Syntax error" << endl;
                } else if (n == -1) {
                    cout << "Invalid response from server" << endl;
                }
            }
            break;
        }
        case UNREGISTER: {  
            try
            {
                if (!loggedIn) throw 1;

                string uid = userInfo[0];
                string pass = userInfo[1];

                if (checkUID(uid) && checkPasswordSyntax(pass)) {
                    message = "UNR " + uid + " " + pass + "\n";
                    sendReceiveUDPRequest(message, message.length(), response);
                    
                    if (response.size() < 7 || response.size() > 8 || response.back() != '\n') throw -1;
                    parseInput(response, response_arguments);
                    if (response_arguments[0] != "RUR") throw -1;

                    if (response_arguments[1] == "OK") {
                        userInfo.clear();
                        loggedIn = false;
                        cout << "User registered successfully" << endl;
                    } else if (response_arguments[1] == "NOK") {
                        cout << "User not logged in" << endl;
                    } else if (response_arguments[1] == "UNR") {
                        cout << "User not registered" << endl;
                    } else {
                        throw -1;
                    }
                } else {
                    throw 0;
                }
            }
            catch(int n)
            {
                if (n == 1) {
                    cout << "User not logged in" << endl;
                } else if (n == 0) {
                    cout << "Syntax error" << endl;
                } else if (n == -1) {
                    cout << "Invalid response from server" << endl;
                }
            }
            break;
        }
        case MYAUCTIONS: {
            try {
                string uid = arguments[1];
                if (checkUID(uid)) {
                    message = "LMA " + uid + "\n";
                    sendReceiveUDPRequest(message, message.length(), response);

                    if (response.size() < 7 || response.back() != '\n') throw -1;
                    parseInput(response, response_arguments);
                    if (response_arguments[0] != "RMA") throw -1;

                    if (response_arguments[1] == "NOK") {
                        cout << "User has no ongoing auctions" << endl;
                    } else if (response_arguments[1] == "NLG") {
                        cout << "User not logged in" << endl;
                    } else if (response_arguments[1] == "OK") {
                        cout << "Listing auctions from user " << response_arguments[1] << ":" << endl;
                        for (int i = 2; i < response_arguments.size() - 1; i += 2) {
                            cout << "Auction " << response_arguments[i] << " ";
                            if (response_arguments[i+1] == "0") {
                                cout << "Ended" << endl;
                            } else if (response_arguments[i+1] == "1") {
                                cout << "Ongoing" << endl;
                            } else {
                                throw -1;
                            }
                        }
                    } else {
                        throw -1;
                    }
                } else {
                    throw 0;
                }
            } catch (int n) {
                if (n == 0) {
                    cout << "Syntax error" << endl;
                } else if (n == -1) {
                    cout << "Invalid response from server" << endl;
                }
            }
            break;
        }
        case MYBIDS: {
            try {
                string uid = arguments[1];
                if (checkUID(uid)) {
                    message = "LMB " + uid + "\n";
                    sendReceiveUDPRequest(message, message.length(), response);
                    
                    if (response.size() < 7 || response.back() != '\n') throw -1;
                    parseInput(response, response_arguments);
                    if (response_arguments[0] != "RMB") throw -1;

                    if (response_arguments[1] == "NOK") {
                        cout << "User has no ongoing bids" << endl;
                    } else if (response_arguments[1] == "NLG") {
                        cout << "User not logged in" << endl;
                    } else if (response_arguments[1] == "OK") {
                        cout << "Listing auctions from user " << uid << " in which has bidded:" << endl;
                        for (int i = 2; i < response_arguments.size() - 1; i += 2) {
                            cout << "Auction " << response_arguments[i] << " ";
                            if (response_arguments[i+1] == "0") {
                                cout << "Ended" << endl;
                            } else if (response_arguments[i+1] == "1") {
                                cout << "Ongoing" << endl;
                            } else {
                                throw -1;
                            }
                        }
                    } else {
                        throw -1;
                    }
                } else {
                    throw 0;
                }
            } catch (int n) {
                if (n == 0) {
                    cout << "Syntax error" << endl;
                } else if (n == -1) {
                    cout << "Invalid response from server" << endl;
                }
            }
            break;
        }
        case LIST: {
            try {
                message = "LST\n";
                sendReceiveUDPRequest(message, message.length(), response);
                
                if (response.size() < 7 || response.back() != '\n') throw -1;
                parseInput(response, response_arguments);
                if (response_arguments[0] != "RLS") throw -1;

                if (response_arguments[1] == "NOK") {
                    cout << "No auctions have been started yet" << endl;
                } else if (response_arguments[1] == "OK") {
                    cout << "Listing all auctions:" << endl;
                    for (int i = 2; i < response_arguments.size() - 1; i += 2) {
                        cout << "Auction " << response_arguments[i] << " ";
                        if (response_arguments[i+1] == "0") {
                            cout << "Ended" << endl;
                        } else if (response_arguments[i+1] == "1") {
                            cout << "Ongoing" << endl;
                        } else {
                            throw -1;
                        }
                    }
                } else {
                    throw -1;
                }
            } catch (int n) {
                if (n == 0) {
                    cout << "Syntax error" << endl;
                } else if (n == -1) {
                    cout << "Invalid response from server" << endl;
                }
            }
            break;
        }
        case SHOW_RECORD: {
            try {
                // check each entry? auction names, fnames ..
                string aid = arguments[1];
                if (checkAID(aid)) {
                    message = "SRC " + aid + "\n";
                    sendReceiveUDPRequest(message, message.length(), response);

                    if (response.size() < 7 || response.back() != '\n') throw -1;
                    parseInput(response, response_arguments);
                    if (response_arguments[0] != "RRC") throw -1;

                    if (response_arguments[1] == "NOK") {
                        cout << "No auction has such AID" << endl;
                    } else if (response_arguments[1] == "OK") {
                        cout << "Auction " << aid << " was started by the user " << response_arguments[2] << "." << endl;
                        cout << "Auction name: " << response_arguments[3] << endl;
                        cout << "Item name: " << response_arguments[4] << endl;
                        cout << "Image name: " << response_arguments[5] << endl;
                        cout << "Starting price: " << response_arguments[6] << endl;
                        cout << "Start date: " << response_arguments[7] << endl;
                        cout << "Time since start: " << response_arguments[8] << endl;

                        int index = 8;
                        bool end = false;
                        bool bids = false;
                        while (!end) {
                            if (response_arguments.size() > index) {
                                index++;
                                if (response_arguments[index] == "B") {
                                    if (!bids) {
                                        cout << "Listing bids:" << endl;
                                    }
                                    bids = true;
                                    cout << "Bidder UID: " << response_arguments[++index] << endl;
                                    cout << "Bid value: " << response_arguments[++index] << endl;
                                    cout << "Bid date: " << response_arguments[++index] << endl;
                                    cout << "Bid time: " << response_arguments[++index] << endl;
                                    cout << "Bid time relative to auction start: " << response_arguments[++index] << endl;
                                }
                                else if (response_arguments[index] == "E") {
                                    cout << "Auction ended in " << response_arguments[++index] << " at ";
                                    cout << response_arguments[++index] << endl;
                                    cout << "Auction duration: " << response_arguments[++index] << endl;
                                } else {
                                    throw -1;
                                }
                            }
                            else {
                                end = true;
                            }
                        }
                    } else {
                        throw -1;
                    }
                } else {
                    throw 0;
                }
            } catch (int n){
                if (n == 0) {
                    cout << "Syntax error" << endl;
                } else if (n == -1) {
                    cout << "Invalid response from server" << endl;
                }
            }
            
            break;
        }
        default:
            cout << "Syntax error" << endl;
            break;
    }
}

int sendTCPmessage(int const &fd, string &message, int size) {
    cout << "[LOG]: Sending TCP request" << endl;
    int total_sent = 0;
    int n, to_send;
    while (total_sent < size) {
        to_send = min(128, size-total_sent);
        n = write(fd, message.c_str() + total_sent, to_send);
        if (n == -1) {
            cout << "TCP send error" << endl;
            return n;
        }
        total_sent += n;    
    }
    cout << "[LOG]: Sent TCP response" << endl;
    return total_sent;
}

int receiveTCPsize(int const &fd, int const &size, string &response) {
    int total_received = 0;
    int n;
    char tmp[128];
    response.clear();
    cout << "[LOG]: Receiving TCP request by size" << endl;
    while (total_received < size) {
        n = read(fd, tmp, 1);
        if (n == -1) {
            cout << "TCP receive error" << endl;
            break;
        }
        concatenateString(response, tmp, n);
        total_received += n;
    }
    cout << "[LOG]: Received response of size " << total_received << endl;

    return total_received;
}

int receiveTCPspace(int fd, int size, string &response) {
    int total_received = 0;
    int total_spaces = 0;
    int n;
    char tmp[128];
    response.clear();
    cout << "[LOG]: Receiving TCP request by spaces" << endl;
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
    cout << "[LOG]: Received response of size " << total_received << endl;

    return total_received;
}

int receiveTCPend(int fd, string &response) {
    int total_received = 0;
    int n;
    char tmp[128];
    response.clear();
    
    cout << "[LOG]: Receiving TCP until the end" << endl;
    while (true) {
        n = read(fd, tmp, 1);
        if (n == -1) {
            cout << "TCP receive error" << endl;
            break;
        } else if (tmp[0] == '\n') {
            break;
        }
        concatenateString(response, tmp, n);
        total_received += n;
        
    }
    cout << "[LOG]: Received response of size " << total_received << endl;

    return total_received;
}

int receiveTCPfile(int fd, int size, string &fname) {
    int total_received = 0;
    int n, to_read;
    char tmp[128];
    ofstream fout(fname, ios::binary);

    while (total_received < size) {
        to_read = min(128, size-total_received);
        n = read(fd, tmp, to_read);
        if (n == -1) {
            cout << "TCP image receive error" << endl;
            fout.close();
            return -1;
        }
        fout.write(tmp, n);
        total_received += n;
    }
    cout << "[LOG]: Received file of size " << total_received << " fsize is " << size << endl;
    fout.close();
    
    return total_received;
    // receive image directly into file
}

void handleTCPRequest(int request, vector<string> inputs) {
    cout << "Handling TCP Request" << endl;

    int n;
    string message, tmp;
    vector<string> message_arguments;

    int fd_tcp = socket(AF_INET, SOCK_STREAM, 0);
    if (fd_tcp == -1) {
        return;
    }

    int on = 1;
    setsockopt(fd_tcp, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    n = connect(fd_tcp, res->ai_addr, res->ai_addrlen);
    if (n == -1) {
        return;
    }
    
    switch (request) {
        case OPEN:
            // check syntax
            // open name asset_fname start_value timeactive
            if (!loggedIn) {
                cout << "User not logged in" << endl;
            }
            message = "OPA " + userInfo[0] + " " + userInfo[1] + " " + inputs[1] + " ";
            message += inputs[3] + " " + inputs[4] + " " + inputs[2] + " ";
            tmp = openJPG(inputs[2]);
            // filesystem::file_size(inputs[2]);
            message += to_string(tmp.size()) + " ";
            message += openJPG(inputs[2]) + "\n";
            n = sendTCPmessage(fd_tcp, message, message.size());

            n = receiveTCPsize(fd_tcp, 7, message);
            // n = sendReceiveTCPRequest(message, message.length());
            parseInput(message, message_arguments);
            receiveTCPend(fd_tcp, message);
            if (message_arguments[1] == "NOK") {
                cout << "Auction could not be started" << endl;
            } else if (message_arguments[1] == "NLG") {
                cout << "User not logged in" << endl;
            } else if (message_arguments[1] == "OK") {
                cout << "Auction created with AID " << message << endl;
            }
            break;
        case CLOSE:
            if (!loggedIn) {
                cout << "User not logged in" << endl;
            }
            message = "CLS" + userInfo[0] + " " + userInfo[1] + " " + inputs[1] + "\n";
            n = sendTCPmessage(fd_tcp, message, message.size());

            n = receiveTCPsize(fd_tcp, 7, message);

            // n = sendReceiveTCPRequest(message, message.length());

            parseInput(message, message_arguments);
            if (message_arguments[1] == "OK") {
                cout << "Auction created by the user has now been closed" << endl;
                receiveTCPend(fd_tcp, message);
            } else if (message_arguments[1] == "NLG") {
                cout << "User not logged in" << endl;
                receiveTCPend(fd_tcp, message);
            } else if (message_arguments[1] == "EAU") {
                cout << "No auction with such AID" << endl;
                receiveTCPend(fd_tcp, message);
            } else if (message_arguments[1] == "EOW") {
                cout << "User not the owner of auction" << endl;
                receiveTCPend(fd_tcp, message);
            } else if (message_arguments[1] == "END") {
                cout << "Auction has already been closed" << endl;
                receiveTCPend(fd_tcp, message);
            }
            break;
        case SHOW_ASSET:
            message =  "SAS " + inputs[1] + "\n";
            n = sendTCPmessage(fd_tcp, message, message.size());

            n = receiveTCPsize(fd_tcp, 7, message);
            parseInput(message, message_arguments);
            //n = sendReceiveTCPRequest(message, message.length());

            //tmp = getSubString(all_response, 0, 7);
            // tmp = all_response.substr(0, 7);
            
            if (message_arguments[1] == "NOK") {
                receiveTCPend(fd_tcp, message);
                cout << "Error showing asset" << endl;
            } else if (message_arguments[1] == "OK") {
                n = receiveTCPspace(fd_tcp, 2, message);
                parseInput(message, message_arguments);

                string fname = message_arguments[0];
                string fsize_str = message_arguments[1];
                ssize_t fsize;
                stringstream stream(fsize_str); // turn size into int
                stream >> fsize;

                n = receiveTCPfile(fd_tcp, fsize, fname);
                n = receiveTCPend(fd_tcp, message);
                // check if message == '\n'

                // tmp.clear();
                // int space_index = indexSpace(4, all_response);
                // tmp = getSubString(all_response, 0, space_index); // get first 4 inputs
                // //tmp = all_response.substr(0, space_index); // get first 4 inputs
                // parseInput(tmp, response);
                // printVectorString(response);
                

                // tmp.clear();
                // tmp = getSubString(all_response, space_index+1, fsize); // get the image from response
                // saveJPG(tmp, response[2]); // save the data into fname
            }
            break;
        case BID:
            // check syntax (bid > 0)
            message = "BID " + userInfo[0] + " " + userInfo[1] + " " + inputs[1] + " " + inputs[2] + "\n";
            n = sendTCPmessage(fd_tcp, message, message.size());

            n = receiveTCPend(fd_tcp, message);
            if (n != 7) {
                cout << "Syntax error while receiving response from server, BID" << endl;
                break;
            }

            parseInput(message, message_arguments);
            if (message_arguments[1] == "NOK") {
                cout << "Given auction is not active" << endl;
            } else if (message_arguments[1] == "ACC") {
                cout << "Bid has been accepted" << endl;
            } else if (message_arguments[1] == "REF") {
                cout << "Bid has been refused" << endl;
            } else if (message_arguments[1] == "ILG") {
                cout << "Cannot bid in an auction hosted by the user" << endl;
            }  
            break;
        default:
            cout << "Syntax Error" << endl;
            n = -1;
            break;
    }
    
    close(fd_tcp);
    cout << "[LOG]: closed tcp connection" <<  endl;
    return;
}

int main(int argc, char *argv[]) {
    switch(argc){
        case(1):{
            hostname = DEFAULT_HOSTNAME;
            port = DEFAULT_PORT;
            break;
        }
        case(3):{
            if (!strcmp(argv[1],"-p")){
                hostname = DEFAULT_HOSTNAME;
                port = argv[2];

            }
            else{
                hostname = argv[2];
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

    int errcode = getaddrinfo(hostname.c_str(), port.c_str(), &hints, &res);
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
            if (loggedIn) {
                vector<string> inputs;
                inputs.push_back(" ");
                inputs.push_back(userInfo[0]);
                inputs.push_back(userInfo[1]);
                handleUDPRequest(LOGOUT, inputs);
            }
            return EXIT_SUCCESS;
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