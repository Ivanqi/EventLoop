#include "MemcacheServer.h"
#include "EventLoop.h"
#include "EventLoopThread.h"
#include <boost/program_options.hpp>

namespace po = boost::program_options;

bool parseCommandLine(int argc, char* argv[], MemcacheServer::Options *options)
{
  	options->tcpport = 11211;
  	options->gperfport = 11212;
  	options->threads = 4;

  	po::options_description desc("Allowed options");

	// add_options() 向 option_description 对象添加选项
  	desc.add_options()
    	("help,h", "Help")
    	("port,p", po::value<uint16_t>(&options->tcpport), "TCP port")
    	("udpport,U", po::value<uint16_t>(&options->udpport), "UDP port")
    	("gperf,g", po::value<uint16_t>(&options->gperfport), "port for gperftools")
    	("threads,t", po::value<int>(&options->threads), "Number of worker threads")
      	;

	po::variables_map vm;

	// parse_command_line() 将命令行输入的参数解析出来，store() 解析出的选项存储至variables_map中
	po::store(po::parse_command_line(argc, argv, desc), vm);

	// notify() 通知variables_map去更新所有外面的外部变量
	po::notify(vm);

	// count()检测某个选项是否被输入
	if (vm.count("help")) {
		return false;
	}

	return true;
}

int main(int argc, char* argv[]) {

	EventLoop loop;
	EventLoopThread inspectThread;
	MemcacheServer::Options options;

	if (parseCommandLine(argc, argv, &options)) {
		MemcacheServer server(&loop, options);
    	server.setThreadNum(options.threads);
    	server.start();
    	loop.loop();
	}

	return 0;
}