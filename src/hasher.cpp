#include "hasher.hpp"


std::string Hasher::convert_to_hex(uint8_t* out){
    std::string hex;
    hex.reserve(BLAKE3_OUT_LEN * 2);
    const char* lut = "0123456789abcdef";
    for (size_t i = 0;i<BLAKE3_OUT_LEN;i++) {
        hex.push_back(lut[out[i] >> 4]); //Extract first 4 bits
        hex.push_back(lut[out[i] & 0xF]); //gets last 4 bits
    }
    return hex;

}

std::string Hasher::hash_file(std::string file_path){
    std::ifstream file(file_path,std::ios::binary);
    if(!file){
        return {};
    }

    std::vector<uint8_t> buf(1024*1024); //buffer to which we will stream
    
    blake3_hasher hasher; //Hasher initialization
    blake3_hasher_init(&hasher);

    while(file){
        file.read(reinterpret_cast<char*>(buf.data()),buf.size());
        std::streamsize n = file.gcount(); //interesting method
        if(n>0){
            blake3_hasher_update(&hasher, buf.data(),static_cast<size_t>(n));
        }
    }
    uint8_t out[BLAKE3_OUT_LEN];
    blake3_hasher_finalize(&hasher,out, BLAKE3_OUT_LEN);

    return convert_to_hex(out);
}

std::string Hasher::hash_string(std::string value){

    blake3_hasher hasher;
    blake3_hasher_init(&hasher);
    blake3_hasher_update(&hasher, value.data(), value.size());
    uint8_t out[BLAKE3_OUT_LEN];
    blake3_hasher_finalize(&hasher,out,BLAKE3_OUT_LEN);
    return convert_to_hex(out);
}

