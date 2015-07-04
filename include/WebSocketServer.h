//
// Created by Håvard on 13.06.2015.
//

#ifndef LIBWEBSOCKETS_WEBSOCKETSERVER_H
#define LIBWEBSOCKETS_WEBSOCKETSERVER_H

#include "Client.h"
#include <poll.h>
#include <vector>
#include <map>
#include <regex>

namespace libwebsockets
{
	using namespace std;
	using namespace placeholders;

	#define GID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
	#define TEMP_BUFFER_SIZE 2048;

	class WebSocketServer
	{
	public:
		WebSocketServer(string IP, uint16 Port, string Endpoint);
		virtual ~WebSocketServer(){};
		void AddToPoll(Client Socket) { Sockets.push_back(Socket); };
		int RemoveFromPoll(Client & Socket);
		vector<Client>& GetSockets() { return Sockets; };
		int WaitForSockets(int Milliseconds);
		function<void(Client&)> OnMessage = nullptr;
		function<void(Client&)> OnPing = nullptr;
		function<void(Client&)> OnPong = nullptr;
		function<void(Client&)> OnClose = nullptr;
		function<void(Client&)> OnOpen = nullptr;
	private:
		void 					HandleConnectionEvent(Client & socket);
		void					HandleClientEvent(Client & socket);
		map<string, string>		ParseHTTPHeader(string header);
		vector<Client>			Sockets;
		regex					Endpoint;



	};
}

#endif //LIBWEBSOCKETS_WEBSOCKETSERVER_H
