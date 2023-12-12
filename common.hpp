#ifndef COMMON_H
#define COMMON_H

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

void checkPrice(string &price);

void checkTime(string &time);

void checkStartValue(string& svalue);

void checkDuration(string& duration);

void checkFileName(string &fname);

void checkFileSize(string &fname);

string openJPG(string fname);

void printVectorString(vector<string> &target);

string getSubString(string const&target, int start, int size);

void concatenateString(string &target, char item[], int size);

#endif