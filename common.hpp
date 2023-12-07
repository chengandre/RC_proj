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

bool checkUID(string &uid);

bool checkPasswordSyntax(string &pw);

bool checkAID(string &aid);

bool checkName(string &name);

bool checkPrice(string &price);

bool checkTime(string &time);

bool checkStartValue(string& svalue);

bool checkDuration(string& duration);

bool checkFileSize(string &fname);

string openJPG(string fname);

void printVectorString(vector<string> &target);

string getSubString(string const&target, int start, int size);

#endif