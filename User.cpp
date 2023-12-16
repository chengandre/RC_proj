#include "User.hpp"
#include "common.hpp"

// [0-9] to .at() check exception, better print
// close streams

using namespace std;

int fd_udp;
socklen_t addrlen;
struct addrinfo hints, *res;
struct sockaddr_in addr;
string hostname, port;
char buffer[BUFFERSIZE];
vector<string> userInfo; // contains uid and password if loggedin
bool loggedIn = false;


// Returns the request_code given string request
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


// Sends a request to server and receives its response
void sendReceiveUDPRequest(string &message, int size, string &response) {

    int n; // returns value of sendto/recvfrom
    n = sendto(fd_udp, message.c_str(), size, 0, res->ai_addr, res->ai_addrlen); // sends the request to server
    if (n == -1) {
        throw string("Error sending message through UDP socket");
    }

    addrlen = sizeof(addr);
    response.clear();
    n = recvfrom(fd_udp, buffer, BUFFERSIZE, 0, (struct sockaddr*) &addr, &addrlen); // waits for a response from server
    if (n == -1) {
        throw string("Error receiving message through UDP socket");
    }
    concatenateString(response, buffer, n); // adds the response to a string

    checkUDPSyntax(response); // checks the overall syntax of the response (double spaces/ ends in '\n')
}

// Creates the request, sends the request, receives a response, parses the response
void handleUDPRequest(int request, vector<string> arguments) {
    string response, message; // response/message to/from server
    vector<string> response_arguments; // split response by spaces

    try {
        switch (request) {
            case LOGIN: {
                string uid = arguments.at(1);
                string pass = arguments.at(2);

                //check arguments syntax
                checkUID(uid);
                checkPasswordSyntax(pass);

                message = "LIN " + uid + " " + pass + "\n"; // request to server
                sendReceiveUDPRequest(message, message.size(), response); // send the request and get the response

                if (response.size() < 4 || response.size() > 8 || response.back() != '\n') { // checks overall size and format of the response
                    throw string("Invalid response from server");
                }
                    
                parseInput(response, response_arguments); // separate the response by spaces
                if (response_arguments.at(0) == "ERR") {
                    throw string("Error from server side, probably it couldn't identify the request");
                } 
                else if (response_arguments.at(0) != "RLI" || response.at(3) != ' ') {
                    throw string("Invalid response from server");
                }

                if (response_arguments.at(1) == "OK") {
                    // Server logged in the user, save the info
                    loggedIn = true;
                    userInfo.push_back(uid);
                    userInfo.push_back(pass);
                    cout << "Logged in successfully" << endl;
                }
                else if (response_arguments.at(1) == "NOK") {
                    // Password sent didn't match the password on the server side
                    cout << "Incorrect password" << endl;
                }
                else if (response_arguments.at(1) == "REG") {
                    // New user was registered and logged
                    loggedIn = true;
                    userInfo.push_back(uid);
                    userInfo.push_back(pass);
                    cout << "New User registered" << endl;
                }
                else if (response_arguments.at(1) == "ERR"){
                    // An Error (probably IO) occured 
                    cout << "Error on server side" << endl;
                } else {
                    throw string("Invalid response from server");
                }
                break;
            }
            case LOGOUT: {
                if (!loggedIn) throw string("User not logged in"); // User has to be logged in to logout

                string uid = userInfo.at(0);
                string pass = userInfo.at(1);
                // Check arguments syntax (should always be right, since it has been verified before)
                checkUID(uid);
                checkPasswordSyntax(pass);

                message = "LOU " + uid + " " + pass + "\n"; // request message
                sendReceiveUDPRequest(message, message.length(), response); 
                
                if (response.size() < 4 || response.size() > 8 || response.back() != '\n') { // check overall size and format
                    throw string("Invalid response from server");
                }
                parseInput(response, response_arguments); // separate by spaces
                if (response_arguments.at(0) == "ERR") {
                    throw string("Error from server side");
                } 
                else if (response_arguments.at(0) != "RLO" || response.at(3) != ' ') {
                    throw string("Invalid response from server");
                }

                if (response_arguments.at(1) == "OK") {
                    // User logged out of the server
                    // Remove user info
                    userInfo.clear();
                    loggedIn = false;
                    cout << "Logged out successfully" << endl;
                } else if (response_arguments.at(1) == "NOK") {
                    cout << "User not logged in or wrong password" << endl;
                } else if (response_arguments.at(1) == "UNR") {
                    cout << "User is not registered" << endl;
                } else if (response_arguments.at(1) == "ERR") {
                    cout << "Error on server side" << endl;
                } else {
                    throw string("Invalid response from server");
                }
                break;
            }
            case UNREGISTER: {  
                if (!loggedIn) throw string("User not logged in"); // User has to be logged in

                string uid = userInfo.at(0);
                string pass = userInfo.at(1);
                // Check arguments syntax
                checkUID(uid);
                checkPasswordSyntax(pass);

                message = "UNR " + uid + " " + pass + "\n"; // request message
                sendReceiveUDPRequest(message, message.length(), response);
                
                if (response.size() < 4 || response.size() > 8 || response.back() != '\n') { // check overall size and format
                    throw string("Invalid response from server");
                }
                parseInput(response, response_arguments); // separate by spaces
                if (response_arguments.at(0) == "ERR") {
                    throw string("Error from server side");
                } 
                else if (response_arguments.at(0) != "RUR" || response.at(3) != ' ') {
                    throw string("Invalid response from server");
                }

                if (response_arguments.at(1) == "OK") {
                    // User unregistered from server
                    // clear user info
                    userInfo.clear();
                    loggedIn = false;
                    cout << "User unregistered successfully" << endl;
                } else if (response_arguments.at(1) == "NOK") {
                    cout << "User not logged in" << endl;
                } else if (response_arguments.at(1) == "UNR") {
                    cout << "User not registered" << endl;
                } else if (response_arguments.at(1) == "ERR") {
                    cout << "Error on server side" << endl;
                } else {
                    throw string("Invalid response from server");
                }
                break;
            }
            case MYAUCTIONS: {
                if (!loggedIn) throw string("User not logged in"); // User has to be logged in
                string uid = userInfo.at(0);
                checkUID(uid); // check argument syntax (should be alright since it was checked on log in)

                message = "LMA " + uid + "\n"; // request message
                sendReceiveUDPRequest(message, message.length(), response);

                if (response.size() < 4 || response.back() != '\n') { // check overall size and format
                    throw string("Invalid response from server");
                }
                parseInput(response, response_arguments); // separate by spaces
                if (response_arguments.at(0) == "ERR") {
                    throw string("Error from server side");
                } 
                else if (response_arguments.at(0) != "RMA" || response.at(3) != ' ') {
                    throw string("Invalid response from server");
                }

                if (response_arguments.at(1) == "NOK") {
                    cout << "User has no ongoing auctions" << endl;
                } else if (response_arguments.at(1) == "NLG") {
                    cout << "User not logged in" << endl;
                } else if (response_arguments.at(1) == "OK") {
                    string to_print; // tmp string that stores the information, in case of a syntax error by the response
                    to_print += "Listing auctions from user " + uid + ":\n";
                    for (size_t i = 2; i < response_arguments.size() - 1; i += 2) {
                        // Iterate over the response two arguments by two, corresponding to the pair AID(i) Status(i+1)
                        to_print += "Auction " + response_arguments.at(i) + " ";
                        if (response_arguments.at(i+1) == "0") {
                            to_print += "Ended\n";
                        } else if (response_arguments.at(i+1) == "1") {
                            to_print += "Ongoing\n";
                        } else {
                            throw string("Invalid response from server");
                        }
                    }
                    cout << to_print; // No syntax errors, print the info
                } else if (response_arguments.at(1) == "ERR") {
                    cout << "Error on server side" << endl;
                } else {
                    throw string("Invalid response from server");
                }
                break;
            }
            case MYBIDS: {
                if (!loggedIn) throw string("User not logged in");
                string uid = userInfo.at(0);
                checkUID(uid); // check argument syntax (should be alright since it was checked on log in)

                message = "LMB " + uid + "\n"; // request message
                sendReceiveUDPRequest(message, message.length(), response);
                
                if (response.size() < 4 || response.back() != '\n') { // check overall size and format
                    throw string("Invalid response from server");
                }
                parseInput(response, response_arguments); // separate by spaces
                if (response_arguments.at(0) == "ERR") {
                    throw string("Error from server side");
                } 
                else if (response_arguments.at(0) != "RMB" || response.at(3) != ' ') {
                    throw string("Invalid response from server");
                }

                if (response_arguments.at(1) == "NOK") {
                    cout << "User has no ongoing bids" << endl;
                } else if (response_arguments.at(1) == "NLG") {
                    cout << "User not logged in" << endl;
                } else if (response_arguments.at(1) == "OK") {
                    string to_print; // tmp string that stores the information, in case of a syntax error by the response
                    to_print += "Listing auctions from user " + uid + " in which has bidded:\n";
                    for (size_t i = 2; i < response_arguments.size() - 1; i += 2) {
                        // Iterate over the response two arguments by two, corresponding to the pair AID(i) Status(i+1)
                        to_print += "Auction " + response_arguments.at(i) + " ";
                        if (response_arguments.at(i+1) == "0") {
                            to_print += "Ended\n";
                        } else if (response_arguments.at(i+1) == "1") {
                            to_print += "Ongoing\n";
                        } else {
                            throw string("Invalid response from server");
                        }
                    }
                    cout << to_print;
                } else if (response_arguments.at(1) == "ERR") {
                    cout << "Error on server side" << endl;
                } else {
                    throw string("Invalid response from server");
                }
                break;
            }
            case LIST: {
                message = "LST\n"; // request message
                sendReceiveUDPRequest(message, message.length(), response);
                
                if (response.size() < 4 || response.back() != '\n') { // check overall size and format
                    throw string("Invalid response from server");
                }
                parseInput(response, response_arguments); // separate by spaces
                if (response_arguments.at(0) == "ERR") {
                    throw string("Error from server side");
                } 
                else if (response_arguments.at(0) != "RLS" || response.at(3) != ' ') {
                    throw string("Invalid response from server");
                }

                if (response_arguments.at(1) == "NOK") {
                    cout << "No auctions have been started yet" << endl;
                } else if (response_arguments.at(1) == "OK") {
                    string to_print; // tmp string that stores the information, in case of a syntax error by the response
                    to_print += "Listing all auctions:\n";
                    for (size_t i = 2; i < response_arguments.size() - 1; i += 2) {
                        // Iterate over the response two arguments by two, corresponding to the pair AID(i) Status(i+1)
                        to_print += "Auction " + response_arguments.at(i) + " ";
                        if (response_arguments.at(i+1) == "0") {
                            to_print += "Ended\n";
                        } else if (response_arguments.at(i+1) == "1") {
                            to_print += "Ongoing\n";
                        } else {
                            throw string("Invalid response from server");
                        }
                    }
                    cout << to_print;
                } else if (response_arguments.at(1) == "ERR") {
                    cout << "Error on server side" << endl;
                } else {
                    throw string("Invalid response from server");
                }
                break;
            }
            case SHOW_RECORD: {
                string aid = arguments.at(1);
                // check argument syntax
                checkAID(aid);

                message = "SRC " + aid + "\n"; // request message
                sendReceiveUDPRequest(message, message.length(), response);

                if (response.size() < 4 || response.back() != '\n') { // check overall size and format
                    throw string("Invalid response from server");
                }
                parseInput(response, response_arguments); // separate by spaces
                if (response_arguments.at(0) == "ERR") {
                    throw string("Error from server side");
                } 
                else if (response_arguments.at(0) != "RRC" || response.at(3) != ' ') {
                    throw string("Invalid response from server");
                }

                if (response_arguments.at(1) == "NOK") {
                    cout << "No auction has such AID" << endl;
                } else if (response_arguments.at(1) == "OK") {
                    string to_print; // tmp string that stores the information, in case of a syntax error by the response

                    string uid = response_arguments.at(2);
                    string auction_name = response_arguments.at(3);
                    string fname = response_arguments.at(4);
                    string start_value = response_arguments.at(5);
                    string date = response_arguments.at(6);
                    string hour = response_arguments.at(7);
                    string duration = response_arguments.at(8);
                    // check response arguments syntax
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

                    size_t index = 8; // index of the current argument being "printed"
                    // 8 is the duration, the next one should be 'B' or 'E'
                    bool end = false; // indicates if we've reached the end of the response
                    bool bids = false; // indicates if we've encountered a bid 
                    string bid_uid;
                    string bid_value;
                    string bid_date;
                    string bid_hour;
                    string bid_duration;
                    string end_date;
                    string end_hour;
                    string end_duration;
                    while (!end) {
                        if (response_arguments.size() - 1 > index) { // check if we've reached the end of the response
                            index++;
                            if (response_arguments[index] == "B") {
                                // Bid's information
                                if (!bids) { // check if we have printed a bid or not
                                    cout << "Listing bids:" << endl;
                                    bids = true; // We've encountered a bid
                                }

                                bid_uid = response_arguments[++index];
                                bid_value = response_arguments[++index];
                                bid_date = response_arguments[++index];
                                bid_hour = response_arguments[++index];
                                bid_duration = response_arguments[++index];
                                // check syntax
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
                                // Auction has ended
                                end_date = response_arguments[++index];
                                end_hour = response_arguments[++index];
                                end_duration = response_arguments[++index];
                                // check syntax
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
                            // Response end reached
                            end = true;
                        }
                    }
                    cout << to_print; // No syntax errors, print everything
                } else if (response_arguments.at(1) == "ERR") {
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
    } catch (string error) {
        cout << error << endl;
    } catch (exception &ex) {
        // catch other exception, e.g., out_of_range caused by syntax error
        cout << "Syntax error on response/request" << endl;
    }
}

// Receives a file from the TCP socket straight into a file (128 bytes at a time)
int receiveTCPfile(int fd, int size, string &fname) {
    int total_received = 0; // number of bytes received
    int n, to_read; // bytes read at a time, bytes to read
    char tmp[128];
    ofstream fout(fname, ios::binary); // create a file to store the data
    if (!fout) {
        throw string("Error creating asset file");
    }

    while (total_received < size) { // Loop until we've received 'size' bytes
        to_read = min(128, size-total_received); // number of bytes to read
        n = read(fd, tmp, to_read);
        if (n == -1) {
            fout.close();
            throw string("Error while reading from TCP socket, probably due to Syntax Error");
        }
        fout.write(tmp, n); // append the data into the file
        total_received += n;
    }
   
    fout.close();
    return total_received;
}

// Creates/Closes TCP connection, sends a request, receives and parses the response
void handleTCPRequest(int request, vector<string> input_arguments) {

    int n;
    string message, tmp;
    vector<string> message_arguments;

    int fd_tcp = socket(AF_INET, SOCK_STREAM, 0);
    if (fd_tcp == -1) {
        return;
    }

    int on = 1; // sets options to reuse socket right away
    setsockopt(fd_tcp, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    // sets read and write timeouts
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
                if (!loggedIn) throw string("User not logged in"); // User has to be logged in

                string auction_name = input_arguments.at(1);
                string fpath = input_arguments.at(2); // full path of the asset
                string start_value = input_arguments.at(3);
                string duration = input_arguments.at(4);

                filesystem::path p(fpath);
                string fname = p.filename(); // asset file name
                // check arguments syntax
                checkName(auction_name);
                checkFileName(fname);
                checkStartValue(start_value);
                checkDuration(duration);

                message = "OPA " + userInfo.at(0) + " " + userInfo.at(1) + " " + auction_name + " ";
                message += start_value + " " + duration + " " + fname + " ";
                message += to_string(filesystem::file_size(fpath)) + " ";
                sendTCPmessage(fd_tcp, message, message.size()); // send the first part of the request

                sendTCPfile(fd_tcp, fpath); // send the asset

                message.clear();
                message = "\n";
                sendTCPmessage(fd_tcp, message, message.size()); // send the final byte

                n = receiveTCPsize(fd_tcp, 4, message); // Receive 4 bytes (could be ERR\n)
                if (message.size() < 4 || message.at(3) != ' ') {
                    throw string("Invalid response from server");
                }
                parseInput(message, message_arguments);
                if (message_arguments.at(0) == "ERR") {
                    throw string("Error from server side, probably it couldn't identify the request");
                }
                else if (message_arguments.at(0) != "ROA") {
                    throw string("Invalid response from server");
                }

                receiveTCPend(fd_tcp, message); // the last byte has to be '\n' otherwise read will throw an error
                parseInput(message, message_arguments); // separate by spaces (should be Status AID)
                if (message_arguments.at(0) == "NOK") {
                    cout << "Auction could not be started" << endl;
                } else if (message_arguments.at(0) == "NLG") {
                    cout << "User not logged in" << endl;
                } else if (message_arguments.at(0) == "OK") {
                    cout << "Auction created with AID " << message_arguments.at(1) << endl;
                } else if (message_arguments.at(0) == "ERR") {
                    cout << "Error on server side" << endl;
                } else {
                    throw string("Invalid response from server");
                }
                break;
            }
            case CLOSE: {
                if (!loggedIn) throw string("User not logged in"); // User has to be logged in

                string aid = input_arguments.at(1);
                // check syntax
                checkAID(aid);

                message = "CLS " + userInfo.at(0) + " " + userInfo.at(1) + " " + aid + "\n"; // request message

                sendTCPmessage(fd_tcp, message, message.size());

                n = receiveTCPsize(fd_tcp, 4, message); // receive 4 bytes (could be ERR\n)
                if (message.size() < 4 || message.at(3) != ' ') {
                    throw string("Invalid response from server");
                }
                parseInput(message, message_arguments);
                if (message_arguments.at(0) == "ERR") {
                    throw string("Error from server side, probably it couldn't identify the request");
                }
                else if (message_arguments.at(0) != "RCL") {
                    throw string("Invalid response from server");
                }

                n = receiveTCPend(fd_tcp, message); // read the rest of the response, last byte has to be '\n'
                if (n > 3) { // status has at max 3 bytes (OK, NLG, EAU, EOW, END, NOK, ERR) not counting '\n'
                    throw string("Invalid response from server");
                }
                parseInput(message, message_arguments);
                if (message_arguments.at(0) == "OK") {
                    cout << "Auction created by the user has now been closed" << endl;
                } else if (message_arguments.at(0) == "NLG") {
                    cout << "User not logged in" << endl;
                } else if (message_arguments.at(0) == "EAU") {
                    cout << "No auction with such AID" << endl;
                } else if (message_arguments.at(0) == "EOW") {
                    cout << "User not the owner of auction" << endl;
                } else if (message_arguments.at(0) == "END") {
                    cout << "Auction has already been closed" << endl;
                } else if (message_arguments.at(0) == "NOK") {
                    cout << "Wrong password" << endl;
                } else if (message_arguments.at(0) == "ERR") {
                    cout << "Error on server side" << endl;
                } else {
                    throw string("Invalid response from server");
                }
                break;
            }
            case SHOW_ASSET: {
                string aid = input_arguments.at(1);
                checkAID(aid); // check syntax

                message =  "SAS " + aid + "\n"; // request message
                sendTCPmessage(fd_tcp, message, message.size());

                n = receiveTCPsize(fd_tcp, 4, message); // receive 4 bytes (could be ERR\n)
                if (message.size() < 4 || message.at(3) != ' ') {
                    throw string("Invalid response from server");
                }
                parseInput(message, message_arguments);
                if (message_arguments.at(0) == "ERR") {
                    throw string("Error from server side, probably it couldn't identify the request");
                }
                else if (message_arguments.at(0) != "RSA") {
                    throw string("Invalid response from server");
                }

                n = receiveTCPsize(fd_tcp, 3, message); // receive 3 bytes which includes Status
                parseInput(message, message_arguments);
                if (message_arguments.at(0) == "NOK") {
                    n = receiveTCPend(fd_tcp, message);
                    if (n != 0) { // '\n' should be the last char, so not counting it we should read 0 bytes
                        throw string("Invalid response from server");
                    }
                    cout << "No auction with such AID" << endl;
                } else if (message_arguments.at(0) == "OK") {
                    n = receiveTCPspace(fd_tcp, 2, message); // receive two arguments, corresponding to filename and filesize
                    parseInput(message, message_arguments); // separate them by spaces

                    string fname = message_arguments.at(0);
                    string fsize_str = message_arguments.at(1);
                    // check syntax
                    checkFileName(fname);
                    checkFileSize(fsize_str);

                    ssize_t fsize;
                    stringstream stream(fsize_str); // turn size into int
                    stream >> fsize;

                    n = receiveTCPfile(fd_tcp, fsize, fname); // receive the asset file and store it directly into a file
                    n = receiveTCPend(fd_tcp, message); // receive the final byte '\n'
                    if (n != 0) { // '\n' should be the last char, so not counting it we should read 0 bytes
                        removeFile(fname); // remove the asset file received previously
                        throw string("Invalid response from server");
                    }

                    cout << "Asset received successfully" << endl;
                }
                break;
            }
            case BID: {
                if (!loggedIn) throw string("User not logged in"); // User has to be logged in

                string aid = input_arguments.at(1);
                string value = input_arguments.at(2);
                // check syntax
                checkAID(aid);
                checkStartValue(value);

                message = "BID " + userInfo.at(0) + " " + userInfo.at(1) + " " + aid + " " + value + "\n"; // request message
                sendTCPmessage(fd_tcp, message, message.size());

                n = receiveTCPend(fd_tcp, message); // receive the whole response
                // number of bytes read should be either 3->ERR or 7 (not counting the '\n')
                if (n == 3 && message == "ERR") {
                    throw string("Error from server side");
                } else if (n != 7) {
                    throw string("Invalid response from server");
                }

                parseInput(message, message_arguments); // separate by spaces
                if (message_arguments.at(1) == "NOK") {
                    cout << "Given auction doesn't exist/ is not active/ wrong password" << endl;
                } else if (message_arguments.at(1) == "ACC") {
                    cout << "Bid has been accepted" << endl;
                } else if (message_arguments.at(1) == "REF") {
                    cout << "Bid has been refused" << endl;
                } else if (message_arguments.at(1) == "ILG") {
                    cout << "Cannot bid in an auction hosted by the user" << endl;
                } else if (message_arguments.at(1) == "ERR") {
                    cout << "Error on server side" << endl;
                } else if (message_arguments.at(1) == "NLG") {
                    cout << "User not logged in" << endl;
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
    } catch (exception &ex) {
        cout << "Syntax error on request/response" << endl;
    }
    
    close(fd_tcp);
    return;
}

int main(int argc, char *argv[]) {

    // ignore signal from writing to socket
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = SIG_IGN;
    if (sigaction(SIGPIPE, &act, NULL) == -1) {
        exit(EXIT_FAILURE);   
    }

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
    // set read/send timeout 
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
    
    string input;
    vector<string> input_arguments;
    while (true) { // Loop to receive commands
        cout << "> ";
        input.clear();
        input_arguments.clear();
        getline(cin, input); // read from stdin

        if (input.size() == 0) continue;

        parseInput(input, input_arguments); // separate by spaces

        int request = parseCommand(input_arguments.at(0)); // get the request type
        if (request > 0 && request < 8) {
            // UDP request
            handleUDPRequest(request, input_arguments);
        } else if (request >= 8) {
            // TCP request
            handleTCPRequest(request, input_arguments);
        } else if (request == 0) {
            // exit request
            if (loggedIn) {
                // if logged in, logout first
                input_arguments.clear();
                input_arguments.push_back(" ");
                input_arguments.push_back(userInfo.at(0));
                input_arguments.push_back(userInfo.at(1));
                handleUDPRequest(LOGOUT, input_arguments);
            }
            return EXIT_SUCCESS; 
        } else {
            cout << "No such request\n";
        }
    }
}