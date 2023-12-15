#include "AS.hpp"
#include "common.hpp"

// remove sigint handler
// maybe change parseInput to splitString cause it is used in other cases
// bids always have 6 chars? (not necessary?)
// send file by parts

using namespace std;

int fd_tcp, fd_udp;
socklen_t addrlen;
struct addrinfo hints, *res;
struct sockaddr_in addr;
string port;
bool verbose = false;
time_t fulltime;
struct tm *current_time;
string current_time_str;
SharedAID *sharedAID;
char host[NI_MAXHOST], service[NI_MAXSERV];

// Checks if a file/directory exists
bool exists(string& name) {
    struct stat buffer;   
    return (stat(name.c_str(), &buffer) == 0); 
}

// Removes a single file, throwing an error if unsuccessful
void removeFile(string &path) {
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

// Removes a directory and all the files inside, throws an error if unsuccessful
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

// Returns the request_code given string request
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

// Check if given password matches the stored one from a user
bool checkPassword(string &uid, string &pw) {
    // path of file with user password
    string fname = "USERS/" + uid + "/" + uid + "_pass.txt";

    ifstream pw_file(fname);
    if (!pw_file) {
        throw string("[LOG]: Couldn't open user password file");
    }
    
    ostringstream oss;
    oss << pw_file.rdbuf(); // reads the stored password
    pw_file.close();
    return oss.str() == pw; // tests it agaisnt the given one
}

// Creates a login file if the password is correct, else returns false
bool createLogin(string &uid, string &pass) {
    string loginName;

    // checks the password
    if (!checkPassword(uid, pass)) {
        cout << "[LOG]: Incorrect password on login" << endl;
        return false;
    }

    // Login file to be created
    loginName = "USERS/" + uid + "/" + uid + "_login.txt";
    ofstream fout(loginName, ios::out);
    if (!fout) {
        throw string("[LOG]: Couldn't create login file");
    }

    cout << "[LOG]: User " + uid << " logged in" << endl;
    fout.close();
    return true;
}

// Removes the login file of a user, verifying the given password
bool removeLogin(string &uid, string &pass) {
    string loginName;

    // checks the password
    if (!checkPassword(uid, pass)) {
        cout << "[LOG]: Incorrect password" << endl;
        // false if wrong password
        return false;
    }

    // file path of the login file
    loginName = "USERS/" + uid + "/" + uid + "_login.txt";
    removeFile(loginName);
    
    cout << "[LOG]: UDP User " + uid + " logged out successfully" << endl;
    return true;
}

// Creates the Dirs and Login/Pass files for a user
void Register(string &uid, string &pass) {
    string userDir, userPass, hostedDir, biddedDir;
    int ret;

    userDir = "USERS/" + uid;
    if (!exists(userDir)) {
        // if the user has not been registered before
        // create all the necessary directories

        ret = mkdir(userDir.c_str(), 0700);
        if (ret == -1) {
            throw string("[LOG]: Couldn't create user directory");
        }

        // Hosted Dir
        hostedDir = userDir + "/HOSTED";
        ret = mkdir(hostedDir.c_str(), 0700);
        if (ret == -1) {
            removeDir(userDir);
            throw string("[LOG]: Couldn't create hosted directory upon registration");
        }

        // Bidded Dir
        biddedDir = userDir + "/BIDDED";
        ret = mkdir(biddedDir.c_str(), 0700);
        if (ret == -1) {
            removeDir(userDir);
            throw string("[LOG]: Couldn't create bidded directory upon registration");
        }
    }

    // path of the password file 
    userPass = "USERS/" + uid + "/" + uid + "_pass.txt";
    ofstream fout(userPass, ios::out); // creates the file
    if (!fout) { 
        removeDir(userDir);
        throw string("[LOG]: Couldn't create bidded directory upon registration");
    }
    fout << pass; // writes the password into the file
    cout << "[LOG]: User " + uid + " registered with password " << pass << endl;
    fout.close();
}

// Checks if an auction is closed
bool auctionEnded(string const &aid) {
    string endedTxt = "AUCTIONS/" + aid + "/END_" + aid + ".txt";
    return exists(endedTxt);
}

// Creates the end file for an auction
void endAuction(string const &aid, int const &itime) {
    // path of the End file
    string endTxt = "AUCTIONS/" + aid + "/END_" + aid + ".txt";
    ofstream fout(endTxt); // creates the file
    if (!fout) {
        throw string("[LOG]: Couldn't create END AUCTION text file");
    }

    char time_str[30];
    time_t ttime = itime;
    struct tm *end_time = gmtime(&ttime);
    snprintf(time_str, 30, "%4d\u002d%02d\u002d%02d %02d:%02d:%02d",
            end_time->tm_year + 1900, end_time->tm_mon + 1,end_time->tm_mday, 
            end_time->tm_hour , end_time->tm_min , end_time->tm_sec);
    
    string content(time_str); // Date and Hour
    content += " " + to_string(itime); // Duration
    fout << content; // Writes the content to the file
    fout.close();
}

// Returns the time in seconds since the beginning of 1970
string getTime() {
    time(&fulltime);
    stringstream ss;
    ss << fulltime;
    
    return ss.str();
}

// Returns the current date, time and seconds since 1970
string getDateAndTime() {
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

// Returns the current date, time and seconds since a given start
string getDateAndDuration(int &start) {

    char time_str[30];

    time(&fulltime);
    current_time = gmtime(&fulltime);
    snprintf(time_str, 30, "%4d\u002d%02d\u002d%02d %02d:%02d:%02d",
            current_time->tm_year + 1900, current_time->tm_mon + 1,current_time->tm_mday, 
            current_time->tm_hour , current_time->tm_min , current_time->tm_sec);

    int duration = fulltime-start; // seconds since start

    return string(time_str) + " " +  to_string(duration);
}

// Checks if auction duration is ok (true), if not ends it (false)
bool checkAuctionDuration(string const &aid) {
    // path of start file of the auction
    string startTxt = "AUCTIONS/" + aid + "/START_" + aid + ".txt";
    ifstream fin(startTxt);
    if (!fin) {
        throw string("Couldn't open start txt to read");
    }
    string content;
    vector<string> content_arguments;
    getline(fin, content); // reads file's content
    fin.close();

    parseInput(content, content_arguments); // Splits the content by spaces

    string duration = content_arguments.at(4);
    string start_fulltime = content_arguments.back();
    string current_fulltime = getTime();

    int iduration = stoi(duration); // Max duration of the auction
    int istart = stoi(start_fulltime); // Start time
    int icurrent = stoi(current_fulltime); // Current time

    if (icurrent > istart + iduration) {
        // Past duration and auction has not been closed yet
        endAuction(aid, iduration); // ends the auction
        return false;
    }
    return true;
}

// Handles UDP request from User, returns the response to send
string handleUDPRequest(char request[]) {

    string response; // message to send to user
    vector<string> request_arguments;
    int request_type;

    parseInput(request, request_arguments); // split request by spaces
    request_type = parseCommand(request_arguments.at(0));

    try {
        checkUDPSyntax(string(request)); // check the overall syntax of the request (spaces and \n)
        switch(request_type) {
            case LOGIN: {
                if (verbose) cout << "[LOG]: UDP Got LOGIN request from " << host << ":" << service << endl;
                response = "RLI ";

                string uid = request_arguments.at(1);
                string pass = request_arguments.at(2);

                // Check arguments syntax
                checkUID(uid);
                checkPasswordSyntax(pass);

                string loginDir = "USERS/" + uid; 
                string passTxt = loginDir + "/" + uid + "_pass.txt";
                if (exists(loginDir) && exists(passTxt)) {
                    // User already registered
                    string loginTxt;
                    loginTxt = loginDir + "/" + uid + "_login.txt";
                    if (exists(loginTxt)) { // check if logged in
                        cout << "[LOG]: User " << uid << " already logged in" << endl;
                        response += "OK\n";
                    } else if (createLogin(uid, pass)) {
                        // User not logged in, tries to log in
                        response += "OK\n";
                    } else {
                        // Wrong password
                        response += "NOK\n";
                    }
                }
                else {
                    // "New" user
                    Register(uid, pass);
                    createLogin(uid, pass);
                    response += "REG\n";
                }
                break;
            }
            case LOGOUT: {
                if (verbose) cout << "[LOG]: UDP Got LOGOUT request from " << host << ":" << service << endl;

                response = "RLO ";

                string uid = request_arguments.at(1);
                string pass = request_arguments.at(2);
                
                // Check arguments syntax
                checkUID(uid);
                checkPasswordSyntax(pass);

                string loginDir = "USERS/" + uid;
                string passTxt = loginDir + "/" + uid + "_pass.txt";
                if (exists(loginDir) && exists(passTxt)) { // checks if user is registered
                    // Registered User

                    string loginTxt;
                    loginTxt = loginDir + "/" + uid + "_login.txt";
                    if (!exists(loginTxt)) { // check if user is logged in
                        cout << "[LOG]: User not logged in" << endl;
                        response += "NOK\n";
                    } else if (!removeLogin(uid, pass)) { // tries to log out
                        // Wrong password
                        response += "NOK\n";
                    } else {
                        // Logged out
                        response += "OK\n";
                    }
                } else {
                    // User not registered
                    cout << "[LOG]: User not yet registered" << endl;;
                    response += "UNR\n";
                }
                break;
            }
            case UNREGISTER: {
                if (verbose) cout << "[LOG]: UDP Got UNREGISTER request from " << host << ":" << service << endl;

                response = "RUR ";
                string uid = request_arguments.at(1);
                string pass = request_arguments.at(2);

                // check arguments syntax
                checkUID(uid);
                checkPasswordSyntax(pass);

                string loginDir = "USERS/" + uid;
                string passTxt = loginDir + "/" + uid + "_pass.txt";
                if (exists(loginDir) && exists(passTxt)) { // checks if the user is registered
                    // Registered User
                    string loginTxt = loginDir + "/" + uid + "_login.txt";
                    if (exists(loginTxt)) { // checks if user is logged in
                        // User logged in -> Unregisters the user
                        removeFile(loginTxt);
                        removeFile(passTxt);
                        response += "OK\n";
                    } else {
                        // User not logged in
                        cout << "[LOG]: User not logged in" << endl;
                        response += "NOK\n";
                    }
                } else {
                    // User is not registered yet
                    cout << "[LOG]: User not registered" << endl;
                    response += "UNR\n";
                }
                break;
            }
            case MYAUCTIONS: {
                if (verbose) cout << "[LOG]: UDP Got MYAUCTIONS request from " << host << ":" << service << endl;

                response = "RMA ";
                string uid = request_arguments.at(1);

                //check UID syntax
                checkUID(uid);

                string hostedDir = "USERS/" + uid + "/HOSTED";
                string loginTxt = "USERS/" + uid + "/" + uid + "_login.txt";

                if (filesystem::is_empty(hostedDir)) { // checks if user has auctions
                    response += "NOK\n";
                } else if (!exists(loginTxt)) { // checks if logged in
                    response += "NLG\n";
                } else {
                    // User is logged in and has auctions

                    string tmp_response; // Add response to a tmp string, in case of error
                    tmp_response += "OK"; 

                    vector<string> auctionsPath;
                    for (auto const &entry : filesystem::directory_iterator(hostedDir)) {
                        // Add all auctions to a vector
                        auctionsPath.push_back(entry.path().string());
                    }
                    // Sorts the vector
                    sort(auctionsPath.begin(), auctionsPath.end());

                    string aid;
                    for (string &path: auctionsPath) {
                        aid = getSubString(path, 20, 3); // gets AID from path
                        if (auctionEnded(aid) || !checkAuctionDuration(aid)) {
                            // Auction closed or should've been closed (now it's closed)
                            tmp_response += " " + aid + " 0";
                        } else {
                            // Active auction
                            tmp_response += " " + aid + " 1";
                        }
                    }
                    response += tmp_response; // if no errors, complete the response
                    response += "\n";
                }
                break;
            }
            case MYBIDS: {
                if (verbose) cout << "[LOG]: UDP Got MYBIDS request from " << host << ":" << service << endl;

                response = "RMB ";
                string uid = request_arguments.at(1);

                // check argument syntax
                checkUID(uid);

                string biddedDir = "USERS/" + uid + "/BIDDED";
                string loginTxt = "USERS/" + uid + "/" + uid + "_login.txt";
                if (filesystem::is_empty(biddedDir)) { // check if user has bids
                    response += "NOK\n";
                } else if (!exists(loginTxt)) { // checks if user is logged in
                    response += "NLG\n";
                } else {
                    // User has bids and is logged in
                    string tmp_response = "OK";

                    vector<string> bidsPath;
                    for (auto const &entry : filesystem::directory_iterator(biddedDir)) {
                        // Add all bids to a vector
                        bidsPath.push_back(entry.path().string());
                    }
                    // Sorts the vector of bids
                    sort(bidsPath.begin(), bidsPath.end());

                    string aid;
                    for (string &entry : bidsPath) {
                        aid = getSubString(entry, 20, 3); // gets the AID from path
                        if (auctionEnded(aid) || !checkAuctionDuration(aid)) { 
                            // Auction closed or should've been closed (now, it's closed)
                            tmp_response += " " + aid + " 0";
                        } else {
                            // Active auction
                            tmp_response += " " + aid + " 1";
                        }
                    }
                    response += tmp_response; // Complete response, if no errors
                    response += "\n";
                }
                break;
            }
            case LIST:{
                if (verbose) cout << "[LOG]: UDP Got LIST request from " << host << ":" << service << endl;

                response = "RLS ";
                string auctionsDir = "AUCTIONS";
                if (filesystem::is_empty(auctionsDir)) { // Checks if there are auctions
                    response += "NOK\n";
                } else {
                    // There are auctions
                    string tmp_response = "OK";

                    vector<string> auctionsPath;
                    for (auto const &entry : filesystem::directory_iterator(auctionsDir)) {
                        // Add the auctions to a vector
                        auctionsPath.push_back(entry.path().string());
                    }
                    // Sort them
                    sort(auctionsPath.begin(), auctionsPath.end());

                    string aid;
                    for (string &entry : auctionsPath) {
                        aid = getSubString(entry, 9, 3); // Get Aid from path
                        if (auctionEnded(aid) || !checkAuctionDuration(aid)) {
                            // Auction closed or should've been closed (now, it's closed)
                            tmp_response += " " + aid + " 0";
                        } else {
                            // Active auction
                            tmp_response += " " + aid + " 1";
                        }
                    }
                    response += tmp_response; // Complete response, if no errors
                    response += "\n";
                }
                break;
            }
            case SHOW_RECORD: {
                if (verbose) cout << "[LOG]: UDP Got SHOW_RECORD request from " << host << ":" << service << endl;
                response = "RRC ";
                string aid = request_arguments.at(1);

                // check argument syntax
                checkAID(aid);
                
                string auctionDir = "AUCTIONS/" + aid;
                if (!exists(auctionDir)) { // checks if auction exists
                    response += "NOK\n";
                } else {
                    checkAuctionDuration(aid); // checks if auction status is correct
                    string tmp_response;
                    tmp_response += "OK ";

                    string startTxt = auctionDir + "/START_" + aid + ".txt";
                    ifstream fin(startTxt); // Open START_AID.txt to read auction's info
                    if (!fin) {
                        throw string("[LOG]: Couldn't open start auction text");
                    }

                    string content;
                    vector<string> content_arguments;
                    getline(fin, content); // read all file's content into content
                    parseInput(content, content_arguments); // separate by spaces
                    fin.close();

                    string uid = content_arguments.at(0);
                    string auction_name = content_arguments.at(1);
                    string fname = content_arguments.at(2);
                    string start_value = content_arguments.at(3);
                    string duration = content_arguments.at(4);
                    string date = content_arguments.at(5);
                    string time = content_arguments.at(6);

                    tmp_response += uid + " ";
                    tmp_response += auction_name + " ";
                    tmp_response += fname + " ";
                    tmp_response += start_value + " ";
                    tmp_response += date + " ";
                    tmp_response += time + " ";
                    tmp_response += duration;

                    string bidsDir = auctionDir + "/BIDS";
                    if (!filesystem::is_empty(bidsDir)) { // checks if auction has bids
                        // there are bids to list
                        vector<string> bidsPath;

                        for (auto const &entry : filesystem::directory_iterator(bidsDir)) {
                            // Add all bids to a vector
                            bidsPath.push_back(entry.path().string());
                        }

                        sort(bidsPath.rbegin(), bidsPath.rend()); // sort them in descending order
                        int n = min(int(bidsPath.size()), 50);
                        vector<string> sortedBidsPath(bidsPath.begin(), bidsPath.begin() + n);
                        // Get the 50 biggest bids, which correspond to the last 50 bids
                        // (AS doesn't accept bids lower than the current highest)

                        for (int i = sortedBidsPath.size()-1; i >= 0; i--) {
                            ifstream fin(sortedBidsPath.at(i)); // Open each bid file to read its info
                            if (!fin) {
                                throw string("[LOG]: Couldn't open bid file on show_record");
                            }
                            getline(fin, content); // read all info into content
                            parseInput(content, content_arguments); // separate by spaces
                            fin.close();

                            string uid = content_arguments.at(0);
                            string value = content_arguments.at(1);
                            string date = content_arguments.at(2);
                            string time = content_arguments.at(3);
                            string seconds = content_arguments.at(4);
                            tmp_response += " B ";
                            tmp_response += uid + " ";
                            tmp_response += value+ " ";
                            tmp_response += date + " ";
                            tmp_response += time + " ";
                            tmp_response += seconds;
                        }
                    }

                    string endTxt = auctionDir + "/END_" + aid + ".txt";
                    if (exists(endTxt)) { // checks if auction has ended
                        // Auction has been closed

                        ifstream fin(endTxt); // open END_AID.txt to read info
                        if (!fin) {
                            throw string("[LOG]: Couldn't open bid file on show_record");
                        }
                        getline(fin, content);
                        parseInput(content, content_arguments);
                        fin.close();

                        string date = content_arguments.at(0);
                        string time = content_arguments.at(1);
                        string duration = content_arguments.at(2);

                        tmp_response += " E ";
                        tmp_response += date + " ";
                        tmp_response += time + " ";
                        tmp_response += duration;
                    }
                    response += tmp_response;
                    response += "\n";
                }
                break;
            }
            default:
                cout << "[LOG]: UDP Request Syntax error from " << host << ":" << service << endl;
                response = "ERR\n";
        }
    }
    catch(string error)
    {
        // Some kind of error occured, probably IO
        cout << error << endl;
        response += "ERR\n";
    }
    catch (exception& ex) {
        cout << ex.what() << endl;
        response += "ERR\n";
    }

    return response; // response to send to user
}


// Starts everything necessary to handle UDP request from users (sequencially)
void startUDP() {

    fd_udp = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd_udp == -1) {
        cout << "[LOG]: UDP Error creating socket" << endl;
        exit(EXIT_FAILURE);
    }
    if (verbose) cout << "[LOG]: UDP socket created" << endl;

    int on = 1;
    // Sets socket option so that it can be reused right away
    if (setsockopt(fd_udp, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1) {
        cout << "[LOG]: TCP Error setting socket options" << endl; 
        exit(EXIT_FAILURE);
    }

    int n_udp = ::bind(fd_udp, res->ai_addr, res->ai_addrlen);
    if (n_udp == -1) {
        cout << "[LOG]: UDP Bind error: " << strerror(errno);
        exit(EXIT_FAILURE);
    }
    cout << "[LOG]: UDP Bind successfully" << endl;

    struct timeval timeout;
    timeout.tv_sec = 5; 
    timeout.tv_usec = 0;
    // Sets sendto timeout
    if (setsockopt(fd_udp, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) == -1) {
        cout << "[LOG]: UDP Error setting timeout" << endl;
        int ret;
        do {
            ret = close(fd_udp);
        } while (ret == -1 && errno == EINTR);
        exit(EXIT_FAILURE);
    }

    if (verbose) cout << "[LOG]: UDP starting to read from socket" << endl;
    string response;
    char buffer[BUFFERSIZE];
    while (true) {
        addrlen = sizeof(addr);
        response.clear();
        memset(buffer, 0, sizeof buffer); // clear buffer

        if (verbose) cout << "[LOG]: UDP waiting for requests" << endl;

        // Receives a request
        n_udp = recvfrom(fd_udp, buffer, BUFFERSIZE, 0, (struct sockaddr*) &addr, &addrlen);
        if (n_udp == -1) {
            // Something went wrong while receiving
            cout << "Error reading request from UDP socket" << endl;
            response = "ERR\n";
        } else {
            // if ok, get user's ip and port
            int errcode;
            if ((errcode = getnameinfo((struct sockaddr *) &addr, addrlen, host, sizeof(host), service, sizeof(service), NI_NUMERICHOST)) != 0) {
                cout << "[LOG]: UDP Error getnameinfo: " << gai_strerror(errcode) << endl;
            }
            response = handleUDPRequest(buffer); 
        }

        if (verbose) cout << "[LOG]: UDP sending response: " << response;
        // Sends the response to the user
        n_udp = sendto(fd_udp, response.c_str(), response.length(), 0, (struct sockaddr*) &addr, addrlen);
        if (n_udp == -1) {
            // Something went wrong sending the response
            cout << "Error sending response to UDP socket" << endl;
        }
    }
}


// Reads from a TCP sockets 'size' bytes
int receiveTCPsize(int fd, int size, string &request) {
    int total_received = 0; // bytes received 
    int n; // bytes read from socket at a time
    char tmp[128];
    request.clear();

    while (total_received < size) { // while bytes read is not enough keep reading
        n = read(fd, tmp, 1); // read one byte at a time
        if (n == -1) {
            throw string("Error reading from TCP socket");
        }
        concatenateString(request, tmp, n); // add the byte to the request
        total_received += n; // update bytes received
    }

    return total_received;
}

// Keep reading from TCP socket untill it reads 'size' spaces
int receiveTCPspace(int fd, int size, string &request) {
    int total_received = 0; // bytes read
    int total_spaces = 0; // spaces read
    int n; // bytes read at a time
    char tmp[128];
    request.clear();

    while (total_spaces < size) { // while spaces read is not enough, keep reading
        n = read(fd, tmp, 1); // read byte
        if (n == -1) {
            throw string("Error reading from TCP socket");
        }
        concatenateString(request, tmp, n); // add byte to request
        total_received += n; // update bytes read
        if (tmp[0] == ' ') {
            // if bytes is a space, increment spaces read
            total_spaces++;
        }
    }

    return total_received;
}

// Reads from a TCP socket untill it reads a '\n'
int receiveTCPend(int fd, string &response) {
    int total_received = 0; // bytes read (not counting the '\n')
    int n; // bytes read at a time
    char tmp[128];
    response.clear();
    
    while (true) {
        n = read(fd, tmp, 1); // reads a bytes
        if (n == -1) {
            throw string("Error reading from TCP socket");
        } else if (tmp[0] == '\n') {
            // bytes is a '\n' then quit
            break;
        }
        concatenateString(response, tmp, n);
        total_received += n;
        
    }

    return total_received;
}

// Reads a File from TCP socket, 128 byte at a time, while storing it to the dest. file
int receiveTCPfile(int fd, int size, string &fname, string &aid) {
    int total_received = 0; // bytes received
    int n, to_read; // bytes read at a time and bytes left to read
    char tmp[128];
    string auctionDir = "AUCTIONS/" + aid;
    string dir = auctionDir + "/ASSET/" + fname; // path to store the file

    ofstream fout(dir, ios::binary); // create a new file to write
    if (!fout) {
        // if something went wrong, then revert everything (delete auction)
        removeDir(auctionDir);
        throw string("Error creating asset file");
    }

    while (total_received < size) { // while there are bytes left to receive, keep reading
        to_read = min(128, size-total_received); // if file has less than 128 bytes left, then we should only read that many
        n = read(fd, tmp, to_read);
        if (n == -1) {
            fout.close();
            removeDir(auctionDir);
            throw string("Error receiving TCP file");
        }
        fout.write(tmp, n); // store the received data to the file
        total_received += n;
    }

    fout.close();
    return total_received;
}

// Sends a response to user through the TCP socket
int sendTCPresponse(int fd, string &message, int size) {
    int total_sent = 0; // bytes sent
    int n; // bytes sent at a time
    while (total_sent < size) { // while message has not been sent totally
        n = write(fd, message.c_str() + total_sent, size - total_sent);
        if (n == -1) {
            cout << "TCP send error" << endl;
            return n;
        }
        total_sent += n;
    }

    return total_sent;
}

// Checks if User is logged in (true), else false
bool checkLogin (string &uid) {

    struct stat buffer;
    string tmp = "USERS/" + uid + "/" + uid + "_login.txt";
    return (stat (tmp.c_str(), &buffer) == 0); 
}

// Creates all the necessary directories to open a new auction
void createAuctionDir(string &aid) {
    // Dirs to be created
    string AID_dirname = "AUCTIONS/" + aid;
    string BIDS_dirname = "AUCTIONS/" + aid + "/BIDS";
    string ASSET_dirname = "AUCTIONS/" + aid + "/ASSET";
    int ret;

    // Creation of Dirs, if something goes wrong reverts back
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

// Deletes the directory of an auction
void deleteAuctionDir(string &aid) {
    string AID_dirname = "AUCTIONS/" + aid; // target
    removeDir(AID_dirname);
}

// If MAX_AUCTIONS has not been reached, returns the next available AID, else return empty string
string getNextAID() {
    // returns the next available AID

    string aid;
    sem_wait(&sharedAID->sem); // prevents other processes from getting an aid

    if (sharedAID->AID > MAX_AUCTIONS) {
        // maximum number of auctions reached
        aid = string();
    } else {
        // there is space for a new AID

        stringstream ss;
        ss << setw(3) << setfill('0') << sharedAID->AID; // get the new AID, fill with 0s if necessary
        aid = ss.str();
        string auctionDir = "AUCTIONS/" + aid;
        sharedAID->AID++; // next "available" AID
        while (exists(auctionDir)) { // loops until it find an available aid
            // There's an auction with the given AID
            ss.str(string()); // clear ss
            ss << setw(3) << setfill('0') << sharedAID->AID; // get a new AID
            aid = ss.str();
            auctionDir = "AUCTIONS/" + aid;
            sharedAID->AID++;
        }
    }
    
    sem_post(&sharedAID->sem); // allows others to get an AID
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

void handleTCPRequest(int &fd) {
    // handles TCP request from user (receives, handles and answers)

    string request, tmp, response, to_print;
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
                    if (verbose) cout << "[LOG]: " << getpid() << " Got OPEN request from " << host << ":" << service << endl;

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
                        string aid = getNextAID();

                        if (aid.size() == 0) {
                            response += "NOK\n"; // Max num auction reached
                            receiveTCPsize(fd, fsize+1, tmp);
                            break;
                        }

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
                        
                        to_print = "[LOG]: " + to_string(getpid()) + " Created auction with aid " + aid + '\n';
                        cout << to_print;
                        response += "OK " + aid + "\n";
                    }
                    break;
                }
                case CLOSE: {
                    if (verbose) cout << "[LOG]: " << getpid() << " Got CLOSE request from " << host << ":" << service << endl;

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

                        to_print = "[LOG]: " + to_string(getpid()) + " Closed auction " + aid + '\n';
                        cout << to_print;
                        response += "OK\n";
                    }
                    break;
                }
                case SHOW_ASSET: {
                    if (verbose) cout << "[LOG]: " << getpid() << " Got SHOW_ASSET request from " << host << ":" << service << endl;

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
                    response += openFile(assetPath) + "\n";
                    to_print = "[LOG]: " + to_string(getpid()) + " prepared asset to send\n";
                    cout << to_print;
                    break;
                }
                case BID:{
                    if (verbose) cout << "[LOG]: " << getpid() << " Got BID request from " << host << ":" << service << endl;

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
                        
                        to_print = "[LOG]: " + to_string(getpid()) + " accepted and created bid from " + uid + " on auction " + aid + "\n";
                        cout << to_print;
                        response += "ACC\n";
                    }
                    break;
                }
                default:{
                    string target = "[LOG]: " + to_string(getpid()) + " Couldn't identify TCP request from " + host + ":" + service + "\n";
                    throw target;
                }
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

    // cout << "[LOG]: " << getpid() << " Sending response " << response;
    sendTCPresponse(fd, response, response.size());

    close(fd);
}

void startTCP() {

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

    int n_tcp = ::bind(fd_tcp, res->ai_addr, res->ai_addrlen);
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
    int tcp_child;
    int errcode;
    while (true) {
        addrlen = sizeof(addr);

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
                ret = close(tcp_child);
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

            if ((errcode = getnameinfo((struct sockaddr *) &addr, addrlen, host, sizeof(host), service, sizeof(service), NI_NUMERICHOST)) != 0) {
                cout << "[LOG]: TCP Error getnameinfo: " << gai_strerror(errcode) << endl;
            }
            
            handleTCPRequest(tcp_child);
            if (shmdt(sharedAID) == -1) {
                cout << "[LOG]: " << getpid() << " erro detaching shared memory" << endl;
            }
            if (verbose) cout << "[LOG]: " << getpid() << " Terminating after handling request" << endl; 
            
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

    if (verbose) cout << "[LOG]: Port: " << port << '\n';

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    int errcode = getaddrinfo(NULL, port.c_str(), &hints, &res);
    if (errcode != 0) {
        cout << gai_strerror(errcode);
        exit(EXIT_FAILURE);
    }

    key_t key = ftok("SharedData", 0);
    int shmid = shmget(key, sizeof(SharedAID), 0666 | IPC_CREAT);
    sharedAID = (SharedAID*) shmat(shmid, (void*)0, 0);

    sharedAID->AID = 1;
    sem_init(&sharedAID->sem, 1, 1);

    pid_t pid = fork();
    if (pid == -1) {
        cout << "Fork error" << endl;
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        startUDP();
    } else {
        signal(SIGINT, SIG_DFL);
        startTCP();
    }
    
    return 0;
}