// check if read==0
// listing function so that they add to a buffer and print everything in the end
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

void sendReceiveUDPRequest(string &message, int size, string &response) {
    //int total_received = 0;
    int total_sent = 0;
    int n;
    cout << "[LOG]: Sending UDP request" << endl;
    while (total_sent < size) {
        n = sendto(fd_udp, message.c_str() + total_sent, size - total_sent, 0, res->ai_addr, res->ai_addrlen);
        if (n == -1) {
            throw string("Error sending message through UDP socket");
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
            throw string("Error receiving message through UDP socket");
        }
        concatenateString(response, buffer, n);
        //total_received += n;
    }
    cout << "[LOG]: Received UDP response of size " <<  response.size() << endl;

    // return total_received;
}

void handleUDPRequest(int request, vector<string> arguments) {
    string response, message; // response/message to/from server
    vector<string> response_arguments; // split response from server

    try {
        switch (request) {
            case LOGIN: {
                string uid = arguments[1];
                string pass = arguments[2];

                checkUID(uid);
                checkPasswordSyntax(pass);

                message = "LIN " + uid + " " + pass + "\n";
                sendReceiveUDPRequest(message, message.size(), response);

                if (response.size() < 7 || response.size() > 8 || response.back() != '\n') {
                    throw string("Invalid response from server");
                }
                    
                parseInput(response, response_arguments);
                if (response_arguments[0] != "RLI") {
                    throw string("Invalid response from server");
                }   

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
                else if (response_arguments[1] == "ERR"){
                    cout << "Error on server side" << endl;
                } else {
                    throw string("Invalid response from server");
                }
                break;
            }
            case LOGOUT: {
                if (!loggedIn) throw string("User not logged in");

                string uid = userInfo[0];
                string pass = userInfo[1];
                checkUID(uid);
                checkPasswordSyntax(pass);

                message = "LOU " + uid + " " + pass + "\n";
                sendReceiveUDPRequest(message, message.length(), response);
                
                if (response.size() < 7 || response.size() > 8 || response.back() != '\n') {
                    throw string("Invalid response from server");
                }
                parseInput(response, response_arguments);
                if (response_arguments[0] != "RLO") {
                    throw string("Invalid response from server");
                }

                if (response_arguments[1] == "OK") {
                    userInfo.clear();
                    loggedIn = false;
                    cout << "Logged out successfully" << endl;
                } else if (response_arguments[1] == "NOK") {
                    cout << "User not logged in" << endl;
                } else if (response_arguments[1] == "UNR") {
                    cout << "User is not registered" << endl;
                } else if (response_arguments[1] == "ERR") {
                    cout << "Error on server side" << endl;
                } else {
                    throw string("Invalid response from server");
                }
                break;
            }
            case UNREGISTER: {  
                if (!loggedIn) throw string("User not logged in");

                string uid = userInfo[0];
                string pass = userInfo[1];
                checkUID(uid);
                checkPasswordSyntax(pass);

                message = "UNR " + uid + " " + pass + "\n";
                sendReceiveUDPRequest(message, message.length(), response);
                
                if (response.size() < 7 || response.size() > 8 || response.back() != '\n') {
                    throw string("Invalid response from server");
                }
                parseInput(response, response_arguments);
                if (response_arguments[0] != "RUR") {
                    throw string("Invalid response from server");
                }

                if (response_arguments[1] == "OK") {
                    userInfo.clear();
                    loggedIn = false;
                    cout << "User registered successfully" << endl;
                } else if (response_arguments[1] == "NOK") {
                    cout << "User not logged in" << endl;
                } else if (response_arguments[1] == "UNR") {
                    cout << "User not registered" << endl;
                } else if (response_arguments[1] == "ERR") {
                    cout << "Error on server side" << endl;
                } else {
                    throw string("Invalid response from server");
                }
                break;
            }
            case MYAUCTIONS: {
                if (!loggedIn) throw string("User not logged in");
                string uid = userInfo[0];

                message = "LMA " + uid + "\n";
                sendReceiveUDPRequest(message, message.length(), response);

                if (response.size() < 7 || response.back() != '\n') {
                    throw string("Invalid response from server");
                }
                parseInput(response, response_arguments);
                if (response_arguments[0] != "RMA") {
                    throw string("Invalid response from server");
                }

                if (response_arguments[1] == "NOK") {
                    cout << "User has no ongoing auctions" << endl;
                } else if (response_arguments[1] == "NLG") {
                    cout << "User not logged in" << endl;
                } else if (response_arguments[1] == "OK") {
                    string to_print;
                    to_print += "Listing auctions from user " + uid + ":\n";
                    for (int i = 2; i < response_arguments.size() - 1; i += 2) {
                        to_print += "Auction " + response_arguments[i] + " ";
                        if (response_arguments[i+1] == "0") {
                            to_print += "Ended\n";
                        } else if (response_arguments[i+1] == "1") {
                            to_print += "Ongoing\n";
                        } else {
                            throw string("Invalid response from server");
                        }
                    }
                    cout << to_print;
                } else if (response_arguments[1] == "ERR") {
                    cout << "Error on server side" << endl;
                } else {
                    throw string("Invalid response from server");
                }
                break;
            }
            case MYBIDS: {
                if (!loggedIn) throw string("User not logged in");
                string uid = userInfo[0];

                message = "LMB " + uid + "\n";
                sendReceiveUDPRequest(message, message.length(), response);
                
                if (response.size() < 7 || response.back() != '\n') {
                    throw string("Invalid response from server");
                }
                parseInput(response, response_arguments);
                if (response_arguments[0] != "RMB") {
                    throw string("Invalid response from server");
                }

                if (response_arguments[1] == "NOK") {
                    cout << "User has no ongoing bids" << endl;
                } else if (response_arguments[1] == "NLG") {
                    cout << "User not logged in" << endl;
                } else if (response_arguments[1] == "OK") {
                    string to_print;
                    to_print += "Listing auctions from user " + uid + " in which has bidded:\n";
                    for (int i = 2; i < response_arguments.size() - 1; i += 2) {
                        to_print += "Auction " + response_arguments[i] + " ";
                        if (response_arguments[i+1] == "0") {
                            to_print += "Ended\n";
                        } else if (response_arguments[i+1] == "1") {
                            to_print += "Ongoing\n";
                        } else {
                            throw string("Invalid response from server");
                        }
                    }
                    cout << to_print;
                } else if (response_arguments[1] == "ERR") {
                    cout << "Error on server side" << endl;
                } else {
                    throw string("Invalid response from server");
                }
                break;
            }
            case LIST: {
                message = "LST\n";
                sendReceiveUDPRequest(message, message.length(), response);
                
                if (response.size() < 7 || response.back() != '\n') {
                    throw string("Invalid response from server");
                }
                parseInput(response, response_arguments);
                if (response_arguments[0] != "RLS") {
                    throw string("Invalid response from server");
                }

                if (response_arguments[1] == "NOK") {
                    cout << "No auctions have been started yet" << endl;
                } else if (response_arguments[1] == "OK") {
                    string to_print;
                    to_print += "Listing all auctions:\n";
                    for (int i = 2; i < response_arguments.size() - 1; i += 2) {
                        to_print += "Auction " + response_arguments[i] + " ";
                        if (response_arguments[i+1] == "0") {
                            to_print += "Ended\n";
                        } else if (response_arguments[i+1] == "1") {
                            to_print += "Ongoing\n";
                        } else {
                            throw string("Invalid response from server");
                        }
                    }
                    cout << to_print;
                } else if (response_arguments[1] == "ERR") {
                    cout << "Error on server side" << endl;
                } else {
                    throw string("Invalid response from server");
                }
                break;
            }
            case SHOW_RECORD: {
                string aid = arguments[1];
                checkAID(aid);

                message = "SRC " + aid + "\n";
                sendReceiveUDPRequest(message, message.length(), response);

                if (response.size() < 7 || response.back() != '\n') {
                    throw string("Invalid response from server");
                }
                parseInput(response, response_arguments);
                if (response_arguments[0] != "RRC") {
                    throw string("Invalid response from server");
                }

                if (response_arguments[1] == "NOK") {
                    cout << "No auction has such AID" << endl;
                } else if (response_arguments[1] == "OK") {
                    string to_print;

                    string uid = response_arguments[2];
                    string auction_name = response_arguments[3];
                    string fname = response_arguments[4];
                    string start_value = response_arguments[5];
                    string date = response_arguments[6];
                    string hour = response_arguments[7];
                    string duration = response_arguments[8];
                    checkUID(uid);
                    checkName(auction_name);
                    checkFileName(fname);
                    checkStartValue(start_value);
                    checkDate(date);
                    checkHour(hour);
                    checkDuration(duration);

                    to_print += "Auction " + aid + " was started by the user " + uid + ".\n";
                    to_print += "Auction name: " + auction_name + "\n";
                    to_print += "File name: " + fname + "\n";
                    to_print += "Starting price: " + start_value + "\n";
                    to_print += "Start date: " + date + "\n";
                    to_print += "Start hour: " + hour + "\n";
                    to_print += "Duration: " + duration + "\n";

                    int index = 8;
                    bool end = false;
                    bool bids = false;
                    string bid_uid;
                    string bid_value;
                    string bid_date;
                    string bid_hour;
                    string bid_duration;
                    string end_date;
                    string end_hour;
                    string end_duration;
                    while (!end) {
                        if (response_arguments.size() - 1 > index) {
                            index++;
                            if (response_arguments[index] == "B") {
                                if (!bids) {
                                    cout << "Listing bids:" << endl;
                                }
                                bids = true;
                                bid_uid = response_arguments[++index];
                                bid_value = response_arguments[++index];
                                bid_date = response_arguments[++index];
                                bid_hour = response_arguments[++index];
                                bid_duration = response_arguments[++index];
                                checkUID(bid_uid);
                                checkStartValue(bid_value);
                                checkDate(bid_date);
                                checkHour(bid_hour);
                                checkDuration(bid_duration);

                                to_print += "Bidder UID: " + bid_uid + "\n";
                                to_print += "Bid value: " + bid_value + "\n";
                                to_print += "Bid date: " + bid_date + "\n";
                                to_print += "Bid hour: " + bid_hour + "\n";
                                to_print += "Bid duration: " + bid_duration + "\n";
                            }
                            else if (response_arguments[index] == "E") {
                                end_date = response_arguments[++index];
                                end_hour = response_arguments[++index];
                                end_duration = response_arguments[++index];
                                checkDate(end_date);
                                checkHour(end_hour);
                                checkDuration(end_duration);

                                to_print += "Auction ended in " + end_date + " at " + end_hour + "\n";
                                to_print += "Auction duration: " + end_duration + "\n";
                            } else {
                                throw string("Invalid response from server");
                            }
                        }
                        else {
                            end = true;
                        }
                    }
                    cout << to_print;
                } else if (response_arguments[1] == "ERR") {
                    cout << "Error on server side" << endl;
                } else {
                    throw string("Invalid response from server");
                }
                break;
            }
            default:
                cout << "Syntax error" << endl;
                break;
        }
    }
    catch(string error)
    {
        cout << error << endl;
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
            throw string("Error while reading from TCP socket");
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
            throw string("Error while reading from TCP socket");
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
            throw string("Error while reading from TCP socket");
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
            throw string("Error while reading from TCP socket");
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
            fout.close();
            throw string("Error while reading from TCP socket");
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

    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    if (setsockopt(fd_tcp, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) == -1) {
        cout << "[LOG]: " << getpid() << " Error setting timeout" << endl;
        int ret;
        do {
            ret = close(fd_tcp);
        } while (ret == -1 && errno == EINTR);
        exit(EXIT_FAILURE);
    }

    if (setsockopt(fd_tcp, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) == -1) {
        cout << "[LOG]: " << getpid() << " Error setting timeout" << endl;
        int ret;
        do {
            ret = close(fd_tcp);
        } while (ret == -1 && errno == EINTR);
        exit(EXIT_FAILURE);
    }

    n = connect(fd_tcp, res->ai_addr, res->ai_addrlen);
    if (n == -1) {
        return;
    }
    
    try{
        switch (request) {
            case OPEN: {
                if (!loggedIn) throw string("User not logged in");

                string auction_name = inputs[1];
                string fname = inputs[2];
                string start_value = inputs[3];
                string duration = inputs[4];
                checkName(auction_name);
                checkFileName(fname);
                checkStartValue(start_value);
                checkDuration(duration);

                message = "OPA " + userInfo[0] + " " + userInfo[1] + " " + auction_name + " ";
                message += start_value + " " + duration + " " + fname + " ";
                tmp = openJPG(fname);
                // filesystem::file_size(inputs[2]);
                message += to_string(tmp.size()) + " ";
                message += openJPG(fname) + "\n";

                sendTCPmessage(fd_tcp, message, message.size());
                 
                n = receiveTCPsize(fd_tcp, 7, message);
                if (message.size() < 7) {
                    throw string("Invalid response from server");
                }
                parseInput(message, message_arguments);
                if (message_arguments[0] != "ROA") {
                    throw string("Invalid response from server");
                }  

                receiveTCPend(fd_tcp, message);
                if (message_arguments[1] == "NOK") {
                    cout << "Auction could not be started" << endl;
                } else if (message_arguments[1] == "NLG") {
                    cout << "User not logged in" << endl;
                } else if (message_arguments[1] == "OK") {
                    cout << "Auction created with AID " << message << endl;
                } else if (message_arguments[1] == "ERR") {
                    cout << "Error on server side" << endl;
                } else {
                    throw string("Invalid response from server");
                }
                break;
            }
            case CLOSE: {
                if (!loggedIn) throw string("User not logged in");

                string aid = inputs[1];
                checkAID(aid);

                message = "CLS " + userInfo[0] + " " + userInfo[1] + " " + aid + "\n";

                sendTCPmessage(fd_tcp, message, message.size());

                n = receiveTCPsize(fd_tcp, 7, message);
                if (message.size() < 7) {
                    throw string("Invalid response from server");
                }
                parseInput(message, message_arguments);
                if (message_arguments[0] != "RCL") {
                    throw string("Invalid response from server");
                } 

                if (message_arguments[1] == "OK") {
                    cout << "Auction created by the user has now been closed" << endl;
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
                } else if (message_arguments[1] == "ERR") {
                    cout << "Error on server side";
                } else {
                    throw string("Invalid response from server");
                }
                break;
            }
            case SHOW_ASSET: {
                string aid = inputs[1];
                checkAID(aid);

                message =  "SAS " + aid + "\n";
                sendTCPmessage(fd_tcp, message, message.size());

                n = receiveTCPsize(fd_tcp, 7, message);
                if (message.size() < 7) {
                    throw string("Invalid response from server");
                }
                parseInput(message, message_arguments);
                if (message_arguments[0] != "RSA") {
                    throw string("Invalid response from server");
                } 
               
                if (message_arguments[1] == "NOK") {
                    receiveTCPend(fd_tcp, message);
                    cout << "Error showing asset" << endl;
                } else if (message_arguments[1] == "OK") {
                    n = receiveTCPspace(fd_tcp, 2, message);
                    parseInput(message, message_arguments);

                    string fname = message_arguments[0];
                    string fsize_str = message_arguments[1];
                    checkFileName(fname);
                    checkFileSize(fsize_str);

                    ssize_t fsize;
                    stringstream stream(fsize_str); // turn size into int
                    stream >> fsize;

                    n = receiveTCPfile(fd_tcp, fsize, fname);
                    n = receiveTCPend(fd_tcp, message);
                }
                break;
            }
            case BID: {
                string aid = inputs[1];
                string value = inputs[2];
                checkAID(aid);
                checkStartValue(value);

                message = "BID " + userInfo[0] + " " + userInfo[1] + " " + inputs[1] + " " + inputs[2] + "\n";
                sendTCPmessage(fd_tcp, message, message.size());

                n = receiveTCPend(fd_tcp, message);
                if (n != 7) {
                    throw string("Invalid response from server");
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
                } else if (message_arguments[1] == "ERR") {
                    cout << "Error on server side" << endl;
                } else {
                    throw string("Invalid response from server");
                }
                break;
            }
            default:
                throw string("Invalid response from server");
                break;
        }
    } catch (string error) {
        cout << error << endl;
    }
    
    close(fd_tcp);
    cout << "[LOG]: closed tcp connection" << endl;
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
        exit(EXIT_FAILURE);
    }

    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    if (setsockopt(fd_udp, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) == -1) {
        cout << "[LOG]: " << getpid() << " Error setting timeout" << endl;
        int ret;
        do {
            ret = close(fd_udp);
        } while (ret == -1 && errno == EINTR);
        exit(EXIT_FAILURE);
    }

    if (setsockopt(fd_udp, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) == -1) {
        cout << "[LOG]: " << getpid() << " Error setting timeout" << endl;
        int ret;
        do {
            ret = close(fd_udp);
        } while (ret == -1 && errno == EINTR);
        exit(EXIT_FAILURE);
    }
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    cout << hostname << '\n';
    cout << port << '\n';

    int errcode = getaddrinfo(hostname.c_str(), port.c_str(), &hints, &res);
    if (errcode != 0) {
        cout << gai_strerror(errcode);
        exit(EXIT_FAILURE);
    }
    
    // loop to receive commands
    while (true) {
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
        } else {
            cout << "No such request\n";
        }
    }
}