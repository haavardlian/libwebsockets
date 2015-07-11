//
// Created by H�vard on 13.06.2015.
//

#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <WebSocketServer.h>
#include <Base64.h>
#include <sha1.h>

using namespace libwebsockets;

WebSocketServer::WebSocketServer(string IP, uint16 Port, string Endpoint)
{
	this->Endpoint = regex(Endpoint);

	Client c(SocketType::STREAM, IP, Port, bind(&WebSocketServer::HandleConnectionEvent, this, _1));
	AddToPoll(c);
}

int WebSocketServer::WaitForSockets(int Milliseconds)
{
	vector<struct pollfd> pfds(Sockets.size());
	vector<struct pollfd>::size_type i = 0;
	for(Client & socket : Sockets)
	{
		pfds[i].fd = socket.GetFileDescriptor();
		pfds[i].events = POLLIN;
		i++;
	}

	int ret = poll(&pfds[0], (int)Sockets.size(), Milliseconds);

	for(i = 0; i < Sockets.size(); i++)
	{
		if((pfds[i].revents & POLLIN) != 0)
			Sockets[i].HandleEvent();
	}

	return ret;
}

int WebSocketServer::Run()
{
	int nRet;
	while(true)
	{
		nRet = WaitForSockets(DEFAULT_WAIT);
		if(nRet < 0)
			break;
	}
	return nRet;
}

void WebSocketServer::HandleConnectionEvent(Client& socket)
{
	struct sockaddr_in client_addr;
	socklen_t client_length = sizeof(client_addr);
	int client = accept(socket.GetFileDescriptor(), (struct sockaddr *)&client_addr, &client_length);
	size_t BufferSize = 1500;
	//Buffer for current packet
	vector<uint8> Buffer(BufferSize);
	ssize_t ReadBytes;
	try
	{
		Client ClientSocket = Client(SocketType::STREAM, client, bind(&WebSocketServer::HandleClientEvent, this, _1));
		ReadBytes = read(ClientSocket.GetFileDescriptor(), Buffer.data(), BufferSize);
		Buffer[ReadBytes] = '\0';
		map<string, string> header = ParseHTTPHeader(string((const char*) &Buffer[0]));

		//TODO: Regex match the endpoint with this->Endpoint

		string appended = header["Sec-WebSocket-Key"] + GID;
		uint8 hash[20];
		sha1::calc(appended.c_str(), static_cast<const int>(appended.length()), hash);

		string HashedKey = Base64Encode(hash, 20);
		string handshake = "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: " + HashedKey + "\r\n\r\n";

		send(ClientSocket.GetFileDescriptor(), handshake.c_str(), handshake.length(), 0);

		AddToPoll(ClientSocket);

		if(OnOpen != nullptr) OnOpen(ClientSocket);

	}
	catch(const exception& ex)
	{
		cout << ex.what() << endl;
	}

}

void WebSocketServer::HandleClientEvent(Client& socket)
{
	try
	{
		WebSocketHeader Header = socket.ReadHeader();
		socket.ReadMessage(Header);

		if(Header.IsFinal)
		{
            switch(Header.Opcode)
            {
                case WebSocketOpcode::CLOSE:
                    socket.SetState(WebSocketState::CLOSING);
                    vector<uint8> reason;

                    if(socket.GetMessageSize())
                        reason.insert(reason.end(), socket.GetMessage().begin(), socket.GetMessage().begin() + 2);

                    socket.SendMessage(reason, WebSocketOpcode::CLOSE);

                    if(OnClose != nullptr) OnClose(socket);
                    RemoveFromPoll(socket);
                    break;
                case WebSocketOpcode::PING:
                    socket.SendMessage(socket.GetMessage(), WebSocketOpcode::PONG);
                    if(OnPing != nullptr) OnPing(socket);
                    break;
                case WebSocketOpcode::PONG:
                    if(OnPong != nullptr) OnPong(socket);
                    break;
                default:
                    if(OnMessage != nullptr) OnMessage(socket);
                    break;
            }

            if(Header.Opcode != WebSocketOpcode::CLOSE)
			    socket.ResetMessage();
		}

	}
	catch(const exception& ex)
	{
		cout << ex.what() << endl;
	}

}

map<string, string> WebSocketServer::ParseHTTPHeader(string header)
{
	std::string delimiter = "\r\n";
	map<string, string> m;
	size_t pos = 0;
	std::string token;
	while ((pos = header.find(delimiter)) != std::string::npos) {
		token = header.substr(0, pos);

		size_t colon = token.find(':');

		if(colon != string::npos)
		{
			int skip = 1;
			if(token.at(colon+skip) == ' ') skip++;
			string key = token.substr(0, colon);
			string value = token.substr(colon+skip);
			m.insert(make_pair(key, value));
		}
		else
		{
			if(token.find("GET ") == 0)
			{
				m.insert(make_pair("Request", token));
			}
		}

		header.erase(0, pos + delimiter.length());
	}

	return m;
}

int WebSocketServer::RemoveFromPoll(Client &socket)
{
	auto pos = std::find(Sockets.begin(), Sockets.end(), socket);
	if(pos != Sockets.end())
	{
		socket.Close();
		Sockets.erase(pos);
	}

	return 0;
}
