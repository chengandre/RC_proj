#include "common.hpp"

using namespace std;

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

void checkUID(string &uid) {
    // primeiro digito tem que ser um (foi dito no slack)
    // 6 digitos
    if ((all_of(uid.begin(), uid.end(), ::isdigit) && uid.length() == 6 && uid.at(0) == '1') == 0) {
        throw string("UID Syntax error");
    }
}

void checkPasswordSyntax(string &pw) {
    // 8 numeros ou letras
    if ((all_of(pw.begin(), pw.end(), ::isalnum) && pw.length() == 8) == 0) {
        throw string("PASSWORD Syntax error");
    }
}

void checkAID(string &aid) {
    if ((all_of(aid.begin(), aid.end(), ::isdigit) && aid.length() == 3) == 0) {
        throw string("AID Syntax error");
    }
}

void checkName(string &name) {
    if ((all_of(name.begin(), name.end(), ::isalnum) && name.length() <= 10) == 0) {
        throw string("Auction name syntax error");
    }
}

void checkPrice(string &price) {
    if (all_of(price.begin(), price.end(), ::isdigit) == 0) {
        throw string("Auction Price syntax error");
    }
}

void checkTime(string &time) {
    if (all_of(time.begin(), time.end(), ::isdigit) == 0) {
        throw string("Time Syntax error");
    }
}

void checkStartValue(string& svalue) {
    if ((all_of(svalue.begin(), svalue.end(),::isdigit) && svalue.length() <= 6) == 0) {
        throw string("Auction Start Value Syntax error");
    }
}

void checkDuration(string& duration) {
    if ((all_of(duration.begin(), duration.end(),::isdigit) && duration.length() <= 5) == 0) {
        throw string("Auction Duration Syntax error");
    }
}

bool isalnumplus(char c) {
    return isalnum(c) || c == '.' || c == '_' || c == '-';
}

void checkFileName(string &fname) {
    if ((all_of(fname.begin(), fname.end(),::isalnumplus) && fname.length() <= 24) == 0) {
        throw string("File name syntax error");
    }
}

void checkFileSize(string &fsize_str) {
    ssize_t fsize;
    stringstream ss(fsize_str);;
    ss >> fsize;
    if ((0 < fsize && fsize <= MAXFILESIZE) == 0) {
        throw string("File size error");
    }
}

string openJPG(string fname) {
    ifstream fin(fname, ios::binary);
    if (!fin) {
        throw string("Error opening file to read");
    }
    ostringstream oss;
    oss << fin.rdbuf();
    return oss.str();
}

void printVectorString(vector<string> &target) {
    for (int i = 0; i < target.size(); i++) {
        cout << target[i] << endl;
    }
}

string getSubString(string const &target, int start, int size) {
    string tmp;
    int j = 0;
    while (j < size && start+j < target.size()) {
        tmp.push_back(target[start+j]);
        j++;
    }

    return tmp;
}

void concatenateString(string &target, char item[], int size) {
    for (int i = 0; i < size; i++) {
        target.push_back(item[i]);
    }
}