//
// Created by Håvard on 13.06.2015.
//

#ifndef LIBWEBSOCKETS_WEBSOCKETSERVER_H
#define LIBWEBSOCKETS_WEBSOCKETSERVER_H

#include "Client.h"
#include <poll.h>
#include <vector>
#include <map>

namespace libwebsockets
{
	using namespace std;


	typedef void (*MessageHandler)(Client & socket);

	#define GID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
	#define TEMP_BUFFER_SIZE 2048;

	class WebSocketServer
	{
	public:
		virtual ~WebSocketServer(){};
		int AddToPoll(Client Socket) { Sockets.push_back(Socket); };
		int RemoveFromPoll(Client & Socket);
		vector<Client>& GetSockets() { return Sockets; };
		int WaitForSockets(int Milliseconds);
		MessageHandler OnMessage = nullptr;
		MessageHandler OnPing = nullptr;
		MessageHandler OnPong = nullptr;
		MessageHandler OnClose = nullptr;
		MessageHandler OnOpen = nullptr;

		static WebSocketServer& CreateInstance(string ip, uint16 port)
		{
			if(!init)
			{
				init = true;
				instance.AddToPoll(Client(SocketType::STREAM, ip, port, &HandleConnectionEvent));
			}
			return instance;
		};

		static WebSocketServer& Instance()
		{
			if(!init) throw exception();
			return instance;
		};
	private:
		WebSocketServer() {};
		WebSocketServer(WebSocketServer const&) {};
		WebSocketServer& operator=(WebSocketServer const&){};
		vector<Client>	Sockets;
		static WebSocketServer instance;
		static bool init;
		static int HandleConnectionEvent(Client & socket);
		static int HandleClientEvent(Client & socket);
		static map<string, string>		ParseHTTPHeader(string header);
	};
}

#endif //LIBWEBSOCKETS_WEBSOCKETSERVER_H
