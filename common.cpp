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

bool checkUID(string &uid) {
    // primeiro digito tem que ser um (foi dito no slack)
    // 6 digitos
    return all_of(uid.begin(), uid.end(), ::isdigit) && uid.length() == 6 && uid.at(0) == '1';
}

bool checkPasswordSyntax(string &pw) {
    // 8 numeros ou letras
    return all_of(pw.begin(), pw.end(), ::isalnum) && pw.length() == 8;
}

bool checkAID(string &aid) {
    return all_of(aid.begin(), aid.end(), ::isdigit) && aid.length() == 3;
}

bool checkName(string &name) {
    return all_of(name.begin(), name.end(), ::isalnum) && name.length() <= 10;
}

bool checkPrice(string &price) {
    return all_of(price.begin(), price.end(), ::isdigit);
}

bool checkTime(string &time) {
    return all_of(time.begin(), time.end(), ::isdigit);
}

bool checkStartValue(string& svalue) {
    return all_of(svalue.begin(), svalue.end(),::isdigit) && svalue.length() <= 6;
}

bool checkDuration(string& duration) {
    return all_of(duration.begin(), duration.end(),::isdigit) && duration.length() <= 5;
}

bool isalnumplus(char c) {
    return isalnum(c) || c == '.' || c == '_' || c == '-';
}

bool checkFileName(string &fname) {
    return all_of(fname.begin(), fname.end(),::isalnumplus) && fname.length() <= 24;
}

bool checkFileSize(string &fname) {
    return filesystem::file_size(fname) <= MAXFILESIZE;
}

string openJPG(string fname) {
    ifstream fin(fname, ios::binary);
    ostringstream oss;
    oss << fin.rdbuf();
    return oss.str();
}