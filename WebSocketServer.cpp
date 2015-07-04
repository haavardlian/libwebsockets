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

	Client c(SocketType::STREAM, IP, Port);
	c.Handler = bind(&WebSocketServer::HandleConnectionEvent, this, _1);
	AddToPoll(c);

}

int WebSocketServer::WaitForSockets(int Milliseconds)
{
	struct pollfd pfds[Sockets.size()];
	int i = 0;
	for(Client & socket : Sockets)
	{
		pfds[i].fd = socket.GetFileDescriptor();
		pfds[i].events = POLLIN;
		i++;
	}

	int ret = poll(&pfds[0], (int)Sockets.size(), Milliseconds);

	for(int i = 0; i < Sockets.size(); i++)
	{
		if((pfds[i].revents & POLLIN) != 0)
			Sockets[i].HandleEvent();
	}

	return ret;
}

void WebSocketServer::HandleConnectionEvent(Client& socket)
{
	struct sockaddr_in client_addr;
	socklen_t client_length = sizeof(client_addr);
	int client = accept(socket.GetFileDescriptor(), (struct sockaddr *)&client_addr, &client_length);
	size_t BufferSize = 1500;
	uint8 Buffer[BufferSize]; //Buffer for current packet
	size_t ReadBytes;
	try
	{
		Client ClientSocket = Client(SocketType::STREAM, client);
		ClientSocket.Handler = bind(&WebSocketServer::HandleClientEvent, this, _1);
		ReadBytes = recv(ClientSocket.GetFileDescriptor(), Buffer, BufferSize, 0);
		Buffer[ReadBytes] = '\0';

		map<string, string> header = ParseHTTPHeader(string((const char*)Buffer));

		string appended = header["Sec-WebSocket-Key"] + GID;
		uint8 hash[20];
		sha1::calc(appended.c_str(), appended.length(), hash);

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
			if(Header.Opcode == WebSocketOpcode::CLOSE)
			{
				socket.SetState(WebSocketState::CLOSING);
				//TODO: Use client class to send close ack.
				uint8 buffer[4];

				bool payload = socket.GetMessageSize() > 0;

				buffer[0] = 0x88;
				if(payload)
				{
					buffer[1] = 2;
					buffer[2] = socket.GetMessage()[0];
					buffer[3] = socket.GetMessage()[1];
				}
				else
					buffer[1] = 0;

				send(socket.GetFileDescriptor(), buffer, payload?4:2, 0);

				if(OnClose != nullptr) OnClose(socket);
				RemoveFromPoll(socket);
			}
			else if(Header.Opcode == WebSocketOpcode::PING)
			{
				uint8 buffer[0xFFFF];
				buffer[0] = 0x89;

				size_t size = socket.GetMessageSize();
				int offset = 2;
				if(size < 0x7E)
					buffer[0] = size;
				else if(size < 0x7F)
				{
					buffer[0] = 0x7E;
					offset += 2;
					*((uint16*) &buffer[2]) = (uint16) size;
				}
				else
				{
					buffer[0] = 0x7F;
					offset += 8;
					*((uint64*) &buffer[2]) = (uint64) size;
				}

				memcpy(buffer + offset, &socket.GetMessage()[0], socket.GetMessageSize());

				int sent = send(socket.GetFileDescriptor(), buffer, offset + socket.GetMessageSize(), 0x0);

				if(sent != sizeof(buffer))
				{
					cout << "ERROR: Could not send ping" << endl;
				}

				if(OnPing != nullptr) OnPing(socket);
			}
			else if(Header.Opcode == WebSocketOpcode::PONG)
			{
				if(OnPong != nullptr) OnPong(socket);
			}
			else
			{
				//Call user function
				if(OnMessage != nullptr) OnMessage(socket);

			}
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
}
