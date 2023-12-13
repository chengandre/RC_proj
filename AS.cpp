#include "AS.hpp"
#include "common.hpp"

// remove sigint handler
// maybe change parseInput to splitString cause it is used in other cases
// bids always have 6 chars? (not necessary?)
// sort listing
// ver que auctions ja existem
// ver se ja existe auction com aid antes de atribuir

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
char host[NI_MAXHOST],service[NI_MAXSERV];


bool exists(string& name) {
    // check if dir/file exists
    struct stat buffer;   
    return (stat(name.c_str(), &buffer) == 0); 
}

void removeFile(string &path) {
    // removes a single file
    // throws error to stop request handling
    error_code ec;
    int ret = filesystem::remove(path, ec);

    if (!ec) { 
        if (!ret) {
            throw string("[IO ERROR]: File didn't exist\n");  
        }
    } 
    else {  
        string tmp = "[IO ERROR]: File " + path + " removed unsuccessful: " + to_string(ec.value()) + " " + ec.message() + "\n";
        throw tmp;
    }
}

void removeDir(string &path) {
    error_code ec;
    
    int ret = filesystem::remove_all(path, ec);
    if (!ec) { 
        if (!ret) {
            throw string("[IO ERROR]: File didn't exist");  
        }
    } 
    else {  
        string tmp = "[IO ERROR]: File " + path + " removed unsuccessful: " + to_string(ec.value()) + " " + ec.message() + "\n";
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
        throw string("[LOG]: Couldn't open user password file");
    }
    
    ostringstream oss;
    oss << pw_file.rdbuf();
    pw_file.close();
    return oss.str() == pw;
}

bool createLogin(string &uid, string &pass) {
    // true if pass is correct, else false
    string loginName;

    if (!checkPassword(uid, pass)) {
        cout << "[LOG]: Incorrect password on login" << endl;
        return false;
    }

    loginName = "USERS/" + uid + "/" + uid + "_login.txt";
    ofstream fout(loginName, ios::out);
    if (!fout) {
        throw string("[LOG]: Couldn't create login file");
    }

    cout << "[LOG]: User " + uid << " logged in" << endl;
    fout.close();
    return true;
}

bool removeLogin(string &uid, string &pass) {
    // true if logged out, false is pass is wrong
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
    // checks if auction has already closed
    string endedTxt = "AUCTIONS/" + aid + "/END_" + aid + ".txt";
    return exists(endedTxt);
}

void endAuction(string const &aid, int const &itime) {
    // ends an auction, creating end_aid.txt
    string endTxt = "AUCTIONS/" + aid + "/END_" + aid + ".txt";
    ofstream fout(endTxt);
    if (!fout) {
        throw string("[LOG]: Couldn't create END AUCTION text file");
    }

    char time_str[30];
    time_t ttime = itime;
    struct tm *end_time = gmtime(&ttime);
    snprintf(time_str, 30, "%4d\u002d%02d\u002d%02d %02d:%02d:%02d",
            end_time->tm_year + 1900, end_time->tm_mon + 1,end_time->tm_mday, 
            end_time->tm_hour , end_time->tm_min , end_time->tm_sec);
    
    string content(time_str);
    content += " " + to_string(itime);
    cout << "[LOG]: Auction should've ended, content is " << content << endl;
    fout << content;
    fout.close();
}

string getTime() {
    // Get time in seconds since 1970
    time(&fulltime);
    stringstream ss;
    ss << fulltime;
    
    return ss.str();
}

string getDateAndTime() {
    // Get date,hour and time in seconds
    char time_str[30];

    time(&fulltime);
    current_time = gmtime(&fulltime);
    snprintf(time_str, 30, "%4d\u002d%02d\u002d%02d %02d:%02d:%02d",
            current_time->tm_year + 1900, current_time->tm_mon + 1,current_time->tm_mday, 
            current_time->tm_hour , current_time->tm_min , current_time->tm_sec);

    stringstream ss;
    ss << fulltime;
    
    return string(time_str) + " " +  ss.str();
}

string getDateAndDuration(int &start) {

    char time_str[30];

    time(&fulltime);
    current_time = gmtime(&fulltime);
    snprintf(time_str, 30, "%4d\u002d%02d\u002d%02d %02d:%02d:%02d",
            current_time->tm_year + 1900, current_time->tm_mon + 1,current_time->tm_mday, 
            current_time->tm_hour , current_time->tm_min , current_time->tm_sec);

    int duration = fulltime-start;

    return string(time_str) + " " +  to_string(duration);
}

bool checkAuctionDuration(string const &aid) {
    // true if auction duration is ok
    // false if should be closed and goes calls endAuction
    string startTxt = "AUCTIONS/" + aid + "/START_" + aid + ".txt";
    ifstream fin(startTxt);
    if (!fin) {
        throw string("Couldn't open start txt to read");
    }
    string content;
    vector<string> content_arguments;
    getline(fin, content);
    fin.close();

    parseInput(content, content_arguments);

    string duration = content_arguments.at(4);
    string start_fulltime = content_arguments.back();
    string current_fulltime = getTime();

    int iduration = stoi(duration);
    int istart = stoi(start_fulltime);
    int icurrent = stoi(current_fulltime);

    if (icurrent > istart + iduration) {
        endAuction(aid, iduration);
        return false;
    }
    return true;
}

string handleUDPRequest(char request[]) {

    // handle UDP request from user, return the response to send
    string response;
    vector<string> request_arguments;
    int request_type;

    parseInput(request, request_arguments);
    // verificar syntax do request (espacos, \n)
    request_type = parseCommand(request_arguments.at(0));

    try {
        switch(request_type) {
            case LOGIN: {
                cout << "[LOG]: UDP Got LOGIN request" << endl;
                response = "RLI ";

                string uid = request_arguments.at(1);
                string pass = request_arguments.at(2);

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
                cout << "[LOG]: UDP Got LOGOUT request" << endl;

                response = "RLO ";
                string uid = request_arguments.at(1);
                string pass = request_arguments.at(2);
                
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
                cout << "[LOG]: UDP Got UNREGISTER request" << endl;

                response = "RUR ";
                string uid = request_arguments.at(1);
                string pass = request_arguments.at(2);

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
                cout << "[LOG]: UDP Got MYAUCTIONS request" << endl;

                response = "RMA ";
                string uid = request_arguments.at(1);

                checkUID(uid);

                string hostedDir = "USERS/" + uid + "/HOSTED";
                string loginTxt = "USERS/" + uid + "/" + uid + "_login.txt";
            
                if (filesystem::is_empty(hostedDir)) {
                    response += "NOK\n";
                } else if (!exists(loginTxt)) {
                    response += "NLG\n";
                } else {
                    string tmp_response;
                    tmp_response += "OK"; 

                    vector<string> auctionsPath;
                    for (auto const &entry : filesystem::directory_iterator(hostedDir)) {
                        auctionsPath.push_back(entry.path().string());
                    }
                    sort(auctionsPath.begin(), auctionsPath.end());

                    string aid;
                    for (string &path: auctionsPath) {
                        aid = getSubString(path, 20, 3);
                        if (auctionEnded(aid) || !checkAuctionDuration(aid)) {
                            // terminou ou ja esta fora de prazo
                            tmp_response += " " + aid + " 0";
                        } else {
                            tmp_response += " " + aid + " 1";
                        }
                    }
                    response += tmp_response;
                    response += "\n";
                }
                break;
            }
            case MYBIDS: {
                cout << "[LOG]: UDP Got MYBIDS request" << endl;

                response = "RMB ";
                string uid = request_arguments.at(1);

                checkUID(uid);

                string biddedDir = "USERS/" + uid + "/BIDDED";
                string loginTxt = "USERS/" + uid + "/" + uid + "_login.txt";
                if (filesystem::is_empty(biddedDir)) {
                    response += "NOK\n";
                } else if (!exists(loginTxt)) {
                    response += "NLG\n";
                } else {
                    string tmp_response = "OK";

                    vector<string> bidsPath;
                    for (auto const &entry : filesystem::directory_iterator(biddedDir)) {
                        bidsPath.push_back(entry.path().string());
                    }
                    sort(bidsPath.begin(), bidsPath.end());

                    string aid;
                    for (string &entry : bidsPath) {
                        aid = getSubString(entry, 20, 3);
                        if (auctionEnded(aid) || !checkAuctionDuration(aid)) {
                            // terminou ou ja esta fora de prazo
                            tmp_response += " " + aid + " 0";
                        } else {
                            tmp_response += " " + aid + " 1";
                        }
                    }
                    response += tmp_response;
                    response += "\n";
                }
                break;
            }
            case LIST:{
                cout << "[LOG]: UDP Got LIST request" << endl;

                response = "RLS ";
                string auctionsDir = "AUCTIONS";
                if (filesystem::is_empty(auctionsDir)) {
                    response += "NOK\n";
                } else {
                    string tmp_response = "OK";

                    vector<string> auctionsPath;
                    for (auto const &entry : filesystem::directory_iterator(auctionsDir)) {
                        auctionsPath.push_back(entry.path().string());
                    }
                    sort(auctionsPath.begin(), auctionsPath.end());

                    string aid;
                    for (string &entry : auctionsPath) {
                        aid = getSubString(entry, 9, 3);
                        if (auctionEnded(aid) || !checkAuctionDuration(aid)) {
                            tmp_response += " " + aid + " 0";
                        } else {
                            tmp_response += " " + aid + " 1";
                        }
                    }
                    response += tmp_response;
                    response += "\n";
                }
                break;
            }
            case SHOW_RECORD: {
                cout << "[LOG]: UDP Got SHOW_RECORD request" << endl;
                response = "RRC ";
                string aid = request_arguments.at(1);
                checkAID(aid);
                
                string auctionDir = "AUCTIONS/" + aid;
                if (!exists(auctionDir)) {
                    response += "NOK\n";
                } else {
                    checkAuctionDuration(aid);

                    response += "OK ";

                    string startTxt = auctionDir + "/START_" + aid + ".txt";
                    ifstream fin(startTxt);
                    if (!fin) {
                        throw string("[LOG]: Couldn't open start auction text");
                    }

                    string content;
                    vector<string> content_arguments;
                    getline(fin, content);
                    parseInput(content, content_arguments);
                    fin.close();

                    string uid = content_arguments.at(0);
                    string auction_name = content_arguments.at(1);
                    string fname = content_arguments.at(2);
                    string start_value = content_arguments.at(3);
                    string duration = content_arguments.at(4);
                    string date = content_arguments.at(5);
                    string time = content_arguments.at(6);

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
                                throw string("[LOG]: Couldn't open bid file on show_record");
                            }
                            getline(fin, content);
                            parseInput(content, content_arguments);
                            fin.close();

                            string uid = content_arguments.at(0);
                            string value = content_arguments.at(1);
                            string date = content_arguments.at(2);
                            string time = content_arguments.at(3);
                            string seconds = content_arguments.at(4);
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
                            throw string("[LOG]: Couldn't open bid file on show_record");
                        }
                        getline(fin, content);
                        parseInput(content, content_arguments);
                        fin.close();

                        string date = content_arguments.at(0);
                        string time = content_arguments.at(1);
                        string duration = content_arguments.at(2);

                        response += "E ";
                        response += date + " ";
                        response += time + " ";
                        response += duration + "\n";
                    }
                }
                break;
            }
            default:
                cout << "[LOG]: UDP Request Syntax error, couldn't identified it" << endl;
                response = "ERR\n";
        }
    }
    catch(string error)
    {
        cout << error << endl;
        response += "ERR\n";
    }
    catch (exception& ex) {
        cout << ex.what() << endl;
        response += "ERR\n";
    }

    return response;
}

void startUDP() {

    fd_udp = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd_udp == -1) {
        cout << "[LOG]: UDP Error creating socket" << endl;
        exit(EXIT_FAILURE);
    }
    cout << "[LOG]: UDP socket created" << endl;

    int on = 1;
    if (setsockopt(fd_udp, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1) {
        cout << "[LOG]: TCP Error setting socket options" << endl; 
        exit(EXIT_FAILURE);
    }

    n_udp = ::bind(fd_udp, res->ai_addr, res->ai_addrlen);
    if (n_udp == -1) {
        cout << "[LOG]: UDP Bind error: " << strerror(errno);
        exit(EXIT_FAILURE);
    }
    cout << "[LOG]: UDP Bind successfully" << endl;

    struct timeval timeout;
    timeout.tv_sec = 5; 
    timeout.tv_usec = 0;
    
    if (setsockopt(fd_udp, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) == -1) {
        cout << "[LOG]: UDP Error setting timeout" << endl;
        int ret;
        do {
            ret = close(fd_udp);
        } while (ret == -1 && errno == EINTR);
        exit(EXIT_FAILURE);
    }

    cout << "[LOG]: UDP starting to read from socket" << endl;
    string response;
    while (true) {
        addrlen = sizeof(addr);
        response.clear();
        
        cout << "[LOG]: UDP waiting for requests" << endl;
        n_udp = recvfrom(fd_udp, buffer, BUFFERSIZE, 0, (struct sockaddr*) &addr, &addrlen);
        if (n_udp == -1) {
            cout << "Error reading request from UDP socket" << endl;
            response = "ERR\n";
        } else {
            if ((errcode = getnameinfo((struct sockaddr *) &addr, addrlen, host, sizeof(host), service, sizeof(service), 0)) != 0) {
                cout << "[LOG]: UDP Error getnameinfo: " << gai_strerror(errcode) << endl;
            } else {
                cout << "[LOG]: UDP got a request from " << host << ":" << service << endl;
            }
            response = handleUDPRequest(buffer);
        }

        cout << "[LOG]: UDP sending response: " << response;
        n_udp = sendto(fd_udp, response.c_str(), response.length(), 0, (struct sockaddr*) &addr, addrlen);
        if (n_udp == -1) {
            cout << "Error sending response to UDP socket" << endl;
        }
    }
}

int receiveTCPsize(int fd, int size, string &response) {
    int total_received = 0;
    int n;
    char tmp[128];
    response.clear();
    //cout << "[LOG]: " << getpid() << " Receiving TCP request by size" << endl;
    while (total_received < size) {
        n = read(fd, tmp, 1);
        if (n == -1) {
            throw string("Error reading from TCP socket");
        }
        concatenateString(response, tmp, n);
        total_received += n;
    }
    //cout << "[LOG]: " << getpid() << " Received response of size " << total_received << endl;

    return total_received;
}

int receiveTCPspace(int fd, int size, string &response) {
    int n;
    int total_received = 0;
    int total_spaces = 0;
    char tmp[128];
    response.clear();
    //cout << "[LOG]: " << getpid() << " Receiving TCP request by spaces" << endl;
    while (total_spaces < size) {
        n = read(fd, tmp, 1);
        if (n == -1) {
            throw string("Error reading from TCP socket");
        }
        concatenateString(response, tmp, n);
        total_received += n;
        if (tmp[0] == ' ') {
            total_spaces++;
        }
    }
    //cout << "[LOG]: " << getpid() << " Received response of size " << total_received << endl;

    return total_received;
}

int receiveTCPend(int fd, string &response) {
    int total_received = 0;
    char tmp[128];
    int n;
    response.clear();
    
    //cout << "[LOG]: Receiving TCP until the end" << endl;
    while (true) {
        n = read(fd, tmp, 1);
        if (n == -1) {
            throw string("Error reading from TCP socket");
        } else if (tmp[0] == '\n') {
            break;
        }
        concatenateString(response, tmp, n);
        total_received += n;
        
    }
    //cout << "[LOG]: " << getpid() << " Received response of size " << total_received << endl;

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

    //cout << "[LOG]: " << getpid() << " Receiving TCP file" << endl;
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

    //cout << "[LOG]: " << getpid() << " Received file of size " << total_received << " fsize is " << size << endl;
    fout.close();

    return total_received;
}

int sendTCPresponse(int fd, string &message, int size) {
    //cout << "[LOG]: " << getpid() << " Sending TCP response" << endl;
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

    //cout << "[LOG]: " << getpid() << " Sent TCP response" << endl;
    return total_sent;
}

bool checkLogin (string &uid) {
    // true if user is logged in, else false
    struct stat buffer;
    string tmp = "USERS/" + uid + "/" + uid + "_login.txt";
    return (stat (tmp.c_str(), &buffer) == 0); 
}

void createAuctionDir(string &aid) {
    // creates all the directories necessary for an auction
    string AID_dirname = "AUCTIONS/" + aid;
    string BIDS_dirname = "AUCTIONS/" + aid + "/BIDS";
    string ASSET_dirname = "AUCTIONS/" + aid + "/ASSET";
    int ret;

    ret = mkdir(AID_dirname.c_str(), 0700);
    if (ret == -1) {
        throw string("Error creating directory AID");
    }

    ret = mkdir(BIDS_dirname.c_str(), 0700);
    if (ret == -1) {
        removeDir(AID_dirname);
        throw string("Error creating directory AID/BIDS");
    }

    ret = mkdir(ASSET_dirname.c_str(), 0700);
    if (ret == -1) {
        removeDir(AID_dirname);
        throw string("Error creating directory AID/ASSET");
    }
}

void deleteAuctionDir(string &aid) {
    string AID_dirname = "AUCTIONS/" + aid;
    removeDir(AID_dirname);
}

string getNextAID(SharedAID *sharedAID) {
    // returns the next available AID
    sem_wait(&sharedAID->sem);

    stringstream ss;
    ss << setw(3) << setfill('0') << sharedAID->AID;
    string aid = ss.str();
    sharedAID->AID++;

    sem_post(&sharedAID->sem);

    return aid;
}



void createStartAuctionText(vector<string> &arguments, string &aid) {
    // creates start_aid.txt
    string name, tmp;

    tmp = arguments.at(0) + " "; // UID
    tmp += arguments.at(2) + " "; // Name
    tmp += arguments.at(5) + " "; // fname
    tmp += arguments.at(3) + " "; // start_value
    tmp += arguments.at(4) + " "; // time active
    tmp += getDateAndTime();

    name = "AUCTIONS/" + aid + "/START_" + aid + ".txt";
    ofstream fout(name, ios::out);
    if (!fout) {
        deleteAuctionDir(aid);
        throw string("Error creating start auction text");
    }

    fout.write(tmp.c_str(), tmp.size());
    fout.close();
}

bool checkOwner(string &uid, string &aid) {
    // checks if user is auctions' owner
    string auctionDir = "AUCTIONS/" + aid;
    string startTxt = auctionDir + "/START_" + aid + ".txt";
    char tmp[6];

    ifstream fin(startTxt);
    if (!fin) {
        throw string("[LOG]: Error opening file to read");
    }
    fin.read(tmp, 6);
    fin.close();

    return strcmp(uid.c_str(), tmp);
}

void handleTCPRequest(int &fd, SharedAID *sharedAID) {
    // handles TCP request from user (receives, handles and answers)

    string request, tmp, response;
    vector<string> request_arguments;
    int request_type;
    try {
        receiveTCPsize(fd, 3, tmp);
        request_type = parseCommand(tmp);
        receiveTCPsize(fd, 1, tmp); // clear space
        if (tmp.at(0) != ' ') {
            throw string("Syntax Error");
        } else {
            switch (request_type) {
                case OPEN: {
                    cout << "[LOG]: " << getpid() << " Got OPEN request" << endl;

                    response = "ROA ";

                    receiveTCPspace(fd, 7, request);
                    parseInput(request, request_arguments);

                    ssize_t fsize;
                    stringstream stream(request_arguments.at(6));
                    stream >> fsize;

                    checkUID(request_arguments.at(0));
                    checkPasswordSyntax(request_arguments.at(1));
                    checkName(request_arguments.at(2));
                    checkStartValue(request_arguments.at(3));
                    checkDuration(request_arguments.at(4));
                    checkFileName(request_arguments.at(5));
                    checkFileSize(request_arguments.at(6));

                    if (!checkLogin(request_arguments.at(0))) {
                        cout << "[LOG]: User not logged in" << endl;
                        response += "NLG\n";
                    } else if (!checkPassword(request_arguments.at(0), request_arguments.at(1))) {
                        cout << "[LOG]: Incorrect password" << endl;
                        response += "NOK\n";
                    } else {
                        string aid = getNextAID(sharedAID);

                        createAuctionDir(aid);
                        receiveTCPfile(fd, fsize, request_arguments.at(5), aid);
                        receiveTCPsize(fd, 1, tmp);
                        if (tmp.at(0) != '\n') {
                            deleteAuctionDir(aid);
                            throw string("Open request with syntax error");
                        }
                        createStartAuctionText(request_arguments, aid);

                        tmp = "USERS/" + request_arguments.at(0) + "/HOSTED/" + aid + ".txt"; // create hosted in user folder
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
                    cout << "[LOG]: " << getpid() << " Got CLOSE request" << endl;

                    response = "RCL ";
                    int n = receiveTCPsize(fd, 20, request);
                    if (n < 20 || request.back() != '\n') {
                        throw string("Syntax error on close request");
                    }
                    parseInput(request, request_arguments);

                    string uid = request_arguments.at(0);
                    string pass = request_arguments.at(1);
                    string aid = request_arguments.at(2);
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
                        string startTxt = auctionDir + "/START_" + aid + ".txt";
                        string start_content;
                        vector<string> start_content_arguments;
                        ifstream fin(startTxt);
                        if (!fin) {
                            throw string("Error opening start file to read");
                        }
                        getline(fin, start_content);
                        parseInput(start_content, start_content_arguments);
                        int start_time = stoi(start_content_arguments.at(7));

                        string content = getDateAndDuration(start_time);

                        ofstream fout(endTxt);
                        if (!fout) {
                            throw string("Error creating end text file");
                        }
                        fout << content;
                        fout.close();

                        response += "OK\n";
                    }
                    break;
                }
                case SHOW_ASSET: {
                    cout << "[LOG]: " << getpid() << " Got SHOW_ASSET request" << endl;

                    response = "RSA ";
                    int n = receiveTCPsize(fd, 4, request);
                    if (n < 4 || request.back() != '\n') {
                        throw string("Syntax error on show_asset request");
                    }
                    parseInput(request, request_arguments);

                    string aid = request_arguments.at(0);
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
                    cout << "[LOG]: " << getpid() << " Got BID request" << endl;

                    response = "RBD ";
                    int n = receiveTCPspace(fd, 3, request);
                    if (n != 20) {
                        throw string("Syntax error on bid reequest");
                    }
                    parseInput(request, request_arguments);

                    string value;
                    receiveTCPend(fd, value);
                    if (value.size() > 6) {
                        throw string("Syntax error on bid request");
                    }

                    string uid = request_arguments.at(0);
                    string pass = request_arguments.at(1);
                    string aid = request_arguments.at(2);
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
                            }
                            getline(fin, content);
                            fin.close();
                            parseInput(content, content_arguments);
                            highest_value = content_arguments.at(1);
                        }

                        int ihighest = stoi(highest_value);
                        int ivalue = stoi(value);
                        if (ivalue <= ihighest) {
                            cout << "[LOG]: On bid, new bid is not higher than the current highest bid" << endl;
                            response += "REF\n";
                            break;
                        }

                        string startTxt = auctionDir + "/START_" + aid + ".txt";
                        string start_content;
                        vector<string> start_content_arguments;
                        ifstream fin(startTxt);
                        if (!fin) {
                            throw string("Couldn't open bid file to read");
                        }
                        getline(fin, start_content);
                        fin.close();
                        parseInput(start_content, start_content_arguments);
                        //fin.read(tmp, 6);

                        string auctionOwner = start_content_arguments.at(0);
                        int start_time = stoi(start_content_arguments.at(7));

                        if (auctionOwner == uid) {
                            cout << "[LOG]: On bid, bid on own auction" << endl;
                            response += "ILG\n";
                            break;
                        }

                        string bidTxt = auctionDir + "/BIDS/" + value + ".txt";
                        ofstream fout(bidTxt);
                        if (!fout) {
                            throw string("Couldn't open file to write on bid");
                        }

                        string content = uid + " ";
                        content += value + " ";
                        content += getDateAndDuration(start_time);
                        fout << content;
                        fout.close();

                        string userBidTxt = "USERS/" + uid + "/BIDDED/" + aid + ".txt";
                        fout.open(userBidTxt);
                        if (!fout) {
                            removeFile(bidTxt);
                            throw string("Couldn't create bid file to user");
                        }
                        fout.close();
                        
                        response += "ACC\n";
                    }
                    break;
                }
                default:
                    throw string("Couldn't identify TCP request");
            }
        }
    }
    catch (string error)
    {
        cout << error << endl;
        response += "ERR\n";
    }
    catch (exception& ex) {
        cout << ex.what() << endl;
        response += "ERR\n";
    }

    cout << "[LOG]: " << getpid() << " Sending response " << response;
    sendTCPresponse(fd, response, response.size());

    close(fd);
}

void startTCP(SharedAID *sharedAID) {

    fd_tcp = socket(AF_INET, SOCK_STREAM, 0);
    if (fd_tcp == -1) {
        cout << "[LOG]: TCP Error creating socket" << endl;
        exit(EXIT_FAILURE);
    }
    cout << "[LOG]: TCP socket created" << endl;

    int on = 1;
    if (setsockopt(fd_tcp, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1) {
        cout << "[LOG]: TCP Error setting socket options" << endl; 
        exit(EXIT_FAILURE);
    }

    n_tcp = ::bind(fd_tcp, res->ai_addr, res->ai_addrlen);
    if (n_tcp == -1) {
        cout << "[LOG]: TCP Bind error" << endl;
        exit(EXIT_FAILURE);
    }
    cout << "[LOG]: TCP Bind successfully" << endl;

    if (listen(fd_tcp, SOMAXCONN) == -1) {
        cout << "[LOG]: TCP listen error" << endl;
        exit(EXIT_FAILURE);
    }

    int ret;
    while (true) {
        addrlen = sizeof(addr);
        cout << "[LOG]: Parent " << getpid() << " starting to accept requests" << endl;

        do {
            tcp_child = accept(fd_tcp, (struct sockaddr*) &addr, &addrlen);
        } while(tcp_child == -1 && errno == EINTR);

        if(tcp_child == -1) {
            cout << "[LOG]: TCP accept error" << endl;
            exit(EXIT_FAILURE);
        }
        
        struct timeval timeout;
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;
        if (setsockopt(tcp_child, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) == -1) {
            cout << "[LOG]: " << getpid() << " Error setting timeout" << endl;
            do {
                ret = close(tcp_child);
            } while (ret == -1 && errno == EINTR);
            exit(EXIT_FAILURE);
        }

        if (setsockopt(tcp_child, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) == -1) {
            cout << "[LOG]: " << getpid() << " Error setting timeout" << endl;
            do {
                ret = close(tcp_child)  ;
            } while (ret == -1 && errno == EINTR);
            exit(EXIT_FAILURE);
        }

        pid_t pid = fork();
        if (pid == -1) {
            cout << "TCP fork error" << endl;
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            close(fd_tcp);
            int on = 1;
            setsockopt(tcp_child, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

            if ((errcode = getnameinfo((struct sockaddr *) &addr, addrlen, host, sizeof(host), service, sizeof(service), 0)) != 0) {
                cout << "[LOG]: TCP Error getnameinfo: " << gai_strerror(errcode) << endl;
            } else {
                cout << "[LOG]: " << getpid() << " will be handling request from " << host << ":" << service << endl;
            }
            
            handleTCPRequest(tcp_child, sharedAID);
            if (shmdt(sharedAID) == -1) {
                cout << "[LOG]: " << getpid() << " erro detaching shared memory" << endl;
            }
            cout << "[LOG]: " << getpid() << " Terminating after handling request" << endl; 
            
            do {
                ret = close(tcp_child);
            } while (ret == -1 && errno == EINTR);
            exit(EXIT_SUCCESS);
        }
    
        do {
            ret = close(tcp_child);
        } while (ret == -1 && errno == EINTR);

        if (ret == -1) {
            exit(EXIT_FAILURE);
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

    close(fd_tcp);
    close(fd_udp);
        
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {

    signal(SIGINT, sigintHandler);
    signal(SIGCHLD, SIG_IGN);

    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = SIG_IGN;
    if (sigaction(SIGPIPE, &act, NULL) == -1) {
        exit(EXIT_FAILURE);   
    }

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
        exit(EXIT_FAILURE);
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