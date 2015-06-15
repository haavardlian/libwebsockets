#include <iostream>
#include "PollableSocket.h"
#include "WebSocketServer.h"
using namespace std;
using namespace libwebsockets;

void CloseHandler(PollableSocket& socket)
{
	cout << "Closed socket " << socket.GetFileDescriptor() << endl;
}

void OpenHandler(PollableSocket& socket)
{

	uint8 buffer[2];
	buffer[0] = 0x89;
	buffer[1] = 0x0;
	cout << "Opened new connection, sending ping" << endl;
	send(socket.GetFileDescriptor(), buffer, 2, 0);
}

void PongHandler(PollableSocket& socket)
{
	cout << "Got pong from " << socket.GetFileDescriptor() << endl;
}

void OnMessage(PollableSocket& socket)
{
	cout << "Got message:" << endl << socket.GetMessage() << endl;
}

int main() {

	auto& ws = WebSocketServer::CreateInstance("127.0.0.1", 1337);

	ws.OnClose = &CloseHandler;
	ws.OnOpen = &OpenHandler;
	ws.OnPong = &PongHandler;
	ws.OnMessage = &OnMessage;


	while(true)
	{
		auto ret = ws.WaitForSockets(1000);

		if(ret == 0)
			//cout << "Timeout..." << endl;
		if(ret < 0)
			break;
	}

    return 0;
}