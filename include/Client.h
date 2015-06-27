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

namespace libwebsockets {

	#define MAX_CONNECTION_QUEUE_SIZE 10
	#define MAX_MESSAGE_SIZE 0xFFFF
	#define MAX_BUFFER_SIZE 0x5DC
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
                    	Client(SocketType Type, string ip, uint16 port, int	(*Handler)(Client &));
						Client(SocketType Type, int fd, int	(*Handler)(Client &));
        virtual     	~Client() {}
        SocketType  	GetType() { return Type; };
        int         	GetFileDescriptor() { return FileDescriptor; };
        size_t      	Read(uint8 *buffer, size_t size);
		int				Close();
		int 			HandleEvent() { return Handler(*this);};
        int         	AddToMessage(uint8* Buffer, size_t Size, struct WebSocketHeader Header);
        void        	ResetMessage() { CurrentBufferPos = 0; };
        uint8*     		GetMessage() { return Message; };
        size_t      	GetMessageSize() { return CurrentBufferPos; };
		WebSocketState	GetState() { return State; };
		void			SetState(WebSocketState state) { State = state; };
        void            SendPing();
        void            SendMessage(uint8 *Buffer, size_t Size, WebSocketOpcode MessageType);

        bool operator==(const Client & s) const
        {
            return (&s == this);
        }

    private:
        SocketType  Type;
        int         FileDescriptor;
		int			(*Handler)(Client &);
		uint8 		Message[MAX_MESSAGE_SIZE];
		int			CurrentBufferPos = 0;
		WebSocketState State;

    };
}

#endif //PROJECT_SOCKET_H
