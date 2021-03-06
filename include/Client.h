//
// Created by H�vard on 13.06.2015.
//

#ifndef CLIENT_H
#define CLIENT_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <string.h>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <regex>
#include <iterator>
#include <sstream>
#include <sys/poll.h>

#define MAX_CONNECTION_QUEUE_SIZE 10
#define htonll(x) ((1==htonl(1)) ? (x) : ((uint64_t)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32))
#define ntohll(x) ((1==ntohl(1)) ? (x) : ((uint64_t)ntohl((x) & 0xFFFFFFFF) << 32) | ntohl((x) >> 32))

#define WEBSOCKET_16BIT_SIZE 0x7E
#define WEBSOCKET_64BIT_SIZE 0x7F

namespace libwebsockets
{
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
        STREAM = SOCK_STREAM,
        DATAGRAM = SOCK_DGRAM,
        SEQUENTIAL = SOCK_SEQPACKET,
        RAW = SOCK_RAW,
        RANDOM = SOCK_RDM
    };

    class Client : public pollfd
    {
    public:
                            Client(SocketType Type, std::string ip, uint16 port, std::function<void(Client&)> callback);
                            Client(SocketType Type, int fd, std::function<void(Client&)> callback);
        virtual             ~Client() {}
        SocketType          GetType() { return Type; };
        int                 GetFileDescriptor() { return fd; };
        size_t              ReadMessage(struct WebSocketHeader Header);
        WebSocketHeader     ReadHeader();
        int                 Close();
        void                HandleEvent() { Handler(*this);};
        int                 AddToMessage(std::vector<uint8>& Buffer, struct WebSocketHeader Header);
        void                ResetMessage() { Message.clear(); };
        std::vector<uint8>& GetMessage() { return Message; };
        std::string         GetMessageString();
        size_t              GetMessageSize() { return Message.size(); };
        WebSocketState      GetState() { return State; };
        void                SetState(WebSocketState state) { State = state; };
        void                SendPing();
        void                SendMessage(std::vector<uint8>& Buffer, WebSocketOpcode MessageType);
        void                SendString(std::string message);
        WebSocketOpcode     GetMessageType() { return MessageType; };
        void                SetRegexResult(std::smatch& result) { RegexResult = result; };
        std::smatch&        GetRegexResult() { return RegexResult; };
        uint16              GetPort() { return Port;};
        std::string         GetIP() { return IP; };
        bool operator==(const Client& c) const { return (&c == this); };

    private:
        SocketType                      Type;
        std::vector<uint8>              Message;
        WebSocketState                  State;
        WebSocketOpcode                 MessageType;
        std::function<void(Client&)>    Handler;
        std::smatch                     RegexResult;
        std::string                     IP;
        uint16                          Port;
    };
}

#endif //CLIENT_H
