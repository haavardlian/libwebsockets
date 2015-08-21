#include <iostream>
#include <WebSocketServer.h>

using namespace std;
using namespace libwebsockets;

WebSocketServer *server;

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
	server->SendToAll(socket, socket.GetMessage(), socket.GetMessageType());
}

class HandlerClass
{
public:
	HandlerClass() {};
	virtual ~HandlerClass() {};
	void HandleOpen(Client& client)
	{
		cout << "New socket opened: " << client.GetFileDescriptor() << endl;
	}
};


int main() {
	server = new WebSocketServer("127.0.0.1", 8154, "/");
    HandlerClass handler;

	//Set up static handler methods
	server->OnClose = &CloseHandler;
	server->OnPong = &PongHandler;
	server->OnMessage = &OnMessage;
	//Set up a handler that is a member of a class
	server->OnOpen = bind(&HandlerClass::HandleOpen, &handler, placeholders::_1);

	cout << "Started server on " << server->GetIP() << ":" << server->GetPort() << endl;

	//Handle main flow if additional tasks needs to be performed
//	while(true)
//	{
//		auto ret = ws.WaitForSockets(DEFAULT_WAIT);
//
//		if(ret == 0)
//		{
//			//Time out from poll, do other housekeeping tasks
//			cout << "Timeout" << endl;
//		}
//		if(ret < 0)
//		{
//			//Poll returned an error
//			cout << "Error in poll: " << strerror(errno) << endl;
//			break;
//		}
//	}
	//Just run the server
	auto ret = server->Run();

    return ret;
}
