// uci protocol
#pragma once    
#include <string>
#include <vector>
#include <unordered_map>
using namespace std;

// constants
const string ENGINE_NAME = "NYX";
const string ENGINE_AUTHOR = "Nurdin Hossain";

// method for calculating time
int getTime(int timeLeft, int inc, int movesToGo, int historyIndex);

// method for uci protocol
void uci(unordered_map<string, vector<string>> book);