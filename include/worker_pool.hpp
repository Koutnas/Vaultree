#pragma once
#include <future>
#include <vector>

class worker_pool{
    private:
        int max_threads = std::thread::hardware_concurrency();
        std::vector<std::future<void>> futures;

        void throttle_threads();

        void wait_all();
        
    public:
        void submit(std::function<void()> task);

        ~worker_pool();
    };