#include "versioneer.hpp"

int main()
{
        auto start = std::chrono::system_clock::now();

        versioneer vers = versioneer("/home/ondra/test");
        vers.get_filesystem_changes();


        auto end = std::chrono::system_clock::now();
        auto elapsed = end - start;
        std::cout << elapsed.count() << '\n';
        return 0;
}
