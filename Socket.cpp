//
// Created by Håvard on 13.06.2015.
//

#include <string.h>
#include "Socket.h"

using namespace libwebsockets;

Socket::Socket(SocketType Type, string ip, uint16 port,  int (*Handler)(Socket &))
{
    struct sockaddr_in ServerAddress;
    int err;
    ServerAddress.sin_family = AF_INET;
    ServerAddress.sin_port = htons(port);
    if(ip.length() == 0)
        ServerAddress.sin_addr.s_addr = INADDR_ANY;
    else
        inet_aton(ip.c_str(), &ServerAddress.sin_addr);


    this->Type = Type;
	this->Handler = Handler;
    this->State = WebSocketState::OPENING;

    int actualType = 0;
    switch (Type)
    {
        case SocketType::STREAM:
            actualType = SOCK_STREAM;
            break;
        case SocketType::DATAGRAM:
            actualType = SOCK_DGRAM;
            break;
        case SocketType::SEQUENTIAL:
            actualType = SOCK_SEQPACKET;
            break;
        case SocketType::RANDOM:
            actualType = SOCK_RDM;
            break;
        default:
            actualType = SOCK_DGRAM;
    }

    FileDescriptor = socket(AF_INET, actualType, 0);
    if(FileDescriptor < 0)
    {
        throw exception();
    }

    err = bind(FileDescriptor, (struct sockaddr*) &ServerAddress, sizeof(ServerAddress));

	if(Type == SocketType::STREAM || Type == SocketType::SEQUENTIAL)
	{
		err = listen(FileDescriptor, MAX_CONNECTION_QUEUE_SIZE);
	}

	if(err < 0)
	{
		throw exception();
	}

}

Socket::Socket(SocketType Type, int fd, int (*Handler)(Socket &))
{
	this->Type = Type;
	this->FileDescriptor = fd;
	this->Handler = Handler;
    this->State = WebSocketState::OPENING;
}

size_t Socket::Read(uint8 *Buffer, size_t size)
{
    ssize_t Read = recv(FileDescriptor, Buffer, size, 0);
	if(Read < 0)
	{
		throw exception();
	}

	return (size_t)Read;
}

int Socket::Close()
{
	return close(FileDescriptor);
}

int Socket::AddToMessage(uint8* Buffer, size_t Size, struct WebSocketHeader Header)
{
	if (CurrentBufferPos + Size > MAX_MESSAGE_SIZE)
	{
		throw exception();
	}

	if (!Header.IsMasked)
		memcpy(&Message[CurrentBufferPos], Buffer, Size);
	else
	{
		for(int i = 0; i < Size; i++)
		{
			Message[CurrentBufferPos+i] = Buffer[i] ^ ((uint8*) &Header.MaskingKey)[i%4];
		}
	}
    CurrentBufferPos += Size;

    return 0;
}