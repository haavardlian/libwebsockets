//
// Created by Håvard on 13.06.2015.
//

#ifndef PROJECT_SOCKET_H
#define PROJECT_SOCKET_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <iterator>
#include <sstream>

namespace libwebsockets {

	#define MAX_CONNECTION_QUEUE_SIZE 10
    #define htonll(x) ((1==htonl(1)) ? (x) : ((uint64_t)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32))
    #define ntohll(x) ((1==ntohl(1)) ? (x) : ((uint64_t)ntohl((x) & 0xFFFFFFFF) << 32) | ntohl((x) >> 32))

    #define WEBSOCKET_16BIT_SIZE 0x7E
    #define WEBSOCKET_64BIT_SIZE 0x7F

	using namespace std;

    typedef unsigned char uint8;
    typedef unsigned short uint16;
    typedef unsigned int uint32;
    typedef unsigned long long uint64;

    enum class WebSocketOpcode
    {
        TEXT = 0x1,
        BINARY = 0x2,
        CLOSE = 0x8,
        PING = 0x9,
        PONG = 0xA
    };

	enum class WebSocketState
	{
		OPENING,
		OPEN,
		CLOSING,
		CLOSED
	};

    struct WebSocketHeader {
		bool IsFinal;
        bool IsMasked;
        WebSocketOpcode Opcode;
        uint64 Length;
        uint32 MaskingKey;
    };

	enum SocketType {
        STREAM,
        DATAGRAM,
        SEQUENTIAL,
        RAW,
        RANDOM
    };

    class Client
	{
    public:
                    	Client(SocketType Type, string ip, uint16 port, function<void(Client&)> callback);
                        Client(SocketType Type, int fd, function<void(Client&)> callback);
        virtual     	~Client() {}
        SocketType  	GetType() { return Type; };
        int         	GetFileDescriptor() { return FileDescriptor; };
        size_t      	ReadMessage(struct WebSocketHeader Header);
        WebSocketHeader ReadHeader();
		int				Close();
		void 			HandleEvent() { Handler(*this);};
        int         	AddToMessage(vector<uint8>& Buffer, struct WebSocketHeader Header);
        void        	ResetMessage() { Message.clear(); };
        vector<uint8>&  GetMessage() { return Message; };
        string          GetMessageString();
        size_t      	GetMessageSize() { return Message.size(); };
		WebSocketState	GetState() { return State; };
		void			SetState(WebSocketState state) { State = state; };
        void            SendPing();
        void            SendMessage(vector<uint8>& Buffer, WebSocketOpcode MessageType);
        void            SendString(string message);
		WebSocketOpcode GetMessageType() { return MessageType; };
        bool operator==(const Client & s) const
        {
            return (&s == this);
        }

    private:
        SocketType      Type;
        int             FileDescriptor;
		vector<uint8>   Message;
		WebSocketState  State;
		WebSocketOpcode MessageType;
        function<void(Client&)> Handler;
    };
}

#endif //PROJECT_SOCKET_H
