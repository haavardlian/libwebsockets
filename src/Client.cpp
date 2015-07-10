//
// Created by Håvard on 13.06.2015.
//

#include <string.h>
#include <Client.h>
#include <WebSocketServer.h>
using namespace libwebsockets;

Client::Client(SocketType Type, string ip, uint16 port, function<void(Client&)> callback)
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
    this->Handler = callback;

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

Client::Client(SocketType Type, int fd, function<void(Client&)> callback)
{
    this->Type = Type;
    this->FileDescriptor = fd;
    this->State = WebSocketState::OPEN;
    this->Handler = callback;
}

size_t Client::ReadMessage(struct WebSocketHeader Header)
{
    if(Header.Length == 0)
        return 0;

    vector<uint8> buffer(Header.Length);
    ssize_t Read = read(FileDescriptor, &buffer[0], Header.Length);
	if(Read < 0)
	{
		throw runtime_error("Could not read from client socket");
	}

    AddToMessage(buffer, Header);

	return (size_t)Read;
}

WebSocketHeader Client::ReadHeader()
{
    uint8 buffer[2];
    struct WebSocketHeader header;

    read(FileDescriptor, buffer, 2);

    if(buffer[0] & 0x80)
        header.IsFinal = true;
    else
        header.IsFinal = false;

    if(buffer[1] & 0x80)
        header.IsMasked = true;
    else
        header.IsMasked = false;

    header.Opcode = static_cast<WebSocketOpcode >(buffer[0] & 0x0F);

    uint8 size = static_cast<uint8>(buffer[1] & 0x7F);
    if(size == WEBSOCKET_16BIT_SIZE)
    {
        uint16 tempSize;
        read(FileDescriptor, &tempSize, 2);
        header.Length = ntohs(tempSize);
    }
    else if(size == WEBSOCKET_64BIT_SIZE)
    {
        uint64 tempSize;
        read(FileDescriptor, &tempSize, 8);
        header.Length = ntohll(tempSize);
    }
    else header.Length = size;

    if(header.IsMasked)
        read(FileDescriptor, &header.MaskingKey, sizeof(uint32));

    return header;
}

int Client::Close()
{
    State = WebSocketState::CLOSED;
	return close(FileDescriptor);
}

int Client::AddToMessage(vector<uint8>& Buffer, struct WebSocketHeader Header)
{
	if(Header.IsFinal)
		MessageType = Header.Opcode;

	if (!Header.IsMasked)
        Message.insert(Message.end(), Buffer.begin(), Buffer.end());
	else
	{
        //TODO: Optimize by iteration over uint32 as much as possible
		for(uint64 i = 0; i < Header.Length; i++)
            Message.push_back(Buffer[i] ^ ((uint8*) &Header.MaskingKey)[i % 4]);
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
    //TODO: Possibly use a std::vector here?
    vector<uint8> Message(Buffer.size() + 10);
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

    ssize_t sent = send(FileDescriptor, &Message[0], length, 0);

    if(sent != static_cast<ssize_t>(Buffer.size() + MessageOffset))
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