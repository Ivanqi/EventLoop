#include "fastcgi.h"
#include "EventLoop.h"
#include "TcpServer.h"
#include <iostream>

const string kPath = "/fastcgi/";

void onRequest(const TcpConnectionPtr& conn, FastCgiCodec::ParamMap& params, Buffer *in)
{
	string uri = params["REQUEST_URI"];

	for (FastCgiCodec::ParamMap::const_iterator it = params.begin(); it != params.end(); ++it) {
		std::cout << it->first << " = " << it->second << std::endl;
	}
	
	if (in->readableBytes() > 0) {
		std::cout << "stdin " << in->retrieveAllAsString() << std::endl;
	}

	Buffer response;
	response.append("Context->Type: text/plain\r\n\r\n");

	if (uri.find(kPath) == 0) {
		response.append("good request!");
	} else {
		response.append("bad request!");
	}

	FastCgiCodec::respond(&response);
	conn->send(&response);
}

void onConnection(const TcpConnectionPtr& conn)
{
	if (conn->connected()) {
		typedef std::shared_ptr<FastCgiCodec> CodecPtr;
		CodecPtr codec(new FastCgiCodec(onRequest));

		conn->setContext(codec);
		conn->setMessageCallback(std::bind(&FastCgiCodec::onMessage, codec, _1, _2, _3));
		conn->setTcpNoDelay(true);
	}
}

int main(int argc, char* argv[]) {

	int port = 19981;
	int threads = 0;

	if (argc > 1) {
		port = atoi(argv[1]);
	}

	if (argc > 2) {
		threads = atoi(argv[2]);
	}

	InetAddress addr(static_cast<uint16_t>(port));

	EventLoop loop;
	TcpServer server(&loop, addr, "FastCGI");
	server.setConnectionCallback(onConnection);
	server.setThreadNum(threads);
	server.start();
	loop.loop();

	return 0;
}