//
// Created by Håvard on 13.06.2015.
//

#ifndef LIBWEBSOCKETS_WEBSOCKETSERVER_H
#define LIBWEBSOCKETS_WEBSOCKETSERVER_H

#include "Socket.h"
#include <poll.h>
#include <vector>
#include <map>

namespace libwebsockets
{
	using namespace std;


	typedef void (*MessageHandler)(Socket & socket);

	#define GID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
	#define TEMP_BUFFER_SIZE 2048;
	#define htonll(x) ((1==htonl(1)) ? (x) : ((uint64_t)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32))
	#define ntohll(x) ((1==ntohl(1)) ? (x) : ((uint64_t)ntohl((x) & 0xFFFFFFFF) << 32) | ntohl((x) >> 32))

	class WebSocketServer
	{
	public:
		virtual ~WebSocketServer(){};
		int AddToPoll(Socket Socket) { Sockets.push_back(Socket); };
		int RemoveFromPoll(Socket & Socket);
		vector<Socket>& GetSockets() { return Sockets; };
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
				instance.AddToPoll(Socket(SocketType::STREAM, ip, port, &HandleConnectionEvent));
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
		vector<Socket>	Sockets;
		static WebSocketServer instance;
		static bool init;
		static int HandleConnectionEvent(Socket & socket);
		static int HandleClientEvent(Socket & socket);
		static map<string, string>		ParseHTTPHeader(string header);
	};
}

#endif //LIBWEBSOCKETS_WEBSOCKETSERVER_H
