#pragma once
#include <string>
#include <thread>

const int BUFFER_SIZE = 1024;
const int PORT = 12345;
const std::string SERVER_IP = "127.0.0.1";

int connectSocket();
void closeSocket(int sock);
std::thread initListenThread(int sock, std::string& buffer);
void sendMessage(int sock, std::string message);
void killListenThread(std::thread& listenThread);