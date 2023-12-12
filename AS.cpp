#include "AS.hpp"
#include "common.hpp"
// -2 for no_error throw (not used anymore)
// handle signal child (done)
// verify if ifstream ofstream have opened correctly (done)
// use remove_all to delete everything in a folder (prolly done)
// add syntax if anything goes wrong change it to false (change to try catch)
// syntax error or error creating/removing files (change to try catch)
// maybe change parseInput to splitString cause it is used in other cases
// If error or syntax send Err or Syn (wrong)
// implement try catch? (done on as)
// check auction duration on close, show asset, bid (done)
// bids always have 6 chars? (not necessary?)
// XXX ERR on tcp (done)
// add timeout to sockets and throws there
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
SharedAID *sharedAID;


bool exists(string& name) {
    // check if dir/file exists
    struct stat buffer;   
    return (stat(name.c_str(), &buffer) == 0); 
}

void removeFile(string &path) {
    error_code ec;
    int ret = filesystem::remove(path, ec);

    if (!ec) { 
        if (!ret) {
            //cout << "[LOG]: File didn't exist" << endl;
            //no_error = false;
            throw string("[IO ERROR]: File didn't exist\n");  
        }
    } 
    else {  
        string tmp = "[IO ERROR]: File " + path + " removed unsuccessful: " + to_string(ec.value()) + " " + ec.message() + "\n";
        //cout << "[LOG]: File " << path << " removed unsuccessful: " << ec.value() << " " << ec.message() << endl;
        //no_error = false;
        throw tmp;
    }
}

void removeDir(string &path) {
    error_code ec;
    
    int ret = filesystem::remove_all(path, ec);
    if (!ec) { 
        if (!ret) {
            //cout<< "[LOG]: File didn't exist" << endl;
            //no_error = false;
            throw string("[IO ERROR]: File didn't exist");  
        }
    } 
    else {  
        string tmp = "[IO ERROR]: File " + path + " removed unsuccessful: " + to_string(ec.value()) + " " + ec.message() + "\n";
        //cout << "[LOG]: File " << path << " removed unsuccessful: " << ec.value() << " " << ec.message() << endl;
        //no_error = false;
        throw tmp;
    }
}

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

bool checkPassword(string &uid, string &pw) {
    string fname = "USERS/" + uid + "/" + uid + "_pass.txt";

    ifstream pw_file(fname);
    if (!pw_file) {
        // cout << "[LOG]: Couldn't open user password file" << endl;
        // no_error = false;
        throw string("[LOG]: Couldn't open user password file");
        // return true;
    }
    
    ostringstream oss;
    oss << pw_file.rdbuf();
    pw_file.close();
    return oss.str() == pw;
}

bool createLogin(string &uid, string &pass) {
    // false if incorrect password
    string loginName;

    if (!checkPassword(uid, pass)) {
        cout << "[LOG]: Incorrect password on login" << endl;
        return false;
    }

    loginName = "USERS/" + uid + "/" + uid + "_login.txt";
    ofstream fout(loginName, ios::out);
    if (!fout) {
        // cout << "[LOG]: Couldn't create login file" << endl;
        // no_error = false;
        throw string("[LOG]: Couldn't create login file");
        // return true;
    }
    cout << "[LOG]: User " + uid << " logged in" << endl;
    fout.close();
    return true;
}

bool removeLogin(string &uid, string &pass) {
    // 0 if pass is ok, -1 otherwise
    // other errors are carried by syntax and no_error;
    string loginName;

    if (!checkPassword(uid, pass)) {
        cout << "[LOG]: Incorrect password" << endl;
        return false;
    }

    loginName = "USERS/" + uid + "/" + uid + "_login.txt";
    removeFile(loginName);
    
    cout << "[LOG]: UDP User " + uid + " logged out successfully" << endl;
    return true;
}

void Register(string &uid, string &pass) {
    // it just tries to register, if an IO error occurs throws error
    string userDir, userPass, hostedDir, biddedDir;
    int ret;

    userDir = "USERS/" + uid;
    ret = mkdir(userDir.c_str(), 0700);
    if (ret == -1) {
        // no_error = false;
        // cout << "[LOG]: Couldn't create user directory" << endl;
        throw string("[LOG]: Couldn't create user directory");
        // return false;
    }

    hostedDir = userDir + "/HOSTED";
    ret = mkdir(hostedDir.c_str(), 0700);
    if (ret == -1) {
        // cout << "[LOG]: Couldn't create hosted directory upon registration" << endl;
        removeDir(userDir);
        throw string("[LOG]: Couldn't create hosted directory upon registration");
        // return false;
    }

    biddedDir = userDir + "/BIDDED";
    ret = mkdir(biddedDir.c_str(), 0700);
    if (ret == -1) {
        //cout << "[LOG]: Couldn't create bidded directory upon registration" << endl;
        removeDir(userDir);
        throw string("[LOG]: Couldn't create bidded directory upon registration");
        //return false;
    }

    userPass = "USERS/" + uid + "/" + uid + "_pass.txt";
    ofstream fout(userPass, ios::out);
    if (!fout) {
        // cout << "[LOG]: Couldn't create user password file" << endl;
        removeDir(userDir);
        throw string("[LOG]: Couldn't create bidded directory upon registration");
        //return false;
    }
    fout << pass;
    cout << "[LOG]: User " + uid + " registered with password " << pass << endl;
    fout.close();
}

bool auctionEnded(string const &aid) {
    string endedTxt = "AUCTIONS/" + aid + "/END_" + aid + ".txt";
    return exists(endedTxt);
}

void endAuction(string const &aid, int const &itime) {
    string endTxt = "AUCTIONS/" + aid + "/END_" + aid + ".txt";
    ofstream fout(endTxt);
    if (!fout) {
        // cout << "[LOG]: Couldn't create END AUCTION text file" << endl;
        // no_error = false;
        throw string("[LOG]: Couldn't create END AUCTION text file");
    }

    char time_str[30];
    time_t ttime = itime;
    struct tm *end_time = gmtime(&ttime);
    snprintf(time_str, 30, "%4d−%02d−%02d %02d:%02d:%02d",
            end_time->tm_year + 1900, end_time->tm_mon + 1,end_time->tm_mday, 
            end_time->tm_hour , end_time->tm_min , end_time->tm_sec);
    
    string content(time_str);
    content += " " + to_string(itime);
    cout << "[LOG]: Auction should've ended, content is " << content << endl;
    fout << content;
    fout.close();
}

string getTime() {
    time(&fulltime);
    stringstream ss;
    ss << fulltime;
    
    return ss.str();
}

string getDateAndTime() {
    char time_str[30];

    time(&fulltime);
    current_time = gmtime(&fulltime);
    snprintf(time_str, 30, "%4d−%02d−%02d %02d:%02d:%02d",
            current_time->tm_year + 1900, current_time->tm_mon + 1,current_time->tm_mday, 
            current_time->tm_hour , current_time->tm_min , current_time->tm_sec);

    stringstream ss;
    ss << fulltime;
    
    return string(time_str) + " " +  ss.str();
}

bool checkAuctionDuration(string const &aid) {
    // true if auction duration is ok, false if should be closed
    string startTxt = "AUCTIONS/" + aid + "/START_" + aid + ".txt";
    ifstream fin(startTxt);
    string content;
    vector<string> content_arguments;
    getline(fin, content);
    fin.close();

    parseInput(content, content_arguments);
    
    string duration = content_arguments[4];
    string start_fulltime = content_arguments.back();
    string current_fulltime = getTime();

    int iduration = stoi(duration);
    int istart = stoi(start_fulltime);
    int icurrent = stoi(current_fulltime);

    if (icurrent > istart + iduration) {
        endAuction(aid, istart + iduration);
        return false;
    }
    return true;
}

string handleUDPRequest(char request[]) {
    // return string with response
    string response;
    vector<string> request_arguments;
    int request_type;
    // bool syntax = true;
    // bool no_error = true;

    parseInput(request, request_arguments);
    // verificar syntax do request (espacos, \n)
    request_type = parseCommand(request_arguments[0]);

    try {
        switch(request_type) {
            case LOGIN: {
                response = "RLI ";

                string uid = request_arguments[1];
                string pass = request_arguments[2];

                checkUID(uid);
                checkPasswordSyntax(pass);

                string loginDir = "USERS/" + uid;
                string passTxt = loginDir + "/" + uid + "_pass.txt";
                if (exists(loginDir) && exists(passTxt)) {
                    string loginTxt;
                    loginTxt = loginDir + "/" + uid + "_login.txt";
                    if (exists(loginTxt)) {
                        cout << "[LOG]: User already logged in" << endl;
                        response += "OK\n";
                    } else if (createLogin(uid, pass)) {
                        response += "OK\n";
                    } else {
                        response += "NOK\n";
                    }
                }
                else {
                    Register(uid, pass);
                    createLogin(uid, pass);
                    response += "REG\n";
                }
                break;
            }
            case LOGOUT: {
                response = "RLO ";
                string uid = request_arguments[1];
                string pass = request_arguments[2];
                
                checkUID(uid);
                checkPasswordSyntax(pass);

                string loginDir = "USERS/" + uid;
                string passTxt = loginDir + "/" + uid + "_pass.txt";
                if (exists(loginDir) && exists(passTxt)) {
                    string loginTxt;
                    loginTxt = loginDir + "/" + uid + "_login.txt";
                    if (!exists(loginTxt)) {
                        cout << "[LOG]: User not logged in" << endl;
                        response += "NOK\n";
                    } else if (!removeLogin(uid, pass)) {
                        response += "NOK\n";
                    } else {
                        response += "OK\n";
                    }
                } else {
                    cout << "[LOG]: User not yet registered" << endl;;
                    response += "UNR\n";
                }
                break;
            }
            case UNREGISTER: {
                response = "RUR ";
                string uid = request_arguments[1];
                string pass = request_arguments[2];

                checkUID(uid);
                checkPasswordSyntax(pass);

                string loginDir = "USERS/" + uid;
                string passTxt = loginDir + "/" + uid + "_pass.txt";
                if (exists(loginDir) && exists(passTxt)) {
                    string loginTxt = loginDir + "/" + uid + "_login.txt";
                    if (exists(loginTxt)) {
                        removeFile(loginTxt);
                        removeFile(passTxt);
                        response += "OK\n";
                    } else {
                        cout << "[LOG]: User not logged in" << endl;
                        response += "NOK\n";
                    }
                } else {
                    cout << "[LOG]: User not registered" << endl;
                    response += "UNR\n";
                }
                break;
            }
            case MYAUCTIONS: {
                response = "RMA ";
                string uid = request_arguments[1];

                checkUID(uid);

                string hostedDir = "USERS/" + uid + "/HOSTED";
                string loginTxt = "USERS/" + uid + "/" + uid + "_login.txt";
            
                if (filesystem::is_empty(hostedDir)) {
                    response += "NOK\n";
                } else if (!exists(loginTxt)) {
                    response += "NLG\n";
                } else {
                    response += "OK ";

                    string aid;
                    for (auto const &entry : filesystem::directory_iterator(hostedDir)) {
                        aid = getSubString(entry.path().string(), 20, 3);
                        if (auctionEnded(aid) || !checkAuctionDuration(aid)) {
                            // terminou ou ja esta fora de prazo
                            response += aid + " 0 ";
                        } else {
                            response += aid + " 1 ";
                        }
                    }
                    response += "\n";
                }
                break;
            }
            case MYBIDS: {
                response = "RMB ";
                string uid = request_arguments[1];

                checkUID(uid);

                string biddedDir = "USERS/" + uid + "/BIDDED";
                string loginTxt = "USERS/" + uid + "/" + uid + "_login.txt";
                if (filesystem::is_empty(biddedDir)) {
                    response += "NOK\n";
                } else if (!exists(loginTxt)) {
                    response += "NLG\n";
                } else {
                    response += "OK ";

                    string aid;
                    for (auto const &entry : filesystem::directory_iterator(biddedDir)) {
                        aid = getSubString(entry.path().string(), 20, 3);
                        if (auctionEnded(aid) || !checkAuctionDuration(aid)) {
                            // terminou ou ja esta fora de prazo
                            response += aid + " 0 ";
                        } else {
                            response += aid + " 1 ";
                        }
                    }
                    response += "\n";
                }
                break;
            }
            case LIST:{
                response = "RLS ";
                string auctionsDir = "AUCTIONS";
                if (filesystem::is_empty(auctionsDir)) {
                    response += "NOK\n";
                } else {
                    response += "OK ";

                    string aid;
                    for (auto const &entry : filesystem::directory_iterator(auctionsDir)) {
                        aid = getSubString(entry.path().string(), 9, 3);
                        if (auctionEnded(aid) || !checkAuctionDuration(aid)) {
                            // terminou ou ja esta fora de prazo
                            response += aid + " 0 ";
                        } else {
                            response += aid + " 1 ";
                        }
                    }
                    response += "\n";
                }
                break;
            }
            case SHOW_RECORD: {
                response = "RRC ";
                string aid = request_arguments[1];
                checkAID(aid);

                string auctionDir = "AUCTIONS/" + aid;
                if (!exists(auctionDir)) {
                    response += "NOK\n";
                } else {
                    response += "OK ";

                    string startTxt = auctionDir + "/START_" + aid + ".txt";
                    ifstream fin(startTxt);
                    if (!fin) {
                        //cout << "[LOG]: Couldn't open start auction text" << endl;
                        //no_error = false;
                        throw string("[LOG]: Couldn't open start auction text");
                    }

                    string content;
                    vector<string> content_arguments;
                    getline(fin, content);
                    parseInput(content, content_arguments);
                    fin.close();

                    string uid = content_arguments[0];
                    string auction_name = content_arguments[1];
                    string fname = content_arguments[2];
                    string start_value = content_arguments[3];
                    string duration = content_arguments[4];
                    string date = content_arguments[5];
                    string time = content_arguments[6];

                    response += uid + " ";
                    response += auction_name + " ";
                    response += fname + " ";
                    response += start_value + " ";
                    response += date + " ";
                    response += time + " ";
                    response += duration + "\n";

                    // 50 maior bids = 50 bids mais recentes?
                    string bidsDir = auctionDir + "/BIDS";
                    if (!filesystem::is_empty(bidsDir)) {
                        vector<string> bidsPath;

                        // obter vetor com todas as bids
                        for (auto const &entry : filesystem::directory_iterator(bidsDir)) {
                            bidsPath.push_back(entry.path().string());
                        }

                        // ordena las decrescente
                        sort(bidsPath.rbegin(), bidsPath.rend());
                        int n = min(int(bidsPath.size()), 50);
                        vector<string> sortedBidsPath(bidsPath.begin(), bidsPath.begin() + n);
                        // obter as 50 bids mais recentes/maiores

                        for (int i = sortedBidsPath.size()-1; i >= 0; i--) {
                            ifstream fin(sortedBidsPath.at(i));
                            if (!fin) {
                                // cout << "[LOG]: Couldn't open bid file on show_record" << endl;
                                // no_error = false;
                                // break;
                                throw string("[LOG]: Couldn't open bid file on show_record");
                            }
                            getline(fin, content);
                            parseInput(content, content_arguments);
                            fin.close();

                            string uid = content_arguments[0];
                            string value = content_arguments[1];
                            string date = content_arguments[2];
                            string time = content_arguments[3];
                            string seconds = content_arguments[4];
                            response += "B ";
                            response += uid + " ";
                            response += value+ " ";
                            response += date + " ";
                            response += time + " ";
                            response += seconds;

                            if (i == 0) {
                                response += "\n";
                            } else {
                                response += " ";
                            }
                        }
                    }

                    string endTxt = auctionDir + "/END_" + aid + ".txt";
                    if (exists(endTxt)) {
                        ifstream fin(endTxt);
                        if (!fin) {
                            // cout << "[LOG]: Couldn't open bid file on show_record" << endl;
                            // no_error = false;
                            // break;
                            throw string("[LOG]: Couldn't open bid file on show_record");
                        }
                        getline(fin, content);
                        parseInput(content, content_arguments);
                        fin.close();

                        string date = content_arguments[0];
                        string time = content_arguments[1];
                        string duration = content_arguments[2];

                        response += "E ";
                        response += date + " ";
                        response += time + " ";
                        response += duration + "\n";
                    }
                }
                break;
            }
            default:
                cout << "[LOG]: Request Syntax error, couldn't identified it" << endl;
                response = "ERR\n";
        }
    }
    catch(string error)
    {
        cout << error << endl;
        response += "ERR\n";
    }

    return response;
}

void startUDP() {
    fd_udp = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd_udp == -1) {
        exit(1);
    }

    n_udp = ::bind(fd_udp, res->ai_addr, res->ai_addrlen);
    if (n_udp == -1) {
        cout << "bind" << strerror(errno);
        exit(1);
    }

    cout << "[LOG]: UDP starting to read from socket" << endl;
    string response;
    while (true) {
        addrlen = sizeof(addr);
        n_udp = recvfrom(fd_udp, buffer, BUFFERSIZE, 0, (struct sockaddr*) &addr, &addrlen);
        if (n_udp == -1) {
            exit(1);
        }

        // string message = "Server received: ";
        // message += buffer;
        // cout << message;

        response.clear();
        response = handleUDPRequest(buffer);
        cout << "[LOG]: UDP sending response: " << response;
        n_udp = sendto(fd_udp, response.c_str(), response.length(), 0, (struct sockaddr*) &addr, addrlen);
        if (n_udp == -1) {
            cout << "Error sending response to UDP socket" << endl;
        }
    }
}


// throw on send and receive
int receiveTCPsize(int fd, int size, string &response) {
    int total_received = 0;
    int n;
    char tmp[128];
    response.clear();
    cout << "[LOG]: " << getpid() << " Receiving TCP request by size" << endl;
    while (total_received < size) {
        n = read(fd, tmp, 1);
        if (n == -1) {
            throw string("Error reading from TCP socket");
            // cout << "TCP receive error" << endl;
            // break;
        }
        concatenateString(response, tmp, n);
        total_received += n;
    }
    cout << "[LOG]: " << getpid() << " Received response of size " << total_received << endl;

    return total_received;
}

int receiveTCPspace(int fd, int size, string &response) {
    int total_received = 0;
    int total_spaces = 0;
    int n;
    char tmp[128];
    response.clear();
    cout << "[LOG]: " << getpid() << " Receiving TCP request by spaces" << endl;
    while (total_spaces < size) {
        n = read(fd, tmp, 1);
        if (n == -1) {
            throw string("Error reading from TCP socket");
            // cout << "TCP receive error" << endl;
            // break;
        }
        concatenateString(response, tmp, n);
        total_received += n;
        if (tmp[0] == ' ') {
            total_spaces++;
        }
    }
    cout << "[LOG]: " << getpid() << " Received response of size " << total_received << endl;

    return total_received;
}

int receiveTCPend(int fd, string &response) {
    int total_received = 0;
    char tmp[128];
    int n;
    response.clear();
    
    cout << "[LOG]: Receiving TCP until the end" << endl;
    while (true) {
        n = read(fd, tmp, 1);
        if (n == -1) {
            throw string("Error reading from TCP socket");
            // cout << "TCP receive error" << endl;
            // break;
        } else if (tmp[0] == '\n') {
            break;
        }
        concatenateString(response, tmp, n);
        total_received += n;
        
    }
    cout << "[LOG]: " << getpid() << " Received response of size " << total_received << endl;

    return total_received;
}

int receiveTCPfile(int fd, int size, string &fname, string &aid) {
    int total_received = 0;
    int n, to_read;
    char tmp[128];
    string auctionDir = "AUCTIONS/" + aid;
    string dir = auctionDir + "/ASSET/" + fname;

    ofstream fout(dir, ios::binary);
    if (!fout) {
        removeDir(auctionDir);
        throw string("Error creating asset file");
    }

    cout << "[LOG]: " << getpid() << " Receiving TCP file" << endl;
    while (total_received < size) {
        to_read = min(128, size-total_received);
        n = read(fd, tmp, to_read);
        if (n == -1) {
            fout.close();
            removeDir(auctionDir);
            throw string("Error receiving TCP file");
        }
        fout.write(tmp, n);
        total_received += n;
    }
    cout << "[LOG]: " << getpid() << " Received file of size " << total_received << " fsize is " << size << endl;
    fout.close();
    
    // filesystem::resize_file(dir, size);

    return total_received;
}

int sendTCPresponse(int fd, string &message, int size) {
    cout << "[LOG]: " << getpid() << " Sending TCP responde" << endl;
    int total_sent = 0;
    int n;
    while (total_sent < size) {
        n = write(fd, message.c_str() + total_sent, size - total_sent);
        if (n == -1) {
            cout << "TCP send error" << endl;
            return n;
        }
        total_sent += n;
    }
    cout << "[LOG]: " << getpid() << " Sent TCP response" << endl;
    return total_sent;
}

bool checkLogin (string &uid) {
    // checks if user is logged in
    struct stat buffer;
    string tmp = "USERS/" + uid + "/" + uid + "_login.txt";
    return (stat (tmp.c_str(), &buffer) == 0); 
}

void createAuctionDir(string &aid) {
    // if error throws
    string AID_dirname = "AUCTIONS/" + aid;
    string BIDS_dirname = "AUCTIONS/" + aid + "/BIDS";
    string ASSET_dirname = "AUCTIONS/" + aid + "/ASSET";
    int ret;

    ret = mkdir(AID_dirname.c_str(), 0700);
    if (ret == -1) {
        //cout << "Error mkdir" << endl;
        //no_error = false;
        throw string("Error creating directory AID");
    }

    ret = mkdir(BIDS_dirname.c_str(), 0700);
    if (ret == -1) {
        removeDir(AID_dirname);
        throw string("Error creating directory AID/BIDS");
        //no_error = false;
        //return -1;
    }

    ret = mkdir(ASSET_dirname.c_str(), 0700);
    if (ret == -1) {
        removeDir(AID_dirname);
        throw string("Error creating directory AID/ASSET");
        //no_error = false;
        //return -1;
    }
}

void deleteAuctionDir(string &aid) {
    string AID_dirname = "AUCTIONS/" + aid;
    removeDir(AID_dirname);
}

string getNextAID(SharedAID *sharedAID) {
    // char tmp[10];
    sem_wait(&sharedAID->sem);

    stringstream ss;
    ss << setw(3) << setfill('0') << sharedAID->AID;
    string aid = ss.str();
    sharedAID->AID++;

    sem_post(&sharedAID->sem);

    return aid;
}



void createStartAuctionText(vector<string> &arguments, string &aid) {
    string name, tmp;

    tmp = arguments[0] + " "; // UID
    tmp += arguments[2] + " "; // Name
    tmp += arguments[5] + " "; // fname
    tmp += arguments[3] + " "; // start_value
    tmp += arguments[4] + " "; // time active
    tmp += getDateAndTime();

    name = "AUCTIONS/" + aid + "/START_" + aid + ".txt";
    ofstream fout(name, ios::out);
    if (!fout) {
        //cout << "Error creating start auction text" << endl;
        deleteAuctionDir(aid);
        throw string("Error creating start auction text");
        //return -1;
    }
    fout.write(tmp.c_str(), tmp.size());
    fout.close();
}

bool checkOwner(string &uid, string &aid) {
    string auctionDir = "AUCTIONS/" + aid;
    string startTxt = auctionDir + "/START_" + aid + ".txt";
    char tmp[6];

    ifstream fin(startTxt);
    if (!fin) {
        throw string("[LOG]: Error opening file to read");
        // cout << "[LOG]: Error opening file to read" << endl;
        // no_error = false;
        // return true;
    }
    fin.read(tmp, 6);
    fin.close();
    string targetUID(tmp);

    return uid == targetUID;
}

void handleTCPRequest(int &fd, SharedAID *sharedAID) {
    cout << "[LOG]: Got one request child " << getpid() << " handling it" << endl;
    string request, tmp, response;
    // bool syntax = true;
    // bool no_error = true;
    vector<string> request_arguments;
    int request_type;
    try
    {
        receiveTCPsize(fd, 3, tmp);
        request_type = parseCommand(tmp);
        receiveTCPsize(fd, 1, tmp); // clear space
        if (tmp.at(0) != ' ') {
            throw string("Syntax Error");
        } else {
            switch (request_type) {
                case OPEN: {
                    response = "ROA ";

                    receiveTCPspace(fd, 7, request);
                    parseInput(request, request_arguments);

                    ssize_t fsize;
                    stringstream stream(request_arguments[6]);
                    stream >> fsize;

                    checkUID(request_arguments[0]);
                    checkPasswordSyntax(request_arguments[1]);
                    checkName(request_arguments[2]);
                    checkStartValue(request_arguments[3]);
                    checkDuration(request_arguments[4]);
                    checkFileName(request_arguments[5]);
                    checkFileSize(request_arguments[6]);

                    // bool ok = true;
                    if (!checkLogin(request_arguments[0])) {
                        cout << "[LOG]: User not logged in" << endl;
                        response += "NLG\n";
                    } else if (!checkPassword(request_arguments[0], request_arguments[1])) {
                        cout << "[LOG]: Incorrect password" << endl;
                        response += "NOK\n";
                    } else {
                        string aid = getNextAID(sharedAID);

                        createAuctionDir(aid);
                        receiveTCPfile(fd, fsize, request_arguments[5], aid);
                        receiveTCPsize(fd, 1, tmp);
                        if (tmp.at(0) != '\n') {
                            deleteAuctionDir(aid);
                            throw string("Open request with syntax error");
                        }
                        createStartAuctionText(request_arguments, aid);

                        tmp = "USERS/" + request_arguments[0] + "/HOSTED/" + aid + ".txt"; // create hosted in user folder
                        ofstream fout(tmp);
                        if (!fout) {
                            deleteAuctionDir(aid);
                            throw string("Couldn't create hosted file in User dir");
                        }
                        fout.close();
                        cout << "[LOG]: " << getpid() << " Created auction with aid " << aid << endl;
                        response += "OK " + aid + "\n";
                    }
                    break;
                }
                case CLOSE: {
                    response = "RCL ";
                    int n = receiveTCPsize(fd, 20, request);
                    if (n < 20 || request.back() != '\n') {
                        throw string("Syntax error on close request");
                    }
                    parseInput(request, request_arguments);

                    string uid = request_arguments[0];
                    string pass = request_arguments[1];
                    string aid = request_arguments[2];
                    checkUID(uid);
                    checkPasswordSyntax(pass);
                    checkAID(aid);

                    string auctionDir = "AUCTIONS/" + aid;
                    string endTxt = auctionDir + "/END_" + aid + ".txt";
                    if (!checkLogin(uid)) {
                        cout << "[LOG]: On close User not logged in" << endl;
                        response += "NLG\n";
                    } else if (!exists(auctionDir)) {
                        cout << "[LOG]: On close auction does not exist" << endl;
                        response += "EAU\n";
                    } else if (!checkAuctionDuration(aid) || exists(endTxt)) {
                        cout << "[LOG]: On close auction already closed" << endl;
                        response += "END\n";
                    } else if (!checkOwner(uid, aid)) {
                        cout << "[LOG]: On close auction not owned by given uid" << endl;
                        response += "EOW\n";
                    } else if (!checkPassword(uid, pass)) {
                        cout << "[LOG]: On close uid password don't match" << endl;
                        response += "NOK\n";
                    } else {
                        string content = getDateAndTime();

                        ofstream fout(endTxt);
                        if (!fout) {
                            throw string("Error creating end text file");
                            // cout << "[LOG]: Error opening file to write" << endl;
                            // no_error = false;
                            // break;
                        }
                        fout << content;
                        fout.close();

                        response += "OK\n";
                    }
                    break;
                }
                case SHOW_ASSET: {
                    response = "RSA ";
                    int n = receiveTCPsize(fd, 4, request);
                    if (n < 4 || request.back() != '\n') {
                        throw string("Syntax error on show_asset request");
                    }
                    parseInput(request, request_arguments);

                    string aid = request_arguments[0];
                    checkAID(aid);

                    string auctionDir = "AUCTIONS/" + aid;
                    if (!exists(auctionDir)) {
                        cout << "[LOG]: No auction with such aid" << endl;
                        response += "NOK\n";
                        break;
                    }
                    
                    checkAuctionDuration(aid);

                    string assetDir = "AUCTIONS/" + aid + "/ASSET";
                    string assetPath = filesystem::directory_iterator(assetDir)->path().string();

                    string fname = getSubString(assetPath, 19, 24);

                    ssize_t fsize = filesystem::file_size(assetPath);
                    string fsize_str = to_string(fsize);

                    response += "OK ";
                    response += fname + " ";
                    response += fsize_str + " ";
                    response += openJPG(assetPath) + "\n";
                    break;
                }
                case BID:{
                    response = "RBD ";
                    int n = receiveTCPspace(fd, 3, request);
                    if (n != 20) {
                        throw string("Syntax error on bid reequest");
                        // cout << "[LOG]: Bid request of size < 20" << endl;
                        // syntax = false;
                        // break;
                    }
                    parseInput(request, request_arguments);

                    string value;
                    receiveTCPend(fd, value);
                    if (value.size() > 6) {
                        throw string("Syntax error on bid request");
                        // cout << "[LOG]: Bid value > 6" << value << endl;
                        // syntax = false;
                        // break;
                    }

                    string uid = request_arguments[0];
                    string pass = request_arguments[1];
                    string aid = request_arguments[2];
                    checkUID(uid);
                    checkPasswordSyntax(pass);
                    checkAID(aid);

                    string auctionDir = "AUCTIONS/" + aid;
                    string endTxt = auctionDir + "/END_" + aid + ".txt";
                    if (!checkPassword(uid, pass)) {
                        cout << "[LOG]: On bid, wrong password given for user";
                        response += "NOK\n";
                    } else if (!exists(auctionDir)) {
                        cout << "[LOG]: On bid, no auction with such aid" << endl;
                        response += "NOK\n";
                    } else if (!checkAuctionDuration(aid) || exists(endTxt)) {
                        cout << "[LOG]: On bid, auction already closed" << endl;
                        response += "NOK\n";
                    } else if (!checkLogin(uid)) {
                        cout << "[LOG]: On bid, user not logged in" << endl;
                        response += "NLG\n";
                    } else {
                        string bidsDir = auctionDir + "/BIDS";
                        string highest_value = "0";
                        if (!filesystem::is_empty(bidsDir)) {
                            vector<string> bidsPath;

                            for (auto const &entry : filesystem::directory_iterator(bidsDir)) {
                                bidsPath.push_back(entry.path().string());
                            }

                            sort(bidsPath.begin(), bidsPath.end());
                            string highestBid = bidsPath.back();
                            
                            string content;
                            vector<string> content_arguments;
                            ifstream fin(highestBid);
                            if (!fin) {
                                throw string("Couldn't open bid file to read");
                                // cout << "[LOG]: Couldn't open bid file on bid" << endl;
                                // no_error = false;
                                // break;
                            }
                            getline(fin, content);
                            fin.close();
                            parseInput(content, content_arguments);
                            highest_value = content_arguments[1];
                        }

                        if (value.compare(highest_value) <= 0) {
                            cout << "[LOG]: On bid, new bid is not higher than the current highest bid" << endl;
                            response += "REF\n";
                            break;
                        }

                        char tmp[6];
                        string startTxt = auctionDir + "/START_" + aid + ".txt";
                        ifstream fin(startTxt);
                        if (!fin) {
                            throw string("Couldn't open bid file to read");
                            // cout << "[LOG]: Couldn't open bid file on bid" << endl;
                            // no_error = false;
                            // break;
                        }
                        fin.read(tmp, 6);
                        string auctionOwner(tmp);

                        if (auctionOwner == uid) {
                            cout << "[LOG]: On bid, bid on own auction" << endl;
                            response += "ILG\n";
                            break;
                        }

                        string bidTxt = auctionDir + "/BIDS/" + value + ".txt";
                        ofstream fout(bidTxt);
                        if (!fout) {
                            throw string("Couldn't open file to write on bid");
                            // cout << "[LOG]: Couldn't open file to write on bid" << endl;
                            // no_error = false;
                            // break;
                        }

                        string content = uid + " ";
                        content += value + " ";
                        content += getDateAndTime();
                        fout << content;
                        fout.close();

                        string userBidTxt = "USERS/" + uid + "/BIDDED/" + value + ".txt";
                        fout.open(userBidTxt);
                        if (!fout) {
                            removeFile(bidTxt);
                            throw string("Couldn't create bid file to user");
                            // cout << "[LOG]: Couldn't create bid file to user" << endl;
                            // no_error = false;
                            // break;
                        }
                        fout.close();
                        
                        response += "ACC\n";
                    }
                    break;
                }
                default:
                    throw string("Couldn't identify TCP request");
                    // cout << "Syntax error" << endl;
                    // syntax = false;
                    // break;
            }
        }
    }
    catch(string error)
    {
        cout << error << endl;
        response += "ERR\n";
    }

    cout << "[LOG]: " << getpid() << " Sending response " << response;
    sendTCPresponse(fd, response, response.size());

    close(fd);
}

void startTCP(SharedAID *sharedAID) {
    fd_tcp = socket(AF_INET, SOCK_STREAM, 0);
    if (fd_tcp == -1) {
        cout << "TCP socket error" << endl;
        exit(EXIT_FAILURE);
    }

    int on = 1;
    setsockopt(fd_tcp, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    n_tcp = ::bind(fd_tcp, res->ai_addr, res->ai_addrlen);
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
        cout << "[LOG]: Parent " << getpid() << " starting to accept requests" << endl;
        if ( (tcp_child = accept(fd_tcp, (struct sockaddr*) &addr, &addrlen)) == -1) {
            cout << "TCP accept error" << endl;
            exit(EXIT_FAILURE);
        }

        pid_t pid = fork();
        if (pid == -1) {
            cout << "TCP fork error" << endl;
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            int on = 1;
            setsockopt(tcp_child, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
            handleTCPRequest(tcp_child, sharedAID);
            if (shmdt(sharedAID) == -1) {
                cout << "[LOG]: " << getpid() << " erro detaching shared memory" << endl;
            }
            cout << "[LOG]: " << getpid() << " Terminating after handling request" << endl; 
            break;
        }
    }
}

void sigintHandler(int signum) {

    sem_destroy(&sharedAID->sem);
    shmdt(sharedAID);

    string userDir = "USERS";
    for (const auto& entry : filesystem::directory_iterator(userDir)) {
        if (filesystem::exists(entry.path())) {
            filesystem::remove_all(entry.path());
        }
    } 

    string auctionsDir = "AUCTIONS";
    for (const auto& entry : filesystem::directory_iterator(auctionsDir)) {
        if (filesystem::exists(entry.path())) {
            filesystem::remove_all(entry.path());
        }
    } 
        
    exit(0);
}

int main(int argc, char *argv[]) {

    signal(SIGINT, sigintHandler);
    signal(SIGCHLD, SIG_IGN);

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

    key_t key = ftok("SharedData", 0);
    int shmid = shmget(key, sizeof(SharedAID), 0666 | IPC_CREAT);
    sharedAID = (SharedAID*) shmat(shmid, (void*)0, 0);

    sharedAID->AID = 0;
    sem_init(&sharedAID->sem, 1, 1);

    pid_t pid = fork();
    if (pid == -1) {
        cout << "Fork error" << endl;
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        startUDP();
    } else {
        signal(SIGINT, SIG_DFL);
        startTCP(sharedAID);
    }
    
    return 0;
}