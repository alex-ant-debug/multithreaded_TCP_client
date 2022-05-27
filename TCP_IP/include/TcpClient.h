#pragma once

#include <cstdint>
#include <cstddef>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "TCPBase.h"
#include <memory.h>

typedef int socket_t;


namespace tcp {

class TcpClient : public TcpBase {

  enum class ThreadManagementType : bool {
    single_thread = false,
    thread_pool = true
  };

  union Thread {
    std::thread* thread;
    ThreadPool* thread_pool;
    Thread() : thread(nullptr) {}
    Thread(ThreadPool* thread_pool) : thread_pool(thread_pool) {}
    ~Thread() {}
  };

  SocketAddr_in address;
  socket_t client_socket;

  std::mutex handle_mutex;
  std::function<void(ReceivedData)> handler_func = [](ReceivedData){};

  ThreadManagementType thread_management_type;
  Thread threads;

  status _status = status::disconnected;

  void handleSingleThread();//обрабатывать одиночный поток
  void handleThreadPool();

public:
  typedef std::function<void(ReceivedData)> handler_function_t;
  TcpClient() noexcept;
  TcpClient(ThreadPool* thread_pool) noexcept;
  virtual ~TcpClient() override;

  status connectTo(uint32_t host, uint16_t port) noexcept;
  virtual status disconnect() noexcept override;

  virtual uint32_t getHost() const override;
  virtual uint16_t getPort() const override;
  virtual status getStatus() const override {return _status;}

  virtual ReceivedData loadData() override;
  ReceivedData loadDataSync();
  void setHandler(handler_function_t handler);
  void joinHandler();

  virtual bool sendData(const void* buffer, const size_t size) const override;
  virtual SocketType getType() const override {return SocketType::client_socket;}
};

}
