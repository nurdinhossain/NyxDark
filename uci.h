// uci protocol
#pragma once    
#include <string>
#include <vector>
using namespace std;

// constants
const string ENGINE_NAME = "NYX";
const string ENGINE_AUTHOR = "Nurdin Hossain";

// method for splitting string
vector<string> split(const string& s, char delimiter);

// method for calculating time
int getTime(int timeLeft, int inc, int movesToGo, int historyIndex);

// method for uci protocol
void uci();