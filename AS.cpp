#include "AS.hpp"
#include "common.hpp"
// handle signal child
// verify if ifstream ofstream have opened correctly
// use remove_all to delete everything in a folder
// add syntax if anything goes wrong change it to false
// syntax error or error creating/removing files
// maybe change parseInput to splitString cause it is used in other cases
// If error or syntax send Err or Syn
// implement try catch?
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

void removeFile(string &path, bool no_error) {
    error_code ec;
    int ret = filesystem::remove(path, ec);

    if (!ec && !ret) {
        no_error = false;
    } 
    else {      
        no_error = false;
        cout << "File " << path << " remove unsuccessful: "<< ec.value() << " " <<ec.message() << endl;
    }
}

void removeDir(string &path, bool no_error) {
    error_code ec;
    
    int ret = filesystem::remove_all(path, ec);
    if (!ec && ret == 0) {
        no_error = false;
    } 
    else {      
        no_error = false;
        cout << "Dir " << path << " remove unsuccessful: "<< ec.value() << " " <<ec.message() << endl;
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

bool checkPassword(string &uid, string &pw, bool &no_error) {
    string fname = "USERS/" + uid + "/" + uid + "_pass.txt";

    ifstream pw_file(fname);
    if (!pw_file) {
        cout << "[LOG]: Couldn't open user password file" << endl;
        no_error = false;
        return true;
    }
    
    ostringstream oss;
    oss << pw_file.rdbuf();
    pw_file.close();
    return oss.str() == pw;
}

bool createLogin(string &uid, string &pass, bool &no_error) {
    // false if incorrect password
    string loginName;

    if (!checkPassword(uid, pass, no_error)) {
        cout << "[LOG]: Incorrect password on login" << endl;
        return false;
    }

    loginName = "USERS/" + uid + "/" + uid + "_login.txt";
    ofstream fout(loginName, ios::out);
    if (!fout) {
        cout << "[LOG]: Couldn't create login file" << endl;
        no_error = false;
        return true;
    }
    cout << "[LOG]: User " + uid << " logged in" << endl;
    fout.close();
    return true;
}

bool removeLogin(string &uid, string &pass, bool &no_error) {
    // 0 if pass is ok, -1 otherwise
    // other errors are carried by syntax and no_error;
    string loginName;

    if (!checkPassword(uid, pass, no_error)) {
        cout << "[LOG]: Incorrect password" << endl;
        return false;
    }

    loginName = "USERS/" + uid + "/" + uid + "_login.txt";
    removeFile(loginName, no_error);
    
    return true;
}

bool Register(string &uid, string &pass, bool &no_error) {
    // true if registered, false otherwise
    string userDir, userPass, hostedDir, biddedDir;
    int ret;

    userDir = "USERS/" + uid;
    ret = mkdir(userDir.c_str(), 0700);
    if (ret == -1) {
        cout << "[LOG]: Couldn't create user directory" << endl;
        return false;
    }

    hostedDir = userDir + "/HOSTED";
    ret = mkdir(hostedDir.c_str(), 0700);
    if (ret == -1) {
        cout << "[LOG]: Couldn't create hosted directory upon registration" << endl;
        removeDir(userDir, no_error);
        return false;
    }

    biddedDir = userDir + "/BIDDED";
    ret = mkdir(biddedDir.c_str(), 0700);
    if (ret == -1) {
        cout << "[LOG]: Couldn't create bidded directory upon registration" << endl;
        removeDir(userDir, no_error);
        return false;
    }

    userPass = "USERS/" + uid + "/" + uid + "_pass.txt";
    ofstream fout(userPass, ios::out);
    if (!fout) {
        cout << "[LOG]: Couldn't create user password file" << endl;
        removeDir(userDir, no_error);
        return false;
    }
    fout << pass;
    cout << "[LOG]: User " + uid + " registered with password " << pass << endl;
    fout.close();
    return true;
}

bool auctionEnded(string const &aid) {
    string endedTxt = "AUCTIONS/" + aid + "/END_" + aid + ".txt";
    return exists(endedTxt);
}

void endAuction(string const &aid, bool &no_error) {
    string endTxt = "AUCTIONS/" + aid + "/END_" + aid + ".txt";
    ofstream fout(endTxt);
    if (!fout) {
        cout << "[LOG]: Couldn't create END AUCTION text file" << endl;
        no_error = false;
    }
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

bool checkAuctionDuration(string const &aid, bool &no_error) {
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

    if (current_fulltime > start_fulltime + duration) {
        endAuction(aid, no_error);
        return false;
    }
    return true;
}

string handleUDPRequest(char request[]) {
    // return string with response
    string response;
    vector<string> request_arguments;
    int request_type;
    bool syntax = true;
    bool no_error = true;

    parseInput(request, request_arguments);
    // verificar syntax do request (espacos, \n)
    request_type = parseCommand(request_arguments[0]);
    switch(request_type) {
        case LOGIN: {
            string uid = request_arguments[1];
            string pass = request_arguments[2];
            if (!checkUID(uid) || !checkPasswordSyntax(pass)) {
                syntax = false;
                break;
            }
            string loginDir = "USERS/" + uid;
            string passTxt = loginDir + "/" + uid + "_pass.txt";
            if (exists(loginDir) && exists(passTxt)) {
                string loginTxt;
                loginTxt = loginDir + "/" + uid + "_login.txt";
                if (exists(loginTxt)) {
                    cout << "[LOG]: User already logged in" << endl;
                    response = "RLI OK\n";
                    break;
                } else if (createLogin(uid, pass, no_error)) {
                    response = "RLI OK\n";
                    break;
                }
                response = "RLI NOK\n";
            }
            else {
                if (Register(uid, pass, no_error)) {
                    cout << "[LOG]: New user " + uid + " registered" << endl;
                    response = "RLI REG\n";
                    if (syntax && no_error) {
                        createLogin(uid, pass, no_error);
                    }
                }
            }
            break;
        }
        case LOGOUT: {
            string uid = request_arguments[1];
            string pass = request_arguments[2];
            if (!checkUID(uid) || !checkPasswordSyntax(pass)) {
                syntax = false;
                break;
            }
            string loginDir = "USERS/" + uid;
            string passTxt = loginDir + "/" + uid + "_pass.txt";
            if (exists(loginDir) && exists(passTxt)) {
                string loginTxt;
                loginTxt = loginDir + "/" + uid + "_login.txt";
                if (!exists(loginTxt)) {
                    cout << "[LOG]: User not logged in" << endl;
                    response = "RLO NOK\n";
                } else if (!removeLogin(uid, pass, no_error)) {
                    response = "RLI NOK\n";
                } else {
                    response = "RLI OK\n";
                }
            } else {
                response = "RLO UNR\n";
            }
            break;
        }
        case UNREGISTER: {
            string uid = request_arguments[1];
            string pass = request_arguments[2];
            if (!checkUID(uid) || !checkPasswordSyntax(pass)) {
                syntax = false;
                break;
            }
            string loginDir = "USERS/" + uid;
            string passTxt = loginDir + "/" + uid + "_pass.txt";
            if (exists(loginDir) && exists(passTxt)) {
                string loginTxt = loginDir + "/" + uid + "_login.txt";
                if (exists(loginTxt)) {
                    removeFile(loginTxt, no_error);
                    if (no_error) {
                        removeFile(passTxt, no_error);
                    }
                } else {
                    cout << "[LOG]: User not logged in" << endl;
                    response = "RUR NOK\n";
                }
            } else {
                cout << "[LOG]: User not registered" << endl;
                response = "RUR UNR\n";
            }
            break;
        }
        case MYAUCTIONS: {
            string uid = request_arguments[1];
            if (!checkUID(uid)) {
                syntax = false;
                break;
            }
            string hostedDir = "USERS/" + uid + "/HOSTED";
            string loginTxt = "USERS/" + uid + "/" + uid + "_login.txt";
        
            if (filesystem::is_empty(hostedDir)) {
                response = "RMA NOK\n";
            } else if (!exists(loginTxt)) {
                response = "RMA NLG\n";
            } else {
                response = "RMA OK ";
                string aid;
                for (auto const &entry : filesystem::directory_iterator(hostedDir)) {
                    aid = getSubString(entry.path().string(), 20, 3);
                    if (auctionEnded(aid) || !checkAuctionDuration(aid, no_error)) {
                        // terminou ou ja esta fora de prazo
                        response += aid + " 0 ";
                    } else {
                        response += aid + " 1 ";
                    }

                    if (!no_error) {
                        break;
                    }
                }
                response += "\n";
            }
            break;
        }
        case MYBIDS: {
            string uid = request_arguments[1];
            if (!checkUID(uid)) {
                syntax = false;
                break;
            }

            string biddedDir = "USERS/" + uid + "/BIDDED";
            string loginTxt = "USERS/" + uid + "/" + uid + "_login.txt";
            if (filesystem::is_empty(biddedDir)) {
                response = "RMB NOK\n";
            } else if (!exists(loginTxt)) {
                response = "RMB NLG\n";
            } else {
                response = "RMB OK ";
                string aid;
                for (auto const &entry : filesystem::directory_iterator(biddedDir)) {
                    aid = getSubString(entry.path().string(), 20, 3);
                    if (auctionEnded(aid) || !checkAuctionDuration(aid, no_error)) {
                        // terminou ou ja esta fora de prazo
                        response += aid + " 0 ";
                    } else {
                        response += aid + " 1 ";
                    }

                    if (!no_error) {
                        break;
                    }
                }
                response += "\n";
            }
            break;
        }
        case LIST:{
            string auctionsDir = "AUCTIONS";
            if (filesystem::is_empty(auctionsDir)) {
                response = "RLS NOK\n";
            } else {
                response = "RLS OK ";
                string aid;
                for (auto const &entry : filesystem::directory_iterator(auctionsDir)) {
                    aid = getSubString(entry.path().string(), 9, 3);
                    if (auctionEnded(aid) || !checkAuctionDuration(aid, no_error)) {
                        // terminou ou ja esta fora de prazo
                        response += aid + " 0 ";
                    } else {
                        response += aid + " 1 ";
                    }

                    if (!no_error) {
                        break;
                    }
                }
                response += "\n";
            }
            break;
        }
        case SHOW_RECORD: {
            string aid = request_arguments[1];
            if (!checkAID(aid)) {
                syntax = false;
                break;
            }

            string auctionDir = "AUCTIONS/" + aid;
            if (!exists(auctionDir)) {
                response = "RRC NOK\n";
            } else {
                response = "RRC OK ";
                string startTxt = auctionDir + "/START_" + aid + ".txt";
                ifstream fin(startTxt);
                if (!fin) {
                    cout << "[LOG]: Couldn't open start auction text" << endl;
                    no_error = false;
                    break;
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

                    for (auto const &entry : filesystem::directory_iterator(bidsDir)) {
                        bidsPath.push_back(entry.path().string());
                    }

                    sort(bidsPath.rbegin(), bidsPath.rend());
                    int n = min(int(bidsPath.size()), 50);
                    vector<string> sortedBidsPath(bidsPath.begin(), bidsPath.begin() + n);

                    for (int i = sortedBidsPath.size()-1; i >= 0; i--) {
                        ifstream fin(sortedBidsPath.at(i));
                        if (!fin) {
                            cout << "[LOG]: Couldn't open bid file on show_record" << endl;
                            no_error = false;
                            break;
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
                        cout << "[LOG]: Couldn't open bid file on show_record" << endl;
                        no_error = false;
                        break;
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
            cout << "[LOG]: UDP syntax error" << endl;
            syntax = false;
            break;
    }
    
    if (!syntax) {
        response = "Syntax Error\n";
    }
    else if (!no_error) {
        response = "Error opening/creating/removing file/directory\n";
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
        cout << "[LOG]: UDP sending response: " << response << endl;
        n_udp = sendto(fd_udp, response.c_str(), response.length(), 0, (struct sockaddr*) &addr, addrlen);
        if (n_udp == -1) {
            exit(1);
        }
    }
}

int receiveTCPsize(int fd, int size, string &response) {
    int total_received = 0;
    int n;
    char tmp[128];
    response.clear();
    cout << "[LOG]: " << getpid() << " Receiving TCP request" << endl;
    while (total_received < size) {
        n = read(fd, tmp, 1);
        if (n == -1) {
            cout << "TCP receive error" << endl;
            break;
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
            cout << "TCP receive error" << endl;
            break;
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

int receiveTCPimage(int fd, int size, string &fname, string &aid) {
    int total_received = 0;
    int n, to_read;
    char tmp[128];
    string dir = "AUCTIONS/" + aid + "/ASSET/" + fname;
    ofstream fout(dir, ios::binary);

    while (total_received < size) {
        to_read = min(128, size-total_received);
        n = read(fd, tmp, to_read);
        if (n == -1) {
            cout << "TCP image receive error" << endl;
            fout.close();
            remove(fname.c_str());
            return -1;
        }
        fout.write(tmp, n);
        total_received += n;
    }
    cout << "[LOG]: " << getpid() << " Received file of size " << total_received << " fsize is " << size << endl;
    fout.close();
    
    // filesystem::resize_file(dir, size);

    return total_received;
    // receive image directly into file
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
    cout << "[LOG]: Sent TCP response" << endl;
    return total_sent;
}

bool checkLogin (string &uid) {
    // checks if user is logged in
    struct stat buffer;
    string tmp = "USERS/" + uid + "/" + uid + "_login.txt";
    return (stat (tmp.c_str(), &buffer) == 0); 
}

int createAuctionDir(string &aid) {
    string AID_dirname = "AUCTIONS/" + aid;
    string BIDS_dirname = "AUCTIONS/" + aid + "/BIDS";
    string ASSET_dirname = "AUCTIONS/" + aid + "/ASSET";
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

    ret = mkdir(ASSET_dirname.c_str(), 0700);
    if (ret == -1) {
        filesystem::remove_all(AID_dirname);
        return -1;
    }

    return 1;
}

void deleteAuctionDir(string &aid, bool &no_error) {
    string AID_dirname = "AUCTIONS/" + aid;
    removeDir(AID_dirname, no_error);

    // string BIDS_dirname = "AUCTIONS/" + aid + "/BIDS";

    // rmdir(AID_dirname.c_str());
    // rmdir(BIDS_dirname.c_str());
}

string getNextAID(SharedAID *sharedAID) {
    char tmp[4];

    sem_wait(&sharedAID->sem);
    //lock_guard<mutex> lock(sharedAID->mutex);
    snprintf(tmp, 4, "%03d", sharedAID->AID);
    sharedAID->AID++;
    sem_post(&sharedAID->sem);

    string aid(tmp);
    return aid;
}



int createStartAuctionText(vector<string> &arguments, string &aid) {
    string name, tmp;
    char time_str[30]; // date and time

    tmp = arguments[0] + " "; // UID
    tmp += arguments[2] + " "; // Name
    tmp += arguments[5] + " "; // fname
    tmp += arguments[3] + " "; // start_value
    tmp += arguments[4] + " "; // time active
    tmp += getDateAndTime();

    name = "AUCTIONS/" + aid + "/START_" + aid + ".txt";
    ofstream fout(name, ios::out);
    if (!fout) {
        cout << "Error creating start auction text" << endl;
        return -1;
    }
    fout.write(tmp.c_str(), tmp.size());
    fout.close();

    return 0;
}

bool checkOwner(string &uid, string &aid, bool &no_error) {
    string auctionDir = "AUCTIONS/" + aid;
    string startTxt = auctionDir + "/START_" + aid + ".txt";
    char tmp[6];

    ifstream fin(startTxt);
    if (!fin) {
        cout << "[LOG]: Error opening file to read" << endl;
        no_error = false;
        return true;
    }
    fin.read(tmp, 6);
    fin.close()
    string targetUID(tmp);

    return uid == targetUID;
}

void handleTCPRequest(int &fd, SharedAID *sharedAID) {
    cout << "[LOG]: Got one request child " << getpid() << " handling it" << endl;
    string request, tmp, response;
    bool syntax, no_error;
    vector<string> request_arguments;
    // char buffer[BUFFERSIZE];
    int request_type;
    // int n;

    receiveTCPsize(fd, 3, tmp);
    request_type = parseCommand(tmp);
    receiveTCPsize(fd, 1, tmp); // clear space
    if (tmp.at(0) != ' ') {
        syntax = false;
    } else {
        switch (request_type) {
            case OPEN: {
                receiveTCPspace(fd, 7, request);
                parseInput(request, request_arguments);
                cout << "[LOG]: TCP open request is " + request << endl;

                ssize_t fsize;
                stringstream stream(request_arguments[6]);
                stream >> fsize;

                bool ok = true;
                if (!checkLogin(request_arguments[0])) {
                    cout << "[LOG]: User not logged in" << endl;
                    response = "ROA NLG\n";
                    ok = false;
                } else if (!checkName(request_arguments[2])){
                    cout << "[LOG]: Auction name syntax error" << endl;
                    response = "ROA NOK\n";
                    ok = false;
                } else if (!checkStartValue(request_arguments[3])) {
                    cout << "[LOG]: Auction start value syntax error" << endl;
                    response = "ROA NOK\n";
                    ok = false;
                } else if (!checkDuration(request_arguments[4])) {
                    cout << "[LOG]: Auction duration syntax error" << endl;
                    response = "ROA NOK\n";
                    ok = false;
                } else if (!checkPassword(request_arguments[0], request_arguments[1], no_error)) {
                    cout << "[LOG]: Incorrect password" << endl;
                    response = "ROA NOK\n";
                    ok = false;
                }

                if (no_error && ok) {
                    // talvez pode-se remover a pasta com tds os ficheiros dentro dela
                    int status;
                    string aid = getNextAID(sharedAID);
                    status = createAuctionDir(aid);
                    if (status == -1) {
                        response = "ROA NOK\n";
                        break;
                    }
                    status = receiveTCPimage(fd, fsize, request_arguments[5], aid);
                    receiveTCPsize(fd, 1, tmp); // get the last char which should be \n
                    if (tmp.at(0) != '\n' || status == -1) {
                        deleteAuctionDir(aid, no_error); // adicionar loop ate confirmar sucesso?
                        response = "ROA NOK\n";
                        break;
                    }
                    status = createStartAuctionText(request_arguments, aid);
                    if (status == -1) {
                        deleteAuctionDir(aid, no_error);
                        response = "ROA NOK\n";
                        break;
                    }

                    tmp = "USERS/" + request_arguments[0] + "/HOSTED/" + aid + ".txt"; // create hosted in user folder
                    ofstream fout(tmp);
                    if (!fout) {
                        deleteAuctionDir(aid, no_error);
                        response = "ROA NOK\n";
                        break;
                    }
                    fout.close();
                    
                    response = "ROA OK " + aid + "\n";
                }
                break;
            }
            case CLOSE: {
                int n = receiveTCPsize(fd, 20, request);
                if (n < 20 || request.back() != '\n') {
                    syntax = false;
                    break;
                }
                parseInput(request, request_arguments);

                string uid = request_arguments[0];
                string pass = request_arguments[1];
                string aid = request_arguments[2];

                string auctionDir = "AUCTIONS/" + aid;
                string endTxt = auctionDir + "/END_" + aid + ".txt";
                if (!checkLogin(uid)) {
                    cout << "[LOG]: On close User not logged in" << endl;
                    response = "RCL NLG\n";
                    break;
                } else if (!exists(auctionDir)) {
                    cout << "[LOG]: On close auction does not exist" << endl;
                    response = "RCL EAU\n";
                    break;
                } else if (!exists(endTxt)) {
                    cout << "[LOG]: On close auction already closed" << endl;
                    response = "RCL END\n";
                    break;
                } else if (!checkOwner(uid, aid, no_error)) {
                    cout << "[LOG]: On close auction not owned by given uid" << endl;
                    response = "RCL EOW\n";
                    break;
                } else if (no_error && !checkPassword(uid, pass, no_error)) {
                    cout << "[LOG]: On close uid password don't match" << endl;
                    response  = "RCL NOK\n";
                    break;
                }

                if (no_error) {
                    string content = getDateAndTime();

                    ofstream fout(endTxt);
                    if (!fout) {
                        cout << "[LOG]: Error opening file to write" << endl;
                        no_error = false;
                        break;
                    }
                    fout << content;
                    fout.close();

                    response = "RCL OK\n";
                }
                break;
            }
            case SHOW_ASSET:
                break;
            case BID:
                break;
            default:
                cout << "Syntax error" << endl;
                syntax = false;
                break;
        }
    }
    

    if (!syntax || !no_error) {
        response = "ERR\n";
    }
    // else if (!no_error) {
    //     response = "Error opening/creating/removing file/directory\n";
    // }

    sendTCPresponse(fd, tmp, tmp.size());

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
            int on = 1;
            setsockopt(tcp_child, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
            handleTCPRequest(tcp_child, sharedAID);
            if (shmdt(sharedAID) == -1) {
                cout << "[LOG]: " << getpid() << " erro detaching shared memory" << endl;
            }
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
    
}