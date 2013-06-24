#include "Connector.h"

#include <exception>

#include "Datagram.pb.h"
#include "MemoryUtils.h"
#include "WsaEvent.h"
#include "WsaException.h"
#include "WsaInit.h"
#include "WsaSocket.h"

using namespace ru::org::codingteam::styx;

Connector::Connector()
	: _started(false), _threadId(0)
{
}

Connector::~Connector()
{
}

void Connector::start()
{
	if (_started)
	{
		throw std::exception("Thread already started");
	}

	CreateThread(nullptr, 0, &loop, this, 0, &_threadId);
}

void Connector::stop()
{
	if (_started)
	{
		// TODO: send signal to the thread.
	}
}

std::unique_ptr<addrinfo, decltype(&freeaddrinfo)> Connector::GetServerAddress()
{
	const auto host = "127.0.0.1";
	const auto defaultPort = "6060";

	auto address = addrinfo();
	address.ai_family = AF_UNSPEC;
	address.ai_socktype = SOCK_STREAM;
	address.ai_protocol = IPPROTO_TCP;

	addrinfo *temp = nullptr;

	// Resolve the server address and port:
	auto code = getaddrinfo(host, defaultPort, &address, &temp);

	auto deleter = &freeaddrinfo;
	auto result = MemoryUtils::MakeUniquePtr(temp, deleter);

	if (code)
	{
		throw WsaException("getaddrinfo failed", code);
	}

	return result;
}

DWORD Connector::loop(LPVOID self)
{
	auto connector = static_cast<Connector*>(self);
	auto init = WsaInit();
	auto address = connector->GetServerAddress();
	auto socket = WsaSocket(address->ai_family, address->ai_socktype, address->ai_protocol);
	socket.connect(*address);
	auto event = WsaEvent();
	auto socketHandle = socket.handle();
	auto eventHandle = event.handle();
	WSAEventSelect(socketHandle, eventHandle, FD_WRITE | FD_READ | FD_CLOSE);
	
	while (true)
	{
		auto waitResult = WaitForMultipleObjects(1, &eventHandle, false, INFINITE); // TODO: Wait for local events (i.e. new message in the queue).
		if (waitResult != WAIT_OBJECT_0)
		{
			throw std::exception("Wait failed");
		}

		auto message = Message(); // TODO: Get message from the local queue.
		auto size = message.ByteSize();

		socket.send(size);
		socket.send(message.SerializeAsString());
	}
}