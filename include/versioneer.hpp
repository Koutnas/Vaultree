#include "file_processor.hpp"
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <future>
#include <queue>

namespace fs = std::filesystem;

class versioneer{
private:
    Hasher hasher;
    db_manager dbm = db_manager(SCAN);
    std::string backup_root;
    std::unordered_set<std::string> skip_dirs;

    void print_changes(std::unordered_map<int,int>& changes);

    void print_node(tree_node node);

    int iterator_to_id(std::string path,std::unordered_map<std::string,int>& cache);

    int fill_node(tree_node& node, int scan_id, int parent_id,std::unordered_map<std::string,int>& cache, auto& entry);

    void check_file(std::vector<tree_node>& added,std::vector<tree_node>& modified,std::vector<tree_node>& finished,tree_node& node);

    void hash_files(std::vector<tree_node>& added,std::vector<tree_node>& modified,std::vector<tree_node>& hashed,std::unordered_map<int,int>& changes);

    void file_traversal(std::vector<tree_node>& added,std::vector<tree_node>& modified,std::vector<tree_node>& finished,std::unordered_map<std::string,int>& cache,uint64_t& sz,int scan_id);

    void build_Mtree(std::vector<tree_node>& tree);

public:

    versioneer(std::string backup_root);

    versioneer(std::string backup_root,std::unordered_set<std::string> skip_dirs);

    void get_filesystem_changes();
};