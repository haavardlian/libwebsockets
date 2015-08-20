//
// Created by Håvard on 13.06.2015.
//

#include <Client.h>
#include <WebSocketServer.h>

using namespace libwebsockets;

Client::Client(SocketType Type, std::string ip, uint16 port, std::function<void(Client&)> callback)
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
    this->IP = ip;
    this->Port = port;
    this->events = POLLIN;

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

    fd = socket(AF_INET, actualType, 0);
    if(fd < 0)
    {
        throw std::runtime_error("Could not open socket");
    }

    err = bind(fd, (struct sockaddr*) &ServerAddress, sizeof(ServerAddress));

    if(Type == SocketType::STREAM || Type == SocketType::SEQUENTIAL)
    {
        err = listen(fd, MAX_CONNECTION_QUEUE_SIZE);
    }

    if(err < 0)
    {
        throw std::runtime_error("Could not bind/listen to socket");
    }

    this->State = WebSocketState::OPEN;
}

Client::Client(SocketType Type, int fd, std::function<void(Client&)> callback)
{
    this->Type = Type;
    this->fd = fd;
    this->State = WebSocketState::OPEN;
    this->Handler = callback;
    this->events = POLLIN;
}

size_t Client::ReadMessage(struct WebSocketHeader Header)
{
    if(Header.Length == 0)
        return 0;

    std::vector<uint8> buffer(Header.Length);
    ssize_t Read = read(fd, buffer.data(), Header.Length);
    if(Read < 0)
    {
        throw std::runtime_error("Could not read from client socket");
    }

    AddToMessage(buffer, Header);

    return (size_t)Read;
}

WebSocketHeader Client::ReadHeader()
{
    uint8 buffer[2];
    struct WebSocketHeader header;

    read(fd, buffer, 2);

    header.IsFinal = (buffer[0] & 0x80) != 0;
    header.IsMasked = (buffer[1] & 0x80) != 0;
    header.Opcode = static_cast<WebSocketOpcode >(buffer[0] & 0x0F);

    uint8 size = static_cast<uint8>(buffer[1] & 0x7F);
    if(size == WEBSOCKET_16BIT_SIZE)
    {
        uint16 tempSize;
        read(fd, &tempSize, 2);
        header.Length = ntohs(tempSize);
    }
    else if(size == WEBSOCKET_64BIT_SIZE)
    {
        uint64 tempSize;
        read(fd, &tempSize, 8);
        header.Length = ntohll(tempSize);
    }
    else header.Length = size;

    if(header.IsMasked)
        read(fd, &header.MaskingKey, sizeof(header.MaskingKey));

    return header;
}

int Client::Close()
{
    State = WebSocketState::CLOSED;
    return close(fd);
}

int Client::AddToMessage(std::vector<uint8>& Buffer, struct WebSocketHeader Header)
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

    send(fd, buffer, 2, 0);
}

void Client::SendString(std::string message)
{
    std::vector<uint8> buffer(message.begin(), message.end());
    SendMessage(buffer, WebSocketOpcode::TEXT);
}

void Client::SendMessage(std::vector<uint8>& Buffer, WebSocketOpcode MessageType)
{
    std::vector<uint8> Message;
    Message.push_back((uint8) 0x80 | static_cast<uint8>(MessageType));

    if(Buffer.size() < WEBSOCKET_16BIT_SIZE)
    {
        Message.push_back((uint8) Buffer.size());
    }
    else if(Buffer.size() < 65535)
    {
        Message.push_back(WEBSOCKET_16BIT_SIZE);
        auto tempSize = htons((uint16) Buffer.size());
        auto data = reinterpret_cast<uint8*>(&tempSize);
        Message.insert(Message.end(), data, data + sizeof(uint16));
    }
    else
    {
        Message.push_back(WEBSOCKET_64BIT_SIZE);
        auto tempSize = htonll(Buffer.size());
        auto data = reinterpret_cast<uint8*>(&tempSize);
        Message.insert(Message.end(), data, data + sizeof(uint64));
    }

    Message.insert(Message.end(), Buffer.begin(), Buffer.end());


    ssize_t sent = send(fd, Message.data(), Message.size(), 0);

    if(sent != static_cast<ssize_t>(Message.size()))
    {
        throw std::runtime_error("Could not send message to client");
    }

}

std::string Client::GetMessageString()
{
    std::ostringstream oss;
    copy(Message.begin(), Message.end(), std::ostream_iterator<uint8>(oss));

    return oss.str();
}