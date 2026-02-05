
const char* db_schema = R"( CREATE TABLE IF NOT EXISTS blobs (
                            id_blob INTEGER PRIMARY KEY AUTOINCREMENT,
                            hash TEXT UNIQUE,
                            ref_count INTEGER DEFAULT 0,
                            storage_pointer TEXT
                            );
                            CREATE TABLE IF NOT EXISTS files (
                            id_file INTEGER PRIMARY KEY AUTOINCREMENT,
                            parent_id INTEGER,
                            path TEXT,
                            is_dir BOOLEAN,
                            size INTEGER,
                            mtm INTEGER,
                            scan_id INTEGER,
                            id_blob INTEGER,
                            FOREIGN KEY(id_blob) REFERENCES blobs(id_blob)
                            );
                            CREATE TABLE IF NOT EXISTS changes (
                            id_file INTEGER,
                            change_type INTEGER,
                            FOREIGN KEY(id_file) REFERENCES files(id_file)
                            );)";

const char* insert = R"(INSERT INTO files(parent_id,path,is_dir,size,mtm,scan_id) VALUES (?,?,?,?,?,?))";
const char* select_s_mtm_h = R"(SELECT size,mtm,hash FROM files f JOIN blobs b ON b.id_blob=f.id_blob WHERE id_file=?)";
const char* update_scan =R"(UPDATE files SET scan_id=? WHERE id_file=?)";
const char* update_minor = R"(UPDATE files SET mtm=?,size=?,scan_id=? WHERE id_file=?)";
const char* check_if_exists = R"(SELECT id_blob FROM blobs WHERE hash=?)";
const char* insert_blob = R"(INSERT INTO blobs(hash) VALUES (?))";
const char* subtract_b_ref=R"(UPDATE blobs SET ref_count=ref_count-1 WHERE id_blob=?)";
const char* add_b_ref = R"(UPDATE blobs SET ref_count=ref_count+1 WHERE id_blob=?)";
const char* update_f_ref = R"(UPDATE files SET id_blob=? WHERE id_file=?)";
const char* get_f_ref = R"(SELECT id_blob FROM files WHERE id_file=?)";