#include "db_manager.hpp"
#include "hasher.hpp"
#include <unordered_set>
#include <unordered_map>
#include <filesystem>
#include <vector>
#include <queue>

namespace fs = std::filesystem;

class versioneer{
private:
    Hasher hasher;
    db_manager dbm = db_manager();
    std::string backup_root;
    std::unordered_set<std::string> skip_dirs;

    void print_changes(std::unordered_map<int,int>& changes);

    void print_node(tree_node node);

    int iterator_to_id(std::string path,std::unordered_map<std::string,int>& cache);

    int fill_node(tree_node& node, std::string path, int scan_id, int parent_id, int id);

    void check_file(tree_node& node,std::unordered_map<int,int>& changes);

public:

    versioneer(std::string backup_root);

    versioneer(std::string backup_root,std::unordered_set<std::string> skip_dirs);

    void get_filesystem_changes();
};