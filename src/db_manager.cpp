#include "db_manager.hpp"
#include "db_schemas.hpp"

db_manager::db_manager(int mode){
    this->mode = mode;
    if(!std::filesystem::exists(".vaultree")){
        std::filesystem::create_directory(".vaultree");
    }
    if (sqlite3_open(".vaultree/metadata.db", &db)) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        return;}
    const char* schema = db_schema;
    char* err;
    if (sqlite3_exec(db, schema, 0, 0, &err) != SQLITE_OK) {
        std::cerr << "SQL error: " << err << std::endl;
        sqlite3_free(err);
    }
    switch (mode){
    case SCAN:
        //PREPEARED STATEMENTS USED FOR FILE ENUMERATION,HASHING AND BUILDING OF MERKLE TREE
        sqlite3_prepare_v2(db, insert, -1, &stmts_scan.insertf, nullptr);
        sqlite3_prepare_v2(db, select_s_mtm_h, -1, &stmts_scan.select, nullptr);
        sqlite3_prepare_v2(db, update_scan, -1, &stmts_scan.update_scan, nullptr);
        sqlite3_prepare_v2(db, update_minor, -1, &stmts_scan.update_minor,nullptr);
        sqlite3_prepare_v2(db, add_b_ref, -1, &stmts_scan.add_blob_ref,nullptr);
        sqlite3_prepare_v2(db, subtract_b_ref, -1, &stmts_scan.subtract_blob_ref,nullptr);
        sqlite3_prepare_v2(db, check_if_exists, -1, &stmts_scan.check_exists,nullptr);
        sqlite3_prepare_v2(db, insert_blob, -1, &stmts_scan.insert_blob,nullptr);
        sqlite3_prepare_v2(db, update_f_ref, -1, &stmts_scan.update_file_ref,nullptr);
        sqlite3_prepare_v2(db, get_f_ref, -1, &stmts_scan.get_file_ref,nullptr);
        break;
    case BACKUP:

        break;
    default:
        break;
    }
}
db_manager::~db_manager(){
    switch (mode)
    {
    case SCAN:
        sqlite3_finalize(stmts_scan.update_scan);
        sqlite3_finalize(stmts_scan.select);
        sqlite3_finalize(stmts_scan.insertf);
        sqlite3_finalize(stmts_scan.update_minor);
        sqlite3_finalize(stmts_scan.add_blob_ref);
        sqlite3_finalize(stmts_scan.subtract_blob_ref);
        sqlite3_finalize(stmts_scan.check_exists);
        sqlite3_finalize(stmts_scan.insert_blob);
        sqlite3_finalize(stmts_scan.update_file_ref);
        sqlite3_finalize(stmts_scan.get_file_ref);
        break;
    case BACKUP:

        break;
    default:
        break;
    }

    if(db){
        sqlite3_close(db);
        db = nullptr;
    }
}

std::unordered_map<std::string,int> db_manager::get_index_map(int scan_id){
    std::unordered_map<std::string,int> cache;
    sqlite3_stmt* stmt;
        
    const char* sql = "SELECT id_file, path FROM files WHERE scan_id=?;";

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
    sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr);
}

void db_manager::get_removed(std::unordered_map<int,int>& changes,int scan_id){
    sqlite3_stmt* stmt;
    const char* sql= R"(SELECT id_file,id_blob FROM files WHERE scan_id=?;)";
    sqlite3_prepare_v2(db,sql,-1,&stmt,nullptr);
    sqlite3_bind_int(stmt,1,scan_id-1);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt,0);
        int id_blob = sqlite3_column_int(stmt,1);
        sqlite3_bind_int(stmts_scan.subtract_blob_ref,1,id_blob); //Removing valid reference from non existent file.
        sqlite3_step(stmts_scan.subtract_blob_ref);
        sqlite3_reset(stmts_scan.subtract_blob_ref);
        changes.insert({id,REMOVED});
    }
    sqlite3_finalize(stmt);
}

void db_manager::step_insert(tree_node& node){
    sqlite3_bind_int(stmts_scan.insertf, 1, node.parent_id);
    sqlite3_bind_text(stmts_scan.insertf, 2, node.path.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmts_scan.insertf, 3, node.is_dir);
    sqlite3_bind_int(stmts_scan.insertf, 4, node.size);
    sqlite3_bind_int64(stmts_scan.insertf, 5, node.mtm);
    sqlite3_bind_int(stmts_scan.insertf,6,node.scan_id);
                    
    sqlite3_step(stmts_scan.insertf);
    node.id = static_cast<int>(sqlite3_last_insert_rowid(db));
    sqlite3_reset(stmts_scan.insertf);
}
void db_manager::update_hash(tree_node& node){
    int new_blob_id = check_hash_exists(node.hash); //Blob id of the newly calculated hash
    int old_blob_id = get_blob_id(node.id);
    
    if(new_blob_id > 0){ //Hash found in blob database
        if(old_blob_id != -1){ //If blob has existing reference we need to substract it
            sqlite3_bind_int(stmts_scan.subtract_blob_ref,1,old_blob_id);
            sqlite3_step(stmts_scan.subtract_blob_ref);
            sqlite3_reset(stmts_scan.subtract_blob_ref);
        }
        update_hash_ref(node.id,new_blob_id);
    }else{//Create new blob using the new hash
        if(old_blob_id != -1){ //If blob has existing reference we need to substract it
            sqlite3_bind_int(stmts_scan.subtract_blob_ref,1,old_blob_id);
            sqlite3_step(stmts_scan.subtract_blob_ref);
            sqlite3_reset(stmts_scan.subtract_blob_ref);
        }//Creation of new blob
        sqlite3_bind_text(stmts_scan.insert_blob, 1, node.hash.c_str(), -1, SQLITE_STATIC);
        sqlite3_step(stmts_scan.insert_blob);
        new_blob_id = static_cast<int>(sqlite3_last_insert_rowid(db));
        sqlite3_reset(stmts_scan.insert_blob);
        update_hash_ref(node.id,new_blob_id);
    }

}

void db_manager::update_scan_id(tree_node& node){
    sqlite3_bind_int(stmts_scan.update_scan,1,node.scan_id);
    sqlite3_bind_int(stmts_scan.update_scan,2,node.id);

    sqlite3_step(stmts_scan.update_scan);
    sqlite3_reset(stmts_scan.update_scan);
}

void db_manager::update_mtm_size(tree_node& node){
    sqlite3_bind_int64(stmts_scan.update_minor,1,node.mtm);
    sqlite3_bind_int(stmts_scan.update_minor,2,node.size);
    sqlite3_bind_int(stmts_scan.update_minor,3,node.scan_id);
    sqlite3_bind_int(stmts_scan.update_minor,4,node.id);

    sqlite3_step(stmts_scan.update_minor);
    sqlite3_reset(stmts_scan.update_minor);
}

int db_manager::compare_metadata(tree_node& node){
    sqlite3_reset(stmts_scan.select);
    sqlite3_bind_int(stmts_scan.select,1,node.id);
    if(sqlite3_step(stmts_scan.select) == SQLITE_ROW){
        int size = sqlite3_column_int(stmts_scan.select,0);
        time_t mtm = sqlite3_column_int64(stmts_scan.select,1);
        std::string hash = reinterpret_cast<const char*>(sqlite3_column_text(stmts_scan.select, 2));
        if(node.mtm == mtm && node.size == size){ //File unmodified
            node.hash = hash;
            update_scan_id(node);
            return 0;
        }else{
            node.hash = hash;
            update_mtm_size(node);
            return 1;
        }
    }
    return -1;
}
int db_manager::check_mtm_size(tree_node& node){
    sqlite3_reset(stmts_scan.select);
    sqlite3_bind_int(stmts_scan.select,1,node.id);

    if(sqlite3_step(stmts_scan.select) == SQLITE_ROW){
        int size = sqlite3_column_int(stmts_scan.select,0);
        time_t mtm = sqlite3_column_int64(stmts_scan.select,1);
        std::string hash = reinterpret_cast<const char*>(sqlite3_column_text(stmts_scan.select, 2));
        if(!node.is_dir){
            node.hash = hash;
        }

        if(node.mtm == mtm && node.size == size){
            return 0;
        }else{
            return 1;
        }
    }
    return -1; //Fix later
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

int db_manager::check_hash_exists(std::string hash){
    sqlite3_reset(stmts_scan.check_exists);
    sqlite3_bind_text(stmts_scan.check_exists, 1, hash.c_str(), -1, SQLITE_STATIC);

    if(sqlite3_step(stmts_scan.check_exists) == SQLITE_ROW){
        int id = sqlite3_column_int(stmts_scan.check_exists,0);
        return id;
    }else{
        return -1;
    }
}

void db_manager::update_hash_ref(int id_file,int id_blob){
    sqlite3_bind_int(stmts_scan.add_blob_ref,1,id_blob);
    sqlite3_bind_int(stmts_scan.update_file_ref,1,id_blob);
    sqlite3_bind_int(stmts_scan.update_file_ref,2,id_file);
    sqlite3_step(stmts_scan.update_file_ref);
    sqlite3_step(stmts_scan.add_blob_ref);
    sqlite3_reset(stmts_scan.update_file_ref);
    sqlite3_reset(stmts_scan.add_blob_ref);
}

int db_manager::get_blob_id(int id_file){
    sqlite3_reset(stmts_scan.get_file_ref);
    sqlite3_bind_int(stmts_scan.get_file_ref, 1, id_file);

    if(sqlite3_step(stmts_scan.get_file_ref) == SQLITE_ROW){
        if(sqlite3_column_type(stmts_scan.get_file_ref,0) == SQLITE_NULL){
            return -1;
        }else{
            int id = sqlite3_column_int(stmts_scan.get_file_ref,0);
            return id;
        }
    }else{
        return -1;
    }
}

void db_manager::clean_blobs(){
    sqlite3_stmt* stmt;
    const char* sql= R"(DELETE FROM blobs WHERE ref_count<1;)";
    sqlite3_prepare_v2(db,sql,-1,&stmt,nullptr);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

void db_manager::write_changes(std::unordered_map<int,int>& changes){
    delete_changes();
    sqlite3_stmt* stmt;
    const char* sql = R"(INSERT INTO changes(id_file,change_type) VALUES (?,?))";
    sqlite3_prepare_v2(db,sql,-1,&stmt,nullptr);

    for(auto change : changes){
        sqlite3_bind_int(stmt,1,change.first);
        sqlite3_bind_int(stmt,2,change.second);
        sqlite3_step(stmt);
        sqlite3_reset(stmt);
    }

    sqlite3_finalize(stmt);
}

void db_manager::delete_changes(){
    sqlite3_stmt* stmt;
    const char* sql = R"(DELETE FROM changes)";
    sqlite3_prepare_v2(db,sql,-1,&stmt,nullptr);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

void db_manager::get_object_hashes(std::unordered_set<std::string>& objects){
    sqlite3_stmt* stmt;
    const char* sql=R"(SELECT DISTINCT hash FROM changes c 
                    JOIN files f ON f.id_file=c.id_file JOIN blobs b ON b.id_blob=f.id_blob
                    WHERE is_dir=0 AND (change_type=1 OR change_type=2);)";
    sqlite3_prepare_v2(db,sql,-1,&stmt,nullptr);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        objects.insert(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
    }
    sqlite3_finalize(stmt);
}

void db_manager::get_redundant_hashes(std::unordered_set<std::string>& objects){
    sqlite3_stmt* stmt;
    const char* sql=R"(SELECT hash FROM blobs b JOIN files f ON f.id_blob=b.id_blob WHERE ref_count<1 AND is_dir=0;)";
    sqlite3_prepare_v2(db,sql,-1,&stmt,nullptr);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        objects.insert(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
    }
    sqlite3_finalize(stmt);
}

/*
MUST FIX LIST:
*/
