#include "hasher.hpp"


std::string Hasher::compute_hash(std::string file_path){
    std::ifstream file(file_path,std::ios::binary);
    if(!file){
        return {};
    }

    std::vector<u_int> buf(512*1024); //buffer to which we will stream
    
    blake3_hasher hasher; //Hasher initialization
    blake3_hasher_init(&hasher);

    while(file){
        file.read(reinterpret_cast<char*>(buf.data()),buf.size());
        std::streamsize n = file.gcount(); //interesting method
        if(n>0){
            blake3_hasher_update(&hasher, buf.data(),(uint8_t)n);
        }
    }
    uint8_t out[BLAKE3_OUT_LEN];
    blake3_hasher_finalize(&hasher,out, BLAKE3_OUT_LEN);

    std::string hex;
    hex.reserve(BLAKE3_OUT_LEN * 2);
    const char* lut = "0123456789abcdef";
    for (unsigned char b : out) {
        hex.push_back(lut[b >> 4]); //Extract first 4 bits
        hex.push_back(lut[b & 0xF]); //gets last 4 bits
    }
    return hex;
}

