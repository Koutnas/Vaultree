#include "file_processor.hpp"

file_processor::file_processor(Hasher& h, db_manager& d,std::string& root )
    : hasher(h),dbm(d){
        this->root = root;
    }



void file_processor::process_existing(tree_node node){
    std::string hash = "";
    if(!node.is_dir){
        hash = hasher.hash_file(root+node.path);
        if(hash.empty()){
            std::cout<<"Unable to hash file: "+root+node.path<<std::endl;
            return;
        }
    }
    std::lock_guard<std::mutex> lock(db_mutex);
        if(hash == node.hash){
            hashed.push_back(node);
        }else{
            node.hash = hash;
            if(!node.is_dir){
                dbm.update_hash(node);
            }
            changes.insert({node.id,MODIFIED});
            hashed.push_back(node);
        }
}

void file_processor::process_new(tree_node node){
    if(!node.is_dir){
        node.hash = hasher.hash_file(root+node.path);
        if(node.hash.empty()){
            std::cout<<"Unable to hash file: "+root+node.path<<std::endl;
            return;
        }
    }
    std::lock_guard<std::mutex> lock(db_mutex);
        if(!node.is_dir){
            dbm.update_hash(node);
        }
        changes.insert({node.id,ADDED});
        hashed.push_back(node);
}

void file_processor::hash_new_files(std::vector<tree_node>& added){
    for(tree_node& node: added){
        pool.submit([this,node](){this->process_new(node);});
    }
    added.clear();
}

void file_processor::hash_exist_files(std::vector<tree_node>& modified){
    for(tree_node& node: modified){
        pool.submit([this,node](){this->process_existing(node);});
    }
    modified.clear();
}

std::vector<tree_node>& file_processor::get_hashed(){
    return hashed;
}

std::unordered_map<int,int>& file_processor::get_changes(){
    return changes;
}


