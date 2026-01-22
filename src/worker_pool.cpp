#include "worker_pool.hpp"


void worker_pool::throttle_threads(){
    while(futures.size() >= max_threads){
        bool slot = false;
        for (auto it = futures.begin(); it != futures.end(); ) {
            if (it->wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            it->get(); // Sync / check for errors
            it = futures.erase(it); // Remove from list, returns iterator to next item
            slot=true;
            } else {
                ++it; // This thread is still busy, move to next
            }
        }

        if(!slot && futures.size() >= max_threads){
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

void worker_pool::submit(std::function<void()> task){
    throttle_threads();
    futures.push_back(std::async(std::launch::async,task));
}

void worker_pool::wait_all(){
    for(auto& f : futures) {
        if(f.valid()) f.get(); 
    }
    futures.clear(); 
}

worker_pool::~worker_pool(){
    wait_all();
}