//
// Created by Håvard on 13.06.2015.
//

#ifndef WEBSOCKETSERVER_H
#define WEBSOCKETSERVER_H

#include <Client.h>
#include <poll.h>
#include <vector>
#include <map>
#include <regex>
#include <algorithm>

#define GID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
#define DEFAULT_WAIT 1000

namespace libwebsockets
{
    using namespace std::placeholders;
    typedef std::function<void(Client&)> callback;

    class WebSocketServer
    {
    public:
                             WebSocketServer(std::string IP, uint16 Port, std::string Endpoint);
        virtual              ~WebSocketServer(){};
        void                 AddToPoll(Client Socket) { Sockets.push_back(Socket); };
        int                  RemoveFromPoll(Client & Socket);
        uint16 GetPort()     { return Sockets[0].GetPort(); };
        std::string GetIP()  { return Sockets[0].GetIP(); };
        std::vector<Client>& GetSockets() { return Sockets; };
        int                  WaitForSockets(int Milliseconds);
        int                  Run();
        void                 SendToAll(Client& Sender, std::vector<uint8> Message, WebSocketOpcode MessageType, bool SendToSelf = true);
        callback             OnMessage = nullptr;
        callback             OnPing = nullptr;
        callback             OnPong = nullptr;
        callback             OnClose = nullptr;
        callback             OnOpen = nullptr;
    private:
        void                                HandleConnectionEvent(Client & socket);
        void                                HandleClientEvent(Client & socket);
        std::map<std::string, std::string>  ParseHTTPHeader(std::string header);
        std::vector<Client>                 Sockets;
        std::regex                          Endpoint;

    };
}

#endif //LIBWEBSOCKETS_WEBSOCKETSERVER_H
