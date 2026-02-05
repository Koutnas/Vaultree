#pragma once

#include <iostream>
#include <sqlite3.h>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>


enum status{
    ADDED = 1,
    MODIFIED,
    REMOVED
};

enum mode{ //Decides which ptsm will be prepeared
    SCAN = 1,
    DIR,
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
    int mode;

    struct prepared_scan {
        sqlite3_stmt* insertf = nullptr;
        sqlite3_stmt* select = nullptr;
        sqlite3_stmt* update_scan = nullptr;
        sqlite3_stmt* update_minor = nullptr;
        sqlite3_stmt* add_blob_ref = nullptr;
        sqlite3_stmt* subtract_blob_ref = nullptr;
        sqlite3_stmt* check_exists = nullptr;
        sqlite3_stmt* insert_blob = nullptr;
        sqlite3_stmt* update_file_ref = nullptr;
        sqlite3_stmt* get_file_ref = nullptr;
    } stmts_scan;

    struct prepared_dir {

    } stmts_dir;


    
    int check_hash_exists(std::string hash);

    void update_hash_ref(int id_blob,int id_file);

    int get_blob_id(int id_blob);

    void insert_change(int id,int change_type);

    void delete_changes();

public:

    db_manager(int mode);

    ~db_manager();

    std::unordered_map<std::string,int> get_index_map(int scan_id);

    void get_object_hashes(std::unordered_set<std::string>& objects);

    void get_redundant_hashes(std::unordered_set<std::string>& objects);

    void start_transaction();

    void commit_transaction();

    void get_removed(std::unordered_map<int,int>& changes,int scan_id);

    void update_hash(tree_node& node);

    void update_scan_id(tree_node& node);

    void update_mtm_size(tree_node& node);

    void step_insert(tree_node& node);

    void clean_blobs();

    void write_changes(std::unordered_map<int,int>& changes);

    int compare_metadata(tree_node& node);  //will be made obsolete

    int check_mtm_size(tree_node& node);

    int get_scan_id();

    

};