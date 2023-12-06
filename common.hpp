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

using namespace std;

void parseInput(string &input, vector<string> &inputs);

void parseInput(char *input, vector<string> &inputs);

int checkUID(string &uid);

int checkPassword(string &pw);

int checkAID(string &aid);

int checkName(string &name);

int checkPrice(string &price);

int checkTime(string &time);

bool checkStartValue(string& svalue);

bool checkDuration(string& duration);

string openJPG(string fname);

#endif