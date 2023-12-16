#include "common.hpp"

using namespace std;
// Parsing Functions

// Store space split components in vector for c++ object string
void parseInput(string &input, vector<string> &inputs) {
    inputs.clear();

    istringstream stream(input);
    string tmp;

    while (stream >> tmp) {
        inputs.push_back(tmp); // append each argument
    }
}
// Store space split components in vector for c strings (char *)
void parseInput(char *input, vector<string> &inputs) {
    inputs.clear();

    istringstream stream(input);
    string tmp;

    while (stream >> tmp) {
        inputs.push_back(tmp); // append each argument
    }
}
// Appends an array of char to a string
void concatenateString(string &target, char item[], int size) {
    for (int i = 0; i < size; i++) {
        target.push_back(item[i]);
    }
}


// Checking functions to verify input format



// checks if an UID is valid 
void checkUID(string &uid) {
    // 6 digits
    if ((all_of(uid.begin(), uid.end(), ::isdigit) && uid.length() == 6) == 0) {
        throw string("UID Syntax error");
    }
}


// checks if a password is valid
void checkPasswordSyntax(string &pw) {
    // 8 numbers or letters
    if ((all_of(pw.begin(), pw.end(), ::isalnum) && pw.length() == 8) == 0) {
        throw string("PASSWORD Syntax error");
    }
}


// checks if an AID is valid
void checkAID(string &aid) {
    // 3 digits (tejo starts with 001, but it does not say it can't be 000 so we allow it)
    if ((all_of(aid.begin(), aid.end(), ::isdigit) && aid.length() == 3) == 0) {
        throw string("AID Syntax error");
    }
}

bool isalnumminus(char c) {
    return isalnum(c) || c == '_' || c == '\u002d';
}


// checks if an auction name is valid
void checkName(string &name) {
    // no more than 10 alphanumeric chars
    if ((all_of(name.begin(), name.end(), ::isalnumminus) && name.length() <= 10) == 0) {
        throw string("Auction name syntax error");
    }
}


// checks if the start value is valid
void checkStartValue(string& svalue) {
    // no more than 6 digits
    if ((all_of(svalue.begin(), svalue.end(),::isdigit) && svalue.length() <= 6) == 0) {
        throw string("Auction Start Value Syntax error");
    }
}


// check if the duration of an auction is valid
void checkDuration(string& duration) {
    // no more than 5 digits
    if ((all_of(duration.begin(), duration.end(),::isdigit) && duration.length() <= 5) == 0) {
        throw string("Auction Duration Syntax error");
    }
}


bool isalnumplus(char c) {
    return isalnum(c) || c == '.' || c == '_' || c == '\u002d';
}


// checks if the asset file name is valid
void checkFileName(string &fname) {
    // No more than 24 chars
    if ((all_of(fname.begin(), fname.end(),::isalnumplus) && fname.length() <= 24) == 0) {
        throw string("File name syntax error");
    }
}


// checks if the asset file size is too big
void checkFileSize(string &fsize_str) {
    ssize_t fsize;
    stringstream ss(fsize_str);;
    ss >> fsize;
    // should not be empty nor bigger than 10MB
    if ((0 < fsize && fsize <= MAXFILESIZE) == 0) {
        throw string("File size error");
    }
}


// checks if the date is valid and its format
void checkDate(string &date) {
    if (date.at(4) != '-' || date.at(7) != '-') { // YYYY-MM-DD
        throw string("Invalid date");
    } else {
        string year = getSubString(date, 0, 4);
        string month = getSubString(date, 5, 2);
        string day = getSubString(date, 8, 2);

        int iyear = stoi(year);
        int imonth = stoi(month);
        int iday = stoi(day);

        if (iyear < 1970 || iyear > 2100) {
            throw string("Invalid Date -> Year");
        } else if (imonth < 1 || imonth > 12) {
            throw string("Invalid Date -> Month");
        } else if (iday < 1 || imonth > 31) {
            throw string("Invalid Date -> Day");
        }
    }
}



// checks if the time is valid with correct format
void checkHour(string &hour) {
    if (hour.at(2) != ':' || hour.at(5) != ':') { // HH:MM:SS
        throw string("Invalid hour");
    } else {
        string str_hours = getSubString(hour, 0, 2);
        string str_minutes = getSubString(hour, 3, 2);
        string str_seconds = getSubString(hour, 6, 2);

        int hours = stoi(str_hours);
        int minutes = stoi(str_minutes);
        int seconds = stoi(str_seconds);

        if (hours < 0 || hours > 23) {
            throw string("Invalid Time -> Hour");
        } else if (minutes < 0 || minutes > 59) {
            throw string("Invalid Time -> Minutes");
        } else if (seconds < 0 || seconds > 59) {
            throw string("Invalid Time -> Seconds");
        }
    }
}


// Returns a part of a string
string getSubString(string const &target, size_t start, size_t size) {
    string tmp;
    size_t j = 0;
    while (j < size && start+j < target.size()) {
        tmp.push_back(target[start+j]);
        j++;
    }

    return tmp;
}


// Checks the overall syntax of an UDP message
void checkUDPSyntax(string request) {
    bool end_found = false;
    bool space_found = false;
    size_t i = 0;
    while (i < request.size()) {

        // '\n' should be the last byte
        if (request.at(i) == '\n' && !end_found) {
            end_found = true;
        } else if (request.at(i) == '\n') {
            throw string("Invalid UDP message");
        }

        // there shouldn't be 2 spaces together
        if (request.at(i) == ' ' && !space_found) {
            space_found = true;
        } else if (request.at(i) == ' ' && space_found) {
            throw string("Invalid UDP message");
        } else if (request.at(i) != ' ' && space_found) {
            space_found = false;
        }

        i++;
    }

    // '\n' should be the last byte
    if (request.back() != '\n') {
        throw string("Invalid UDP message");
    }
}


// Sends a response to user through the TCP socket
int sendTCPmessage(int fd, string &message, int size) {
    int total_sent = 0; // bytes sent
    int n; // bytes sent at a time
    while (total_sent < size) { // while message has not been sent totally
        n = write(fd, message.c_str() + total_sent, size - total_sent);
        if (n == -1) {
            throw string("Error while sending TCP message, probably due to syntax error");
        }
        total_sent += n;
    }

    return total_sent;
}


// Sends a response to user through the TCP socket
int sendTCPmessage(int fd, char message[], int size) {
    int total_sent = 0; // bytes sent
    int n; // bytes sent at a time
    while (total_sent < size) { // while message has not been sent totally
        n = write(fd, message + total_sent, size - total_sent);
        if (n == -1) {
            throw string("Error while sending TCP message, probably due to syntax error");
        }
        total_sent += n;
    }

    return total_sent;
}


// Sends a file through the TCP socket (reads and sends 128 bytes at a time)
int sendTCPfile(int fd, string &fpath) {
    int total_sent = 0; // total byte sent
    int n_read, n_sent; // bytes read from file, bytes sent through socket
    char tmp[128];
    ifstream fin(fpath); // open file to read
    if (!fin) {
        throw string("Error opening file to read");
    }

    while( !fin.eof()) { // Read untill EOF is reached
        fin.read(tmp, 128);
        n_read = fin.gcount();
        n_sent = sendTCPmessage(fd, tmp, n_read);
        if (n_sent != n_read) { // number of bytes read should be the number of bytes sent
            throw string("TCP send error, probably due to syntax error");
        }
        total_sent += n_sent;
    }

    fin.close();
    return total_sent;
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
            throw string("Error while reading from TCP socket, probably due to Syntax Error");
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
            throw string("Error while reading from TCP socket, probably due to Syntax Error");
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
            throw string("Error while reading from TCP socket, probably due to Syntax Error");
        } else if (tmp[0] == '\n') {
            // bytes is a '\n' then quit
            break;
        }
        concatenateString(response, tmp, n);
        total_received += n;
        
    }

    return total_received;
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