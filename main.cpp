#include <iostream>
#include <sys/errno.h>
#include <string.h>
#include <WebSocketServer.h>

using namespace std;
using namespace libwebsockets;

void CloseHandler(Client & socket)
{
	cout << "Closed socket " << socket.GetFileDescriptor() << endl;
}

void OpenHandler(Client & socket)
{

	cout << "Opened new connection, sending ping" << endl;
	socket.SendPing();
}

void PongHandler(Client & socket)
{
	cout << "Got pong from " << socket.GetFileDescriptor() << endl;
}

void OnMessage(Client & socket)
{
	cout << "Got message:" << endl << socket.GetMessageString() << endl;

	socket.SendMessage(socket.GetMessage(), WebSocketOpcode::TEXT);

}

int main() {
	//Create instance and get reference to it
	auto& ws = WebSocketServer::CreateInstance("127.0.0.1", 8154);

	//Set up handlers for events
	ws.OnClose = &CloseHandler;
	ws.OnOpen = &OpenHandler;
	ws.OnPong = &PongHandler;
	ws.OnMessage = &OnMessage;

	//Main program loop
	while(true)
	{
		auto ret = ws.WaitForSockets(1000);

		if(ret == 0)
		{
			//Time out from poll, do other housekeeping tasks
		}
		if(ret < 0)
		{
			//Poll returned an error
			cout << "Error in poll: " << strerror(errno) << endl;
			break;
		}
	}

    return 1;
}
