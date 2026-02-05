#include "dir_manager.hpp"

std::string dir_manager::hash_to_path(std::string& hash){
    std::string path = ".vaultree/objects/" + hash.substr(0,2) + "/" + hash.substr(2,2) + "/" + hash;
    return path; 
}

void dir_manager::clean_object_storage(){
    std::unordered_set<std::string> redundant_objects;
    dbm.get_redundant_hashes(redundant_objects);
    if(redundant_objects.empty()){
        return;
    }
    for(auto it: redundant_objects){
        std::string path = hash_to_path(it);
        if(fs::exists(path)){
            fs::remove(path); //Removing the object itself
        }
        if(fs::is_empty(path.substr(0,23))){
            fs::remove(path.substr(0,23));
        }
        if(fs::is_empty(path.substr(0,20))){
            fs::remove(path.substr(0,20));
        }
    }
    dbm.clean_blobs();
}


void dir_manager::update_object_storage(){
    if(!fs::exists(".vaultree/objects")){
        fs::create_directory(".vaultree/objects");
    }
    clean_object_storage();
    std::unordered_set<std::string> objects;
    dbm.get_object_hashes(objects);

    for(auto it : objects){
        std::string path = hash_to_path(it);
        if(!fs::exists(path.substr(0,20))){
            fs::create_directory(path.substr(0,20));
        }
        if(!fs::exists(path.substr(0,23))){
            fs::create_directory(path.substr(0,23));
        }
        if(!fs::exists(path)){
            std::ofstream output(path);
        }
         
    }
}