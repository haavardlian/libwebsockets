//
// Created by Håvard on 13.06.2015.
//

#include <string.h>
#include "Client.h"

using namespace libwebsockets;

Client::Client(SocketType Type, string ip, uint16 port,  int (*Handler)(Client &))
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
        throw runtime_error("Could not open socket");
    }

    err = bind(FileDescriptor, (struct sockaddr*) &ServerAddress, sizeof(ServerAddress));

	if(Type == SocketType::STREAM || Type == SocketType::SEQUENTIAL)
	{
		err = listen(FileDescriptor, MAX_CONNECTION_QUEUE_SIZE);
	}

	if(err < 0)
	{
		throw runtime_error("Could not bind/listen to socket");
	}

}

Client::Client(SocketType Type, int fd, int (*Handler)(Client &))
{
	this->Type = Type;
	this->FileDescriptor = fd;
	this->Handler = Handler;
    this->State = WebSocketState::OPENING;
}

size_t Client::Read(uint8 *Buffer, size_t size)
{
    ssize_t Read = recv(FileDescriptor, Buffer, size, 0);
	if(Read < 0)
	{
		throw runtime_error("Could not read from client socket");
	}

	return (size_t)Read;
}

int Client::Close()
{
	return close(FileDescriptor);
}

int Client::AddToMessage(uint8* Buffer, size_t Size, struct WebSocketHeader Header)
{
	if (CurrentBufferPos + Size > MAX_MESSAGE_SIZE)
	{
		throw runtime_error("Message is too big");
	}

	if(Header.IsFinal)
	{
		MessageType = Header.Opcode;
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

void Client::SendPing()
{
    uint8 buffer[2];
    buffer[0] = 0x89;
    buffer[1] = 0x0;

    send(FileDescriptor, buffer, 2, 0);
}

void Client::SendMessage(uint8 *Buffer, size_t Size, WebSocketOpcode MessageType)
{
    uint8 Message[Size + 10];
    size_t MessageOffset = 2;
    Message[0] = (uint8) 0x80 | static_cast<uint8>(MessageType);

    if(Size < 126)
    {
        Message[1] = (uint8) Size;
    }
    else if(Size < 65535)
    {
        Message[1] = 126;
        MessageOffset += 2;
        *((uint16*) &Buffer[2]) = (uint16) Size;
    }
    else
    {
        MessageOffset += 8;
        Message[1] = 127;
        *((uint64*) &Buffer[2]) = Size;
    }

    memcpy(&Message[MessageOffset], Buffer, Size);

    ssize_t sent = send(FileDescriptor, Message, Size + MessageOffset, 0);

    if(sent != Size + MessageOffset)
    {
        throw runtime_error("Could not send message to client");
    }

}
