#ifndef BUFFERED_CHANNEL_H_
#define BUFFERED_CHANNEL_H_

#include <queue>
#include <mutex>
#include <condition_variable>

template<class T>
class BufferedChannel {
public:
    explicit BufferedChannel(int size) : capacity(size) {
        if (capacity <= 0) {
            throw std::invalid_argument("BufferedChannel capacity must be positive");
        }
    }

    void Send(T value) {
        std::unique_lock<std::mutex> lock(mutex);
        not_full.wait(lock, [this] {
            return closed || static_cast<int>(queue.size()) < capacity; // защита от "ложных пробуждений"
            }
        );
        if (closed) {
            throw std::runtime_error("Send on closed channel");
        }
        queue.push(std::move(value));
        not_empty.notify_one();
    }

    std::pair<T, bool> Recv() {
        std::unique_lock<std::mutex> lock(mutex);
        not_empty.wait(lock, [this] {
            return !queue.empty() || closed;
            }
        );
        if (queue.empty()) {
            return { T{}, false };
        }
        T value = std::move(queue.front());
        queue.pop();
        not_full.notify_one();
        return { std::move(value), true };
    }

    void Close() {
        std::lock_guard<std::mutex> lock(mutex);
        if (!closed) {
            closed = true;
            not_empty.notify_all();
            not_full.notify_all();
        }
    }

private:
    std::queue<T> queue;
    int capacity;
    bool closed = false;

    std::mutex mutex;
    std::condition_variable not_empty;
    std::condition_variable not_full;
};

#endif // BUFFERED_CHANNEL_H_
