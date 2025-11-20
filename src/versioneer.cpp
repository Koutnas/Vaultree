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
            return 0;
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
    std::reverse(tree.begin(),tree.end()); //reversing search path made by BFS
    std::unordered_map<int,std::string> hash_cache;
    for(tree_node& n: tree){
        if(!n.is_dir){
            hash_cache[n.parent_id] += n.hash;  //acumulate file hashes
            continue;
        }
        auto it = hash_cache.find(n.id);
        if(it != hash_cache.end()) {
            std::string hash = hasher.hash_string(it->second);  //hash non empty dirs
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
}

void versioneer::check_file(tree_node& node,std::unordered_map<int,int>& changes){
    if(node.id == -1){
        if(!node.is_dir){
            node.hash = hasher.hash_file(node.path);
        }
        dbm.step_insert(node); //adds new node if node wasnt found
        changes.insert({node.id,ADDED});
    }else{
        if(dbm.compare_metadata(node) == 1){
        changes.insert({node.id,MODIFIED});
        };
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
        std::queue<tree_node> file_que;
        std::vector<tree_node> file_tree;
        std::unordered_map<int,int> changes;

        //INTIALIZATION OF ROOT NODE
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
        
        check_file(start,changes);
            
        file_tree.push_back(start);
        file_que.push(start);

        while (!file_que.empty()) {
            std::error_code ec;
            for (const auto& entry : fs::directory_iterator(file_que.front().path,ec)) {
                if (ec) {
                    std::cerr << "Cannot access /some/path: " << ec.message() << "\n"; //handles permission exceptions file coruption etc... just be careful if you want to backup data owned by root or SYSTEM
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
                //END NODE FILLING LOGIC//
                check_file(node,changes);
                if(node.is_dir){
                    file_que.push(node);
                }
                file_tree.push_back(node);
                }

            file_que.pop();
        }
        dbm.get_removed(changes,scan_id);

        build_Mtree(file_tree);
        //for (int i = 0; i < file_tree.size(); i++) {
        //    print_node(file_tree[i]);
        //}

        //print_changes(changes);
        //std::cout<<"Tree size: "<< sz <<"B" << std::endl;
        //std::cout<<"Root hash: "<< file_tree.back().hash<< std::endl;

        dbm.commit_transaction();
    }
