#include "db_manager.hpp"
#include <fstream>


namespace fs = std::filesystem;


class dir_manager
{
private:
    db_manager dbm = db_manager(DIR);

    void clean_object_storage();
public:

    void update_object_storage();

    void create_file_structure();

    static std::string hash_to_path(std::string& hash);
};
