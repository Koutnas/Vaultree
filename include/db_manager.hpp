#include "hasher.hpp"
#include <iostream>
#include <sqlite3.h>
#include <unordered_map>


enum status{
    ADDED = 0,
    MODIFIED,
    REMOVED
};

struct tree_node {
        int id = -1;
        int parent_id = -1;
        int scan_id = 0;
        int size = 0;
        bool is_dir;        
        time_t mtm;
        std::string path;
        std::string hash = "";
};

class db_manager{
private:
    sqlite3* db;
    Hasher hasher;
    
public:
    struct Prepared {
    sqlite3_stmt* insertf = nullptr;
    sqlite3_stmt* select = nullptr;
    sqlite3_stmt* update = nullptr;
    sqlite3_stmt* update_scan = nullptr;
    sqlite3_stmt* update_minor = nullptr;
    sqlite3_stmt* update_hash = nullptr;
    } stmts;

    db_manager();

    ~db_manager();

    std::unordered_map<std::string,int> get_index_map(int scan_id);

    void start_transaction();

    void commit_transaction();

    void get_removed(std::unordered_map<int,int>& changes,int scan_id);

    void update_hash(tree_node& node);

    void update_scan_id(tree_node& node);

    void update_mtm_size(tree_node& node);

    void update_all(tree_node& node);

    void step_insert(tree_node& node);

    int compare_metadata(tree_node& node);  //will be made obsolete

    int check_mtm_size(tree_node& node);

    int get_scan_id();
};