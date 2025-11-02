#include "db_manager.hpp"

db_manager::db_manager(){
    if (sqlite3_open(".metadata.db", &db)) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        return;}
    const char* schema = R"(CREATE TABLE IF NOT EXISTS files (
                            id INTEGER PRIMARY KEY AUTOINCREMENT,
                            parent_id INTEGER,
                            path TEXT UNIQUE,
                            is_dir BOOLEAN,
                            size INTEGER,
                            mtm INTEGER,
                            hash TEXT,
                            scan_id INTEGER
                            );)";
    char* err;
    if (sqlite3_exec(db, schema, 0, 0, &err) != SQLITE_OK) {
        std::cerr << "SQL error: " << err << std::endl;
        sqlite3_free(err);
    }
    const char* insertf =R"(INSERT INTO files (parent_id,path,is_dir,size,mtm,hash,scan_id) VALUES (?,?,?,?,?,?,?))";
    sqlite3_prepare_v2(db, insertf, -1, &stmts.insertf, nullptr);
    const char* select =R"(SELECT size,mtm,hash FROM files WHERE id=?)";
    sqlite3_prepare_v2(db, select, -1, &stmts.select, nullptr);
    const char* update =R"(UPDATE files SET size=?,mtm=?,hash=?,scan_id=? WHERE id=?)";
    sqlite3_prepare_v2(db, update, -1, &stmts.update, nullptr);
    const char* update_s =R"(UPDATE files SET scan_id=? WHERE id=?)";
    sqlite3_prepare_v2(db, update_s, -1, &stmts.update_s, nullptr);
}
db_manager::~db_manager(){
    sqlite3_finalize(stmts.update_s);
    sqlite3_finalize(stmts.update);
    sqlite3_finalize(stmts.select);
    sqlite3_finalize(stmts.insertf);

    if(db){
        sqlite3_close(db);
        db = nullptr;
    }
}

std::unordered_map<std::string,int> db_manager::get_index_map(int scan_id){
    std::unordered_map<std::string,int> cache;
    sqlite3_stmt* stmt;
        
    const char* sql = "SELECT id, path FROM files WHERE scan_id=?;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return cache;
        }
    sqlite3_bind_int(stmt,1,scan_id);
        
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        std::string path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        cache.insert({path,id});
    }
    sqlite3_finalize(stmt);

    return cache;
}

void db_manager::start_transaction(){
    sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);
}

void db_manager::commit_transaction(){
    sqlite3_finalize(stmts.update_s);
    sqlite3_finalize(stmts.update);
    sqlite3_finalize(stmts.select);
    sqlite3_finalize(stmts.insertf);
    sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr);
}

void db_manager::get_removed(std::unordered_map<int,int>& changes,int scan_id){
    sqlite3_stmt* stmt;
    const char* sql= R"(SELECT id FROM files WHERE scan_id=?;)";
    sqlite3_prepare_v2(db,sql,-1,&stmt,nullptr);
    sqlite3_bind_int(stmt,1,scan_id-1);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt,0);
        changes.insert({id,REMOVED});
    }
    sqlite3_finalize(stmt);
}

void db_manager::update_scan_id(tree_node& node){
    sqlite3_bind_int(stmts.update_s,1,node.scan_id);
    sqlite3_bind_int(stmts.update_s,2,node.id);

    sqlite3_step(stmts.update_s);
    sqlite3_reset(stmts.update_s);
}

void db_manager::step_insert(tree_node& node){
    sqlite3_bind_int(stmts.insertf, 1, node.parent_id);
    sqlite3_bind_text(stmts.insertf, 2, node.path.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmts.insertf, 3, node.is_dir);
    sqlite3_bind_int(stmts.insertf, 4, node.size);
    sqlite3_bind_int64(stmts.insertf, 5, node.mtm);
    sqlite3_bind_text(stmts.insertf, 6, node.hash.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmts.insertf,7,node.scan_id);
                    
    sqlite3_step(stmts.insertf);
    node.id = static_cast<int>(sqlite3_last_insert_rowid(db));
    sqlite3_reset(stmts.insertf);
}

void db_manager::update_metadata(tree_node& node){
    sqlite3_bind_int(stmts.update,1,node.size);
    sqlite3_bind_int64(stmts.update,2,node.mtm);
    sqlite3_bind_text(stmts.update,3,node.hash.c_str(),-1,SQLITE_STATIC);
    sqlite3_bind_int(stmts.update,4,node.scan_id);
    sqlite3_bind_int(stmts.update,5,node.id);

    sqlite3_step(stmts.update);
    sqlite3_reset(stmts.update);
}

int db_manager::compare_metadata(tree_node& node){
    sqlite3_reset(stmts.select);
    sqlite3_bind_int(stmts.select,1,node.id);

    if(sqlite3_step(stmts.select) == SQLITE_ROW){
        int size = sqlite3_column_int(stmts.select,0);
        time_t mtm = sqlite3_column_int64(stmts.select,1);
        std::string hash = reinterpret_cast<const char*>(sqlite3_column_text(stmts.select, 2));

        if(node.mtm == mtm && node.size == size){ //File unmodified
            update_scan_id(node);
            return 0;
        }else{
            update_metadata(node);
            return 1;
        }
    }
    return -1;
}

int db_manager::get_scan_id(){
    sqlite3_stmt* stmt;
    const char* sql= R"(SELECT MAX(scan_id) FROM files;)";
    if(sqlite3_prepare_v2(db,sql,-1,&stmt,nullptr) != SQLITE_OK){
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
    }

    if(sqlite3_step(stmt) == SQLITE_ROW){
        int scan_id = sqlite3_column_int(stmt,0);
        sqlite3_finalize(stmt);
        return scan_id; 
    }else{
        sqlite3_finalize(stmt);
        return -1;
        }
    }
