#ifndef COMMON_H
#define COMMON_H

#include <unistd.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <vector>
#include <string.h>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <filesystem>

#define MAXFILESIZE 10000000

using namespace std;

void parseInput(string &input, vector<string> &inputs);

void parseInput(char *input, vector<string> &inputs);

void checkUID(string &uid);

void checkPasswordSyntax(string &pw);

void checkAID(string &aid);

void checkName(string &name);

void checkStartValue(string& svalue);

void checkDuration(string& duration);

void checkFileName(string &fname);

void checkFileSize(string &fname);

void checkDate(string &date);

void checkHour(string &hour);

string getSubString(string const&target, size_t start, size_t size);

void concatenateString(string &target, char item[], int size);

void checkUDPSyntax(string request);

int sendTCPmessage(int fd, string &message, int size);

int sendTCPmessage(int fd, char message[], int size);

int sendTCPfile(int fd, string &fpath);

int receiveTCPsize(int fd, int size, string &request);

int receiveTCPspace(int fd, int size, string &request);

int receiveTCPend(int fd, string &response);

void removeFile(string &path);

void removeDir(string &path);

#endif