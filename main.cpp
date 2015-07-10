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
	if(socket.GetMessageType() == WebSocketOpcode::TEXT)
		cout << "Got message:" << endl << socket.GetMessageString() << endl;

	//Echo the message back
	socket.SendMessage(socket.GetMessage(), socket.GetMessageType());

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
	WebSocketServer ws = WebSocketServer("127.0.0.1", 8154, "/");
	HandlerClass handler;

	//Set up static handler methods
	ws.OnClose = &CloseHandler;
	ws.OnPong = &PongHandler;
	ws.OnMessage = &OnMessage;
	//Set up a handler that is a member of a class
	ws.OnOpen = bind(&HandlerClass::HandleOpen, &handler, placeholders::_1);

	//Handle main flow if additional tasks needs to be performed
//	while(true)
//	{
//		auto ret = ws.WaitForSockets(5000);
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
	ws.Run();

    return 1;
}
