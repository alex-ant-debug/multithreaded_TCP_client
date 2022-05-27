#include "../include/TcpClient.h"
#include <stdio.h>
#include <cstring>
#include <iostream>

using namespace tcp;


void TcpClient::handleSingleThread() {
  try {
    while(_status == status::connected) {
      if(ReceivedData data = loadData(); !data.empty()) {
        std::lock_guard lock(handle_mutex);
        handler_func(std::move(data));
      } else if (_status != status::connected) return;
    }
  }  catch (std::exception& except) {
    std::cerr << except.what() << std::endl;
    return;
  }
}

void TcpClient::handleThreadPool() {
  try {
    if(ReceivedData data = loadData(); !data.empty()) {
      std::lock_guard lock(handle_mutex);
      handler_func(std::move(data));
    }
    if(_status == status::connected) {
        threads.thread_pool->addJob([this]{handleThreadPool();});
    }
  } catch (std::exception& except) {
    std::cerr << except.what() << std::endl;
    return;
  } catch (...) {
    std::cerr << "Unhandled exception!" << std::endl;
    return;
  }
}

TcpClient::TcpClient() noexcept : _status(status::disconnected) {}
TcpClient::TcpClient(ThreadPool* thread_pool) noexcept :
  thread_management_type(ThreadManagementType::thread_pool),
  threads(thread_pool),
  _status(status::disconnected) {}

TcpClient::~TcpClient() {
  disconnect();

      switch (thread_management_type) {
    case tcp::TcpClient::ThreadManagementType::single_thread:
      if(threads.thread) threads.thread->join();
      delete threads.thread;
    break;
    case tcp::TcpClient::ThreadManagementType::thread_pool: break;
  }
}

TcpClient::status TcpClient::connectTo(uint32_t host, uint16_t port) noexcept {

  if((client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP)) < 0) return _status = status::err_socket_init;

  new(&address) SocketAddr_in;
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = host;

  address.sin_addr.s_addr = host;

  address.sin_port = htons(port);

  if(connect(client_socket, (sockaddr *)&address, sizeof(address)) != 0) {
    close(client_socket);
		return _status = status::err_socket_connect;
	}
	return _status = status::connected;
}

TcpClient::status TcpClient::disconnect() noexcept {
	if(_status != status::connected)
		return _status;
  _status = status::disconnected;
  switch (thread_management_type) {
    case tcp::TcpClient::ThreadManagementType::single_thread:
      if(threads.thread) threads.thread->join();
      delete threads.thread;
    break;
    case tcp::TcpClient::ThreadManagementType::thread_pool: break;
  }
  shutdown(client_socket, SD_BOTH);
  close(client_socket);
  return _status;
}

ReceivedData TcpClient::loadData() {
    ReceivedData buffer;
    uint32_t size;
    int err;

    int answ = recv(client_socket, (char*)&size, sizeof (size), MSG_DONTWAIT);

    // Disconnect
    if(!answ) {
      disconnect();
      return ReceivedData();
    } else if(answ == -1) {
      // Error handle (f*ckin OS-dependence!)
        SockLen_t len = sizeof (err);
        getsockopt (client_socket, SOL_SOCKET, SO_ERROR, (char*)&err, &len);
        if(!err) err = errno;

      switch (err) {
        case 0: break;
          // Keep alive timeout
        case ETIMEDOUT:
        case ECONNRESET:
        case EPIPE:
          disconnect();
          [[fallthrough]];
          // No data
        case EAGAIN: return ReceivedData();
        default:
          disconnect();
        return ReceivedData();
      }
    }

    if(!size) return ReceivedData();
    buffer.resize(size);
    recv(client_socket, (char*)buffer.data(), buffer.size(), 0);
    return buffer;
}

ReceivedData TcpClient::loadDataSync() {
  ReceivedData data;
  uint32_t size = 0;
  int answ = recv(client_socket, reinterpret_cast<char*>(&size), sizeof(size), 0);
  if(size && answ == sizeof (size)) {
    data.resize(size);
    recv(client_socket, reinterpret_cast<char*>(data.data()), data.size(), 0);
  }
  return data;
}

void TcpClient::setHandler(TcpClient::handler_function_t handler) {

  {
    std::lock_guard lock(handle_mutex);
    handler_func = handler;
  }

  switch (thread_management_type) {
    case tcp::TcpClient::ThreadManagementType::single_thread:
      if(threads.thread) return;
      threads.thread = new std::thread(&TcpClient::handleSingleThread, this);
    break;
    case tcp::TcpClient::ThreadManagementType::thread_pool:
      threads.thread_pool->addJob([this]{handleThreadPool();});
    break;
  }
}

void TcpClient::joinHandler() {
  switch (thread_management_type) {
    case tcp::TcpClient::ThreadManagementType::single_thread:
      if(threads.thread) threads.thread->join();
    break;
    case tcp::TcpClient::ThreadManagementType::thread_pool:
      threads.thread_pool->join();
    break;
  }
}

bool TcpClient::sendData(const void* buffer, const size_t size) const {
  void* send_buffer = malloc(size + sizeof (int));
  memcpy(reinterpret_cast<char*>(send_buffer) + sizeof(int), buffer, size);
  *reinterpret_cast<int*>(send_buffer) = size;
  if(send(client_socket, reinterpret_cast<char*>(send_buffer), size + sizeof(int), 0) < 0) return false;
  free(send_buffer);
	return true;
}

uint32_t TcpClient::getHost() const {
    return address.sin_addr.s_addr;
}

uint16_t TcpClient::getPort() const {
    return address.sin_port;
}
