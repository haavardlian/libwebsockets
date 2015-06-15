//
// Created by H�vard on 13.06.2015.
//

#include <stdio.h>
#include <string.h>
#include "WebSocketServer.h"
#include "Base64.h"
#include "sha1.h"

using namespace libwebsockets;

WebSocketServer WebSocketServer::instance;
bool WebSocketServer::init = false;

int WebSocketServer::WaitForSockets(int Milliseconds)
{
	struct pollfd pfds[Sockets.size()];
	int i = 0;
	for(auto& socket : Sockets)
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

int WebSocketServer::HandleConnectionEvent(Socket & socket)
{
	auto& ws = WebSocketServer::Instance();
	struct sockaddr_in client_addr;
	socklen_t client_length = sizeof(client_addr);
	int client = accept(socket.GetFileDescriptor(), (struct sockaddr *)&client_addr, &client_length);
	size_t BufferSize = 1500;
	uint8 Buffer[BufferSize]; //Buffer for current packet
	size_t ReadBytes;
	Socket ClientSocket = Socket(SocketType::STREAM, client, &HandleClientEvent);

	try
	{
		ReadBytes = ClientSocket.Read(Buffer, BufferSize);
		Buffer[ReadBytes] = '\0';

		map<string, string> header = ParseHTTPHeader(string((const char*)Buffer));

		string appended = header["Sec-WebSocket-Key"] + GID;
		uint8 hash[20];
		sha1::calc(appended.c_str(), appended.length(), hash);

		string HashedKey = Base64Encode(hash, 20);

		char handshake[BufferSize];

		sprintf(handshake, "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: %s\r\n\r\n", HashedKey.c_str());

		send(ClientSocket.GetFileDescriptor(), handshake, strlen(handshake), 0);

		ws.AddToPoll(ClientSocket);

		if(ws.OnOpen != nullptr) ws.OnOpen(ClientSocket);

	}
	catch(const exception& ex)
	{
		cout << "Could not read from socket" << endl;
	}

}

int WebSocketServer::HandleClientEvent(Socket &socket)
{
	auto& ws = WebSocketServer::Instance();
	size_t BufferSize = 1500;
	uint8 Buffer[BufferSize]; //Buffer for current packet

	size_t ReadBytes;
	try
	{
		ReadBytes = socket.Read(Buffer, BufferSize);
		Buffer[ReadBytes] = '\0';
		size_t MessageOffset = 2;
		struct WebSocketHeader header;

		if(Buffer[0] & 0x80) header.IsFinal = true;
		else header.IsFinal = false;
		if(Buffer[1] & 0x80) header.IsMasked = true;
		else header.IsMasked = false;

		header.Opcode = static_cast<WebSocketOpcode >(Buffer[0] & 0x0F);


		if(Buffer[1] == 0x7E)
		{
			uint16* tempSize = (uint16*) &Buffer[2];
			header.Length = ntohs(*tempSize);
			MessageOffset += 2;
		}
		else if(Buffer[1] == 0x7F)
		{
			uint64* tempSize = (uint64*) &Buffer[2];
			header.Length = ntohll(*tempSize);
			MessageOffset += 8;
		}
		else header.Length = Buffer[1] & 0x7F;

		if(header.IsMasked)
		{
			uint32* tempMask = (uint32*) &Buffer[MessageOffset];
			header.MaskingKey = *tempMask;
			MessageOffset += 4;
		}

		socket.AddToMessage(&Buffer[MessageOffset], ReadBytes - MessageOffset, header);

		if(header.IsFinal)
		{
			if(header.Opcode == WebSocketOpcode::CLOSE)
			{
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

				if(ws.OnClose != nullptr) ws.OnClose(socket);
				ws.RemoveFromPoll(socket);
			}
			else if(header.Opcode == WebSocketOpcode::PING)
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

				memcpy(buffer + offset, socket.GetMessage(), socket.GetMessageSize());

				int sent = send(socket.GetFileDescriptor(), buffer, offset + socket.GetMessageSize(), 0x0);

				if(sent != sizeof(buffer))
				{
					cout << "ERROR: Could not send ping" << endl;
				}

				if(ws.OnPing != nullptr) ws.OnPing(socket);
			}
			else if(header.Opcode == WebSocketOpcode::PONG)
			{
				if(ws.OnPong != nullptr) ws.OnPong(socket);
			}
			else
			{
				if(header.Opcode == WebSocketOpcode::TEXT && socket.GetMessageSize() < MAX_MESSAGE_SIZE)
					socket.GetMessage()[socket.GetMessageSize()] = 0;
				//Call some function
				if(ws.OnMessage != nullptr) ws.OnMessage(socket);

			}
			socket.ResetMessage();
		}

		return 0;
	}
	catch(const exception& ex)
	{
		cout << "Could not read from socket" << endl;
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

int WebSocketServer::RemoveFromPoll(Socket &Socket)
{
	for(int i = 0; i < Sockets.size(); i++)
	{
		if(&Sockets[i] == &Socket)
		{
			Sockets[i].Close();
			Sockets.erase(Sockets.begin() + i);

			return 0;
		}
	}
}
