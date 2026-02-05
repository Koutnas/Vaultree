#pragma once
#include <future>
#include <vector>

class worker_pool{
    private:
        int max_threads = std::thread::hardware_concurrency();
        std::vector<std::future<void>> futures;

        void throttle_threads();
        
    public:
        void submit(std::function<void()> task);

        void wait_all();

        ~worker_pool();
    };