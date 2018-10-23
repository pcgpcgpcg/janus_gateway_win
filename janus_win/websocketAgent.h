#pragma once
#include<iostream>
#include <uWS.h>

using namespace std;
using namespace uWS;

//session exchange over websocket
class websocketAgent
{
public:
	websocketAgent();
	~websocketAgent();
public:
	void ConnectToRoom(std::string roomUrl);
	void DisconnectFromRoom();
	void KeepAlive();
	void CreateSession();

private:
	uWS::Hub h;
};

