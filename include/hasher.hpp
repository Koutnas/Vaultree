#pragma once
#include <iostream>
#include <vector>
#include <fstream>
#include "blake3.h"

class Hasher{
public:

    static std::string compute_hash(std::string file_path);

};