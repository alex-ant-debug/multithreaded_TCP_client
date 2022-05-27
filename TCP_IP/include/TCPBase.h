#pragma once

#define SD_BOTH 0
#include <sys/socket.h>

#include <cstdint>
#include <cstring>
#include <cinttypes>
#include <malloc.h>

#include <queue>
#include <vector>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>


namespace tcp {

typedef socklen_t SockLen_t;
typedef struct sockaddr_in SocketAddr_in;
typedef int Socket;
typedef int ka_prop_t;

enum SocketStatus : uint8_t {
  connected = 0,
  err_socket_init = 1,
  err_socket_bind = 2,
  err_socket_connect = 3,
  disconnected = 4
};

typedef std::vector<uint8_t> ReceivedData;

enum SocketType : uint8_t {
  client_socket = 0,
  server_socket = 1
};

/// Simple thread pool implementation
class ThreadPool {
  std::vector<std::thread> thread_pool;  //объединение потоков
  std::queue<std::function<void()>> job_queue; //очередь заданий
  std::mutex queue_mtx;
  std::condition_variable condition;    //состояние
  std::atomic<bool> pool_terminated = false;//объединение закрыто

  void setupThreadPool(uint thread_count) { //настроить объединение потоков
    thread_pool.clear();                    //очищаем вектор потоков
    for(uint i = 0; i < thread_count; ++i) {// перебираем число потоков
        thread_pool.emplace_back(&ThreadPool::workerLoop, this); // добавляем потоков
    }
  }

  //рабочий цикл
  void workerLoop() {
    std::function<void()> job;
    while (!pool_terminated) {  // пока объединение открыто
      {
        std::unique_lock lock(queue_mtx);   //блокирует соответствующий мьютекс
        condition.wait(lock, [this](){      //Операция ожидания освобождают мьютекс и приостанавливают выполнение потока;
            return !job_queue.empty() || pool_terminated;
        });

        if(pool_terminated) {// объединение закрыто
            return;          //выходим из workerLoop
        }
        job = job_queue.front();// в job записываем первый элемент очереди заданий
        job_queue.pop();        //удаляет первый элемент в очереди заданий
      }
      job();//запускаем первый элемент из очереди заданий
    }
  }

public:
  ThreadPool(uint thread_count = std::thread::hardware_concurrency()) {//количество одновременных потоков
    setupThreadPool(thread_count);
  }

  ~ThreadPool() {
    pool_terminated = true;
    join();
  }

  template<typename F>
  void addJob(F job) {//добавить работу
    if(pool_terminated) return;
    {
      std::unique_lock lock(queue_mtx);          //блокирует соответствующий мьютекс
      job_queue.push(std::function<void()>(job));// вставляем элемент в конец очереди заданий
    }
    condition.notify_one();//уведомляем только один случайный поток из числа ожидающих на condition variable
  }

  template<typename F, typename... Arg>
  void addJob(const F& job, const Arg&... args) {
    addJob([job, args...]{job(args...);});
  }

  void join() {
    for(auto& thread : thread_pool) {//перебор вектора потокав
        thread.join();//ожидания завершения потока
    }
  }

  uint getThreadCount() const {//получить количество потоков
    return thread_pool.size();
  }

  void dropUnstartedJobs() {//удалить незапущенные задания
    pool_terminated = true;
    join();
    pool_terminated = false;
    // Clear jobs in queue
    std::queue<std::function<void()>> empty;
    std::swap(job_queue, empty);
    // reset thread pool
    setupThreadPool(thread_pool.size());
  }

  void stop() {
    pool_terminated = true;
    join();
  }

  void start(uint thread_count = std::thread::hardware_concurrency()) {
    if(!pool_terminated) return;
    pool_terminated = false;
    setupThreadPool(thread_count);
  }

};

class TcpBase {
public:
  typedef SocketStatus status;
  virtual ~TcpBase() {};
  virtual status disconnect() = 0;
  virtual status getStatus() const = 0;
  virtual bool sendData(const void* buffer, const size_t size) const = 0;
  virtual ReceivedData loadData() = 0;
  virtual uint32_t getHost() const = 0;
  virtual uint16_t getPort() const = 0;
  virtual SocketType getType() const = 0;
};

}
