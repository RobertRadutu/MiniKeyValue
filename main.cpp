#include "minikeyvalue.hpp"
#include <iostream>

int main(){
    MiniKeyValue mkv{"data.bin"};

    std::vector<uint8_t> values = {1, 2, 3, 4, 5};

    mkv.Set("Robert", values);
    mkv.Set("Andra", {20, 30, 40, 50 ,60, 61});

    std::vector<uint8_t> tmp = mkv.Get("Robert");
    for(const auto& v : tmp) std::cout << static_cast<int>(v) << ", ";
}

