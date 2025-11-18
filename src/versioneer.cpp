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

int versioneer::fill_node(tree_node& node, std::string path, int scan_id, int parent_id, int id){
        node.id = id;
        node.parent_id = parent_id;
        node.path = path;
        node.scan_id = scan_id;
        node.is_dir = fs::is_directory(node.path);
        if(!node.is_dir){
            try{
            node.hash = hasher.compute_hash(node.path);
            node.size = fs::file_size(node.path);
            }catch(const fs::filesystem_error& e){
                std::cerr <<" Cannot read file_size: " << e.what() << "\n";
                return 0;
            }
        }
        try{
            auto ftime = fs::last_write_time(node.path);
            node.mtm = std::chrono::system_clock::to_time_t(std::chrono::file_clock::to_sys(ftime));
        }catch(const fs::filesystem_error& e){
            std::cerr << node.path <<" Cannot read mtm: " << e.what() << "\n";
            return 0;
            }
        return 1;
    }

void versioneer::check_file(tree_node& node,std::unordered_map<int,int>& changes){
    if(node.id == -1){
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
        fill_node(start,backup_root,scan_id,0,iterator_to_id(backup_root,cache));
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
                try {                                                                   //Exists just because sometimes especially when browsing windows appdata this throws sys error.
                    path = entry.path().generic_string();
                    if (skip_dirs.contains(entry.path().filename().string())) continue; //Also nice to have if we have some folders we dont want to back-up
                }
                catch (const std::system_error& e) {
                    std::cerr << "Cannot read path: " << e.what() << "\n";
                    continue;
                }
                tree_node node;
                if(!fill_node(node,path,scan_id,file_que.front().id,iterator_to_id(path,cache))){
                    continue;
                };
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

        //std::cout <<"\n\n" << file_tree.size() << "\n\n" << sz <<"\n\n" << std::endl;
        for (int i = 0; i < file_tree.size(); i++) {
              print_node(file_tree[i]);
        }
        print_changes(changes);

        dbm.commit_transaction();
    }
