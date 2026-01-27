#include "versioneer.hpp"


versioneer::versioneer(std::string backup_root){
    this->backup_root = backup_root;
}

versioneer::versioneer(std::string backup_root,std::unordered_set<std::string> skip_dirs){
    this->backup_root = backup_root;
    this->skip_dirs = skip_dirs;
}

void versioneer::print_changes(std::unordered_map<int,int>& changes){
        if(changes.empty()){
            std::cout<<"No files modified"<<std::endl;
        }else{
            for (auto i : changes) {
                switch(i.second){
                    case ADDED:
                        std::cout<<"file id: "<< i.first << " - ADDED"<<std::endl;
                        break;
                    case MODIFIED:
                        std::cout<<"file id: "<< i.first << " - MODIFIED"<<std::endl;
                        break;
                    case REMOVED:
                        std::cout<<"file id: "<< i.first << " - REMOVED"<<std::endl;
                        break;
                }
            } 
        }
    }

void versioneer::print_node(tree_node node) { //used in early stages for debugging
    std::cout << "id: " << node.id << std::endl;
    std::cout << "parent id: " << node.parent_id << std::endl;
    std::cout << "path: " << node.path << std::endl;
    std::cout << "is_dir: " << node.is_dir << std::endl;
    std::cout << "size: " << node.size<< std::endl;
    std::cout << "mtm: " << node.mtm << std::endl;
    std::cout << "hash: " << node.hash << std::endl;
    std::cout << "scan_id: " << node.scan_id << std::endl;
    std::cout << "===========================" << std::endl;
}

int versioneer::iterator_to_id(std::string path,std::unordered_map<std::string,int>& cache){
    auto it = cache.find(path);
    if (it != cache.end()) {
        return it->second;
    }else{
        return -1;
    }
}

int versioneer::fill_node(tree_node& node, int scan_id, int parent_id, std::unordered_map<std::string,int>& cache, auto& entry){
        node.parent_id = parent_id;
        try {                                                                   //Exists just because sometimes especially when browsing windows appdata this throws sys error.
            node.path = entry.path().generic_string();
            if (skip_dirs.contains(entry.path().filename().string())){ 
                return 0;
            } //Also nice to have if we have some folders we dont want to back-up
        } catch (const std::system_error& e) {
            std::cerr << "Cannot read path: " << e.what() << "\n";
            return 0; //Implement feature that stores what file we skipped
        }
        node.id = iterator_to_id(node.path,cache);
        node.scan_id = scan_id;
        node.is_dir = entry.is_directory();
        if(!node.is_dir){
            try{
                node.size = entry.file_size();
            }catch(const fs::filesystem_error& e){
                std::cerr <<" Cannot read file_size: " << e.what() << "\n";
                return 0;
            }
        }
        try{
            auto ftime = entry.last_write_time();
            node.mtm = std::chrono::system_clock::to_time_t(std::chrono::file_clock::to_sys(ftime));
        }catch(const fs::filesystem_error& e){
            std::cerr << node.path <<" Cannot read mtm: " << e.what() << "\n";
            return 0;
            }
        return 1;
    }

void versioneer::build_Mtree(std::vector<tree_node>& tree){
    auto start = std::chrono::system_clock::now();
    std::sort(tree.begin(),tree.end(),
                [](const tree_node& n1,const tree_node& n2){
                    return n1.path.length()>n2.path.length();
                });
    
    std::unordered_map<int,std::string> hash_cache;
    for(tree_node& n: tree){
        if(!n.is_dir){
            hash_cache[n.parent_id] += n.hash;  //acumulate file hashes
            continue;
        }
        auto it = hash_cache.find(n.id);
        if(it != hash_cache.end()) {
            std::string hash = hasher.hash_string(it->second);
            if(n.hash != hash){
                n.hash = hash;
                dbm.update_hash(n);
            }
            hash_cache[n.parent_id] += n.hash;
        }else{
            std::string hash = hasher.hash_string(n.path); //hash empty dirs
            if(n.hash != hash){
                n.hash = hash;
                dbm.update_hash(n);
            }
            hash_cache[n.parent_id] += n.hash;
        }
    }
    auto end = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout <<"Time spent in merkle tree: "<< elapsed.count() << '\n';
}

void versioneer::hash_files(std::vector<tree_node>& added,std::vector<tree_node>& modified,std::vector<tree_node>& hashed,std::unordered_map<int,int>& changes){
    file_processor fp = file_processor(hasher,dbm);
    fp.hash_new_files(added);
    fp.hash_exist_files(modified);
    fp.pool.wait_all();
    hashed = fp.get_hashed();
    changes = fp.get_changes();
}


void versioneer::check_file(std::vector<tree_node>& added,std::vector<tree_node>& modified,std::vector<tree_node>& finished,tree_node& node){
    if(node.id == -1){
        dbm.step_insert(node); //adds new node if node wasnt found
        added.push_back(node);
    }else{
        if(dbm.compare_metadata(node) == 1){
            modified.push_back(node);
        }else{
            finished.push_back(node);
        }
    }
}

void versioneer::file_traversal(std::vector<tree_node>& added,std::vector<tree_node>& modified,std::vector<tree_node>& finished,std::unordered_map<std::string,int>& cache,uint64_t& sz,int scan_id){
    std::queue<tree_node> file_que;    
    tree_node start;
    start.path = backup_root;
    start.id = iterator_to_id(start.path,cache);
    start.scan_id = scan_id;
    start.parent_id = 0;
    start.is_dir = fs::is_directory(start.path);
    auto ftime = fs::last_write_time(start.path);
    start.mtm = std::chrono::system_clock::to_time_t(std::chrono::file_clock::to_sys(ftime));
    if(!start.is_dir){
        start.size = fs::file_size(start.path);
    }
        
    check_file(added,modified,finished,start);
    file_que.push(start);

    while (!file_que.empty()) {
        std::error_code ec;
        for (const auto& entry : fs::directory_iterator(file_que.front().path,ec)) {
            if (ec) {
                std::cerr << "Cannot access: " << ec.message() << "\n"; //handles permission exceptions file coruption etc... just be careful if you want to backup data owned by root or SYSTEM
                continue;
            }
            //BEGIN NODE FILLING LOGIC//
            std::string path;
            tree_node node;
            if(!fill_node(node,scan_id,file_que.front().id,cache,entry)){
                std::cout<<"Unable to backup file: "<<path<<std::endl;
                continue;
                };
            sz+=node.size;
            std::cout<<"//////////////////////////////////"<<std::endl;
            print_node(node);
            check_file(added,modified,finished,node);
            //END NODE FILLING LOGIC//
            print_node(node);
            if(node.is_dir){
                file_que.push(node);
                }
            }

        file_que.pop();
    }

}

void versioneer::get_filesystem_changes(){   
    //DB TRANSACTION BEGINNING
    int scan_id = dbm.get_scan_id();
    std::unordered_map<std::string,int> cache = dbm.get_index_map(scan_id);
    dbm.start_transaction();
    scan_id += 1;

    //INITIALIZATION OF BFS
    uint64_t sz = 0;
    std::vector<tree_node> finished;
    std::vector<tree_node> added;
    std::vector<tree_node> modified;
    std::vector<tree_node> hashed;
    std::unordered_map<int,int> changes;

    file_traversal(added,modified,finished,cache,sz,scan_id);
    std::cout<<"Finished traversing file system, proceeding to start hasing..."<<std::endl;
    
    hash_files(added,modified,hashed,changes);
    finished.insert(finished.end(),hashed.begin(),hashed.end()); //Merging two vectors after we are done with them
    dbm.get_removed(changes,scan_id);

    std::cout<<"Finished hashing, proceeding to start building merkle tree..."<<std::endl;
    build_Mtree(finished);
   
    std::cout<<"Finished building a merkle tree..."<<std::endl;
    /*
    for (int i = 0; i < finished.size(); i++) {
        print_node(finished[i]);
    }
    */


    print_changes(changes);
    std::cout<<"Tree size: "<< sz <<"B" << std::endl;
    std::cout<<"Root hash: "<< finished.back().hash<< std::endl;
    dbm.clean_blobs();


    dbm.commit_transaction();
    }
