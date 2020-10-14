### EVENTLOOP

#### Reactor模式
- 所谓Reactor模式，是有一个循环的过程，监听对应事件是否触发，触发时调用对应的callback进行处理
- 这里的事件包括Socket可读写事件、定时器事件, signal、用户自定义事件等
- 负责事件循环的部分 为EventLoop
- 负责监听事件是否触发的部分 为 Poller
- 提供epoll和poll两种来实现，默认是epoll实现

#### TCP网络编程的本质是处理三个半事件
- 连接的建立
  - 在我们单纯使用linux的API，编写一个简单的Tcp服务器时，建立一个新的连接通常需要四步
    ```
    步骤1. socket() // 调用socket函数建立监听socket    
    步骤2. bind()   // 绑定地址和端口 
    步骤3. listen() // 开始监听端口 
    步骤4. accept() // 返回新建立连接的fd
    ```
- 连接的断开：包括主动断开和被动断开
- 消息到达，文件描述符可读
  - 在新连接建立的时候，会将新连接的socket的可读事件注册到EventLoop中
  - 假如客户端发送消息，导致已连接socket的可读事件触发，该事件对应的callback同样也会在EventLoop::loop()中被调用
  - 该事件的callback实际上就是TcpConnection::handleRead方法. 在TcpConnection::handleRead方法中，主要做了两件事
    - 从socket中读取数据，并将其放入inputbuffer中
    - 调用messageCallback，执行业务逻辑
        ```
        ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
        if (n > 0)
        {
            messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
        }
        ```
    - messageCallback是在建立新连接时，将TcpServer::messageCallback方法bind到了TcpConnection::messageCallback的方法
- 消息发送完毕。这个算半个事件

#### 为什么要移除可写事件
- 因为当OutputBuffer中没数据时，我们不需要向socket中写入数据
- 但是此时socket一直是处于可写状态的， 这将会导致TcpConnection::handleWrite()一直被触发,然而这个触发毫无意义，因为并没有什么可以写的
- 所以处理方式是，当OutputBuffer还有数据时，socket可写事件是注册状态。当OutputBuffer为空时，则将socket的可写事件移除
- 此外，highWaterMarkCallback和writeCompleteCallback一般配合使用，起到限流的作用
  

#### 连接的断开
- 被动断开即远程端断开了连接，server端需要感知到这个断开的过程，然后进行的相关的处理
- 其中感知远程断开这一步是在Tcp连接的可读事件处理函数handleRead中进行的：当对socket进行read操作时，返回值为0，则说明此时连接已断开
- 接下来会做四件事情
  - 将该TCP连接对应的事件从EventLoop移除
  - 调用用户的ConnectionCallback
  - 将对应的TcpConnection对象从Server移除
  - close对应的fd。此步骤是在析构函数中被动触发的，当TcpConnection对象被移除后，引用计数为0，对象析构时会调用close
  
#### 线程模型
- 主从reactor模式
  - 主从reactor是netty的默认模型，一个reactor对应一个EventLoop
  - 主Reactor只有一个，只负责监听新的连接，accept后将这个连接分配到子Reactor上
  - 子Reactor可以有多个。这样可以分摊一个Eventloop的压力，性能方面可能会更好
- 业务线程池
  - 对于一些阻塞型或者耗时型的任务，例如MySQL操作等。这些显然是不能放在IO线程（即EventLoop所在的线程）中运行的，因为会严重影响EventLoop的正常运行
  - 对于这类耗时型的任务，一般做法是可以放在另外单独线程池中运行，这样就不会阻塞IO线程的运行了。我们一般把这种处理耗时任务的线程叫做Worker线程
  - 推荐做法是，在OnMessage中，自行进行包的切分，然后将数据和对应的处理函数打包成Task的方式提交给线程池