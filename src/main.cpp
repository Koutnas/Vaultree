#include "versioneer.hpp"
#include "dir_manager.hpp"

int main(){       
        auto start = std::chrono::system_clock::now();

        //versioneer vers = versioneer("/home/ondra/");
        //vers.get_filesystem_changes();
        dir_manager dir = dir_manager();
        dir.update_object_storage();
        auto end = std::chrono::system_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::cout << elapsed.count() << '\n';
        return 0;
}
