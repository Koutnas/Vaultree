#pragma once
#include <iostream>
#include <vector>
#include <fstream>
#include "blake3.h"

class Hasher{
private:
    static std::string convert_to_hex(uint8_t* out);
public:

    static std::string hash_file(std::string file_path);

    static std::string hash_string(std::string value);

};