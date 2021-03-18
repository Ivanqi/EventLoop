#include "EventLoop.h"
#include "TcpServer.h"
#include "hash.h"

#include <fstream>
#include <stdio.h>

class WordCountReceiver
{
	private:
		EventLoop *loop_;
		TcpServer server_;
		int senders_;
		WordCountMap wordcounts_;
	
	public:
		WordCountReceiver(EventLoop *loop, const InetAddress& listenAddr)
			:loop_(loop), server_(loop, listenAddr, "WordCountReceiver"),
			senders_(0)
		{
			server_.setConnectionCallback(std::bind(&WordCountReceiver::onConnection, this, _1));

			server_.setMessageCallback(std::bind(&WordCountReceiver::onMessage, this, _1, _2, _3));
		}

		void start(int senders)
		{
			printf("start %d senders\n", senders);
			senders_ = senders;
			wordcounts_.clear();
			server_.start();
		}
	
	private:
		void onConnection(const TcpConnectionPtr& conn)
		{
			string state = conn->connected() ? "UP" : "DOWN";
    		printf("EchoServer - %s -> %s is %s\n", conn->peerAddress().toIpPort().c_str(), conn->localAddress().toIpPort().c_str(), state.c_str());

			if (!conn->connected()) {
				if (--senders_ == 0) {
					output();
					loop_->quit();
				}
			}
		}

		void onMessage(const TcpConnectionPtr& conn, Buffer *buf, Timestamp)
		{
			const char *crlf = NULL;
			while ((crlf  = buf->findCRLF()) != NULL) {
				const char *tab = std::find(buf->peek(), crlf, '\t');
				if (tab != crlf) {
					string word(buf->peek(), tab);
					int64_t cnt = atoll(tab);
					wordcounts_[word] += cnt;
				} else {
					printf("Wrong format, no tab found\n");
					conn->shutdown();
				}
				buf->retrieveUntil(crlf + 2);
			}
		}

		void output()
		{
			printf("Writing shard\n");
			std::ofstream out("shard");

			for (WordCountMap::iterator it = wordcounts_.begin(); it != wordcounts_.end(); ++it) {
				out << it->first << '\t' << it->second << '\n';
			}
		}
};

int main(int argc, char* argv[]) {

	if (argc < 3) {
		printf("Usage: %s listen_port number_of_senders\n", argv[0]);
	} else {
		EventLoop loop;
    	int port = atoi(argv[1]);
    	InetAddress addr(static_cast<uint16_t>(port));
    	WordCountReceiver receiver(&loop, addr);
    	receiver.start(atoi(argv[2]));
    	loop.loop();
	}
	return 0;
}