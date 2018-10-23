#include "websocketAgent.h"



websocketAgent::websocketAgent()
{
	
}


websocketAgent::~websocketAgent()
{
}


void ConnectToRoom(std::string roomUrl)
{
	uWS::Hub h;
	h.onError([](void *user) {
		switch ((long)user) {
		case 1:
			std::cout << "Client emitted error on what?" << std::endl;
			break;
		default:
			std::cout << "FAILURE: " << user << " should not emit error!" << std::endl;
			exit(-1);
		}
	});

	h.onConnection([](uWS::WebSocket<uWS::CLIENT> *ws, uWS::HttpRequest req) {
		switch ((long)ws->getUserData()) {
		case 1:
			std::cout << "Client established a remote connection over non-SSL" << std::endl;
			ws->close(1000);
			break;
		default:
			std::cout << "FAILURE: " << ws->getUserData() << " should not connect!" << std::endl;
			exit(-1);
		}
	});

	h.onDisconnection([](uWS::WebSocket<uWS::CLIENT> *ws, int code, char *message, size_t length) {
		std::cout << "Client got disconnected with data: " << ws->getUserData() << ", code: " << code << ", message: <" << std::string(message, length) << ">" << std::endl;
	});

	std::map<std::string, std::string> protocol_map;
	protocol_map.insert(pair<string, std::string>(string("1"), string("janus-protocol")));
	h.connect("ws://39.106.100.180:8188", (void *)1, protocol_map);
	h.run();
	std::cout << "Falling through testConnections" << std::endl;

}


void DisconnectFromRoom()
{

}


void KeepAlive()
{

}


void CreateSession()
{

}
