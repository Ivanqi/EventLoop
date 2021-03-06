#include "CountDownLatch.h"
#include "EventLoop.h"
#include "EventLoopThreadPool.h"
#include "TcpClient.h"

#include <boost/program_options.hpp>
#include <iostream>
#include <stdio.h>

namespace po = boost::program_options;

class Client
{
	public:
		enum Operation
		{
			kGet,	// get命令
			kSet,	// set 命令
		};

	private:
		string name_;	// client 名称
		TcpClient client_;	// tcp client
		TcpConnectionPtr conn_; // tcp connect
		const Operation op_;	// 状态
		int send_;	// 发送数
		int acked_;	// 应答数
		const int requests_;	// 最大请求数
		const int keys_;	// 客户端的密钥数
		const int valuelen_;
		string value_;
		CountDownLatch* const connected_;	// 启动连接的条件变量
		CountDownLatch* const finished_;	// 关闭连接的条件变量
	
	public:
		Client(const string& name, EventLoop *loop, const InetAddress& serverAdrr, Operation op, 
		int requests, int keys, int valuelen, CountDownLatch *connected, CountDownLatch *finished)
			:name_(name), client_(loop, serverAdrr, name), op_(op), send_(0),
			acked_(0), requests_(requests), keys_(keys), valuelen_(valuelen), value_(valuelen_, 'a'),
			connected_(connected), finished_(finished)
		{
			value_ += "\r\n";
			client_.setConnectionCallback(std::bind(&Client::onConnection, this, _1));
			client_.setMessageCallback(std::bind(&Client::onMessage, this, _1, _2, _3));
			client_.connect();
		}

		void send()
		{
			Buffer buf;
			fill(&buf);
			conn_->send(&buf);
		}
	
	private:
		void onConnection(const TcpConnectionPtr& conn)
		{
			if (conn->connected()) {
				conn_ = conn;
				connected_->countDown();	// 当有连接的时候，通知所有线程
			} else {
				conn_.reset();
				client_.getLoop()->queueInLoop(std::bind(&CountDownLatch::countDown, finished_));
			}
		}

		void onMessage(const TcpConnectionPtr& conn, Buffer *buffer, Timestamp receiveTime)
		{
			if (op_ == kSet) {
				while (buffer->readableBytes() > 0) {
					const char *crlf = buffer->findCRLF();
					if (crlf) {
						buffer->retrieveUntil(crlf + 2);
						++acked_;
						if (send_ < requests_) {
							send();
						}
					} else {
						break;
					}
				}
			} else {
				while (buffer->readableBytes() > 0) {
					/**
					 * memmem
					 * 	在一块内存中寻找匹配另一块内存的内容的第一个位置
					 * 	返回值：返回该位置的指针，如找不到，返回空指针
					 */
					const char *end = static_cast<const char*>(memmem(buffer->peek(), buffer->readableBytes(), "END\r\n", 5));

					if (end) {
						buffer->retrieveUntil(end + 5);
						++acked_;
						if (send_ < requests_) {
							send();
						}
					} else {
						break;
					}
				}
			}

			// 达成最大请求数，关闭连接
			if (acked_ == requests_) {
				conn_->shutdown();
			}
		}

		void fill(Buffer *buf)
		{
			char req[255];
			if (op_ == kSet) {
				snprintf(req, sizeof(req), "set %s%d 42 0 %d\r\n", name_.c_str(), send_ % keys_, valuelen_);
				++send_;
				buf->append(req);
				buf->append(value_);
			} else {
				snprintf(req, sizeof(req), "get %s%d\r\n", name_.c_str(), send_ % keys_);
				++send_;
				buf->append(req);
			}
		}
};

int main(int argc, char* argv[]) {

	uint16_t tcpport = 11211;
	string hostIp = "127.0.0.1";
	int threads = 4;
	int clients = 100;
	int requests = 100000;
	int keys = 10000;
	bool set = false;

	po::options_description desc("Allowed options");
  	desc.add_options()
    	("help,h", "Help")
    	("port,p", po::value<uint16_t>(&tcpport), "TCP port")
    	("ip,i", po::value<string>(&hostIp), "Host IP")
    	("threads,t", po::value<int>(&threads), "Number of worker threads")
    	("clients,c", po::value<int>(&clients), "Number of concurrent clients")
    	("requests,r", po::value<int>(&requests), "Number of requests per clients")
    	("keys,k", po::value<int>(&keys), "Number of keys per clients")
    	("set,s", "Get or Set")
    	;

	po::variables_map vm;
  	po::store(po::parse_command_line(argc, argv, desc), vm);
  	po::notify(vm);

	if (vm.count("help")) {
		std::cout << desc << "\n";
		return 0;
	}

	set = vm.count("set");

	InetAddress serverAddr(hostIp, tcpport);

	EventLoop loop;
	EventLoopThreadPool pool(&loop, "bench-memcache");

	int valuelen = 100;
	Client::Operation op = set ? Client::kSet : Client::kGet;

	double memoryMiB = 1.0 * clients * keys * (32 + 80 + valuelen + 8) / 1024 / 1024;
	std::cout << "estimated memcached-debug memory usage " << int(memoryMiB) << " MiB";

	pool.setThreadNum(threads);
	pool.start();

	char buf[32];
	CountDownLatch connected(clients);
  	CountDownLatch finished(clients);

	std::vector<std::unique_ptr<Client>> holder;

	for (int i = 0; i < clients; ++i) {
		snprintf(buf, sizeof(buf), "%d-", i + 1);
		holder.emplace_back(new Client(buf, pool.getNextLoop(), serverAddr, op, requests, keys, valuelen, &connected, &finished));
	}

	// 当无法连接对端，会在这阻塞
	connected.wait();

	std::cout << clients << " clients all connected" << std::endl;
	Timestamp start = Timestamp::now();

	for (int i = 0; i < clients; ++i) {
		holder[i]->send();
	}

	// 当client还连接完成阻塞
	finished.wait();
	Timestamp end = Timestamp::now();
	std::cout << "All finished" << std::endl;

	double seconds = timeDifference(end, start);
	std::cout << seconds << " sec" << std::endl;
	std::cout << 1.0 * clients * requests / seconds << " QPS" << std::endl;

	return 0;
}