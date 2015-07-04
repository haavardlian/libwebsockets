//
// Created by H�vard on 13.06.2015.
//

#include <string.h>
#include <Client.h>

using namespace libwebsockets;

Client::Client(SocketType Type, string ip, uint16 port)
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

    this->State = WebSocketState::OPEN;
}

Client::Client(SocketType Type, int fd)
{
	this->Type = Type;
	this->FileDescriptor = fd;
    this->State = WebSocketState::OPENING;
}

size_t Client::ReadMessage(struct WebSocketHeader Header)
{
    if(Header.Length == 0)
        return 0;

    uint8 Buffer[Header.Length];
    ssize_t Read = recv(FileDescriptor, Buffer, Header.Length, 0);
	if(Read < 0)
	{
		throw runtime_error("Could not read from client socket");
	}

    AddToMessage(&Buffer[0], Header);

	return (size_t)Read;
}

WebSocketHeader Client::ReadHeader()
{
    uint8 Buffer[2];
    struct WebSocketHeader header;

    read(FileDescriptor, Buffer, 2);

    if(Buffer[0] & 0x80) header.IsFinal = true;
    else header.IsFinal = false;
    if(Buffer[1] & 0x80) header.IsMasked = true;
    else header.IsMasked = false;

    header.Opcode = static_cast<WebSocketOpcode >(Buffer[0] & 0x0F);


    uint8 smallSize = static_cast<uint8>(Buffer[1] & 0x7F);

    if(smallSize == 0x7E)
    {
        uint16 tempSize;
        read(FileDescriptor, &tempSize, sizeof(uint16));
        header.Length = ntohs(tempSize);
    }
    else if(smallSize == 0x7F)
    {
        uint64 tempSize;
        read(FileDescriptor, &tempSize, sizeof(uint64));
        header.Length = ntohll(tempSize);
    }
    else header.Length = smallSize;

    if(header.IsMasked)
    {
        read(FileDescriptor, &header.MaskingKey, sizeof(uint32));
    }

    return header;
}

int Client::Close()
{
    State = WebSocketState::CLOSED;
	return close(FileDescriptor);
}

int Client::AddToMessage(uint8* Buffer, struct WebSocketHeader Header)
{
	if(Header.IsFinal)
	{
		MessageType = Header.Opcode;
	}

	if (!Header.IsMasked)
    {
        Message.insert(Message.end(), &Buffer[0], &Buffer[0] + Header.Length);
    }
	else
	{
		for(int i = 0; i < Header.Length; i++)
		{
            Message.push_back(Buffer[i] ^ ((uint8*) &Header.MaskingKey)[i%4]);
		}
	}

    return 0;
}

void Client::SendPing()
{
    uint8 buffer[2];
    buffer[0] = 0x89;
    buffer[1] = 0x0;

    send(FileDescriptor, buffer, 2, 0);
}

void Client::SendMessage(vector<uint8>& Buffer, WebSocketOpcode MessageType)
{
    uint8 Message[Buffer.size() + 10];
    size_t MessageOffset = 2;
    Message[0] = (uint8) 0x80 | static_cast<uint8>(MessageType);

    if(Buffer.size() < 126)
    {
        Message[1] = (uint8) Buffer.size();
    }
    else if(Buffer.size() < 65535)
    {
        Message[1] = 0x7E;
        MessageOffset += 2;
        *((uint16*) &Message[2]) = htons((uint16) Buffer.size());
    }
    else
    {
        MessageOffset += 8;
        Message[1] = 0x7F;
        *((uint64*) &Message[2]) = htonll(Buffer.size());
    }

    memcpy(&Message[MessageOffset], &Buffer[0], Buffer.size());

    size_t length = Buffer.size() + MessageOffset;

    ssize_t sent = send(FileDescriptor, Message, length, 0);

    if(sent != Buffer.size() + MessageOffset)
    {
        throw runtime_error("Could not send message to client");
    }

}

string Client::GetMessageString()
{
    ostringstream oss;
    copy(Message.begin(), Message.end(), ostream_iterator<uint8>(oss));

    return oss.str();
}