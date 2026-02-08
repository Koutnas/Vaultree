#include "dir_manager.hpp"

class orchestrator
{
private:
    db_manager dbm = db_manager(BACKUP);
public:
    void backup_files();
    void restore_files();
};
