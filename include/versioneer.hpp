#include "db_manager.hpp"
#include "hasher.hpp"
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <future>
#include <filesystem>
#include <queue>

namespace fs = std::filesystem;

class versioneer{
private:
    Hasher hasher;
    db_manager dbm = db_manager();
    std::mutex db_mutex; // The lock for your changes map/database
    std::vector<std::future<void>> futures; // Keeps track of running tasks
    int max_threads = std::thread::hardware_concurrency();
    std::string backup_root;
    std::unordered_set<std::string> skip_dirs;

    void print_changes(std::unordered_map<int,int>& changes);

    void print_node(tree_node node);

    int iterator_to_id(std::string path,std::unordered_map<std::string,int>& cache);

    int fill_node(tree_node& node, int scan_id, int parent_id,std::unordered_map<std::string,int>& cache, auto& entry);

    void check_file(std::vector<tree_node>& added,std::vector<tree_node>& modified,std::vector<tree_node>& finished,tree_node& node);

    void mt_existing_file(tree_node node, std::unordered_map<int,int>& changes,std::vector<tree_node>& hashed,std::mutex& mutex);

    void mt_new_file(tree_node node, std::unordered_map<int,int>& changes,std::vector<tree_node>& hashed, std::mutex& mutex);

    void hash_files(std::vector<tree_node>& added,std::vector<tree_node>& modified,std::vector<tree_node>& hashed,std::unordered_map<int,int>& changes);

    void manage_threads();

    void file_traversal(std::vector<tree_node>& added,std::vector<tree_node>& modified,std::vector<tree_node>& finished,std::unordered_map<std::string,int>& cache,uint64_t& sz,int scan_id);

    void build_Mtree(std::vector<tree_node>& tree);

public:

    versioneer(std::string backup_root);

    versioneer(std::string backup_root,std::unordered_set<std::string> skip_dirs);

    void get_filesystem_changes();
};