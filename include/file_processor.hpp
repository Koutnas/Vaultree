#include "db_manager.hpp"
#include "worker_pool.hpp"
#include "hasher.hpp"
#include <future>

class file_processor{

    private:
        //Dependencies
        Hasher& hasher;
        db_manager& dbm;

        //Class owned
        std::mutex db_mutex;
        std::vector<tree_node> hashed;
        std::unordered_map<int,int> changes;
        worker_pool pool;

        void process_existing(tree_node node);

        void process_new(tree_node node);

    public:
        file_processor(Hasher& hasher,db_manager& dbm);

        void hash_new_files(std::vector<tree_node>& added);

        void hash_exist_files(std::vector<tree_node>& modified);

        std::vector<tree_node>& get_hashed();
        std::unordered_map<int,int>& get_changes(); 
    };
