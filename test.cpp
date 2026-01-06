#include <iostream>

class MyClass{
public:
    explicit MyClass(const char x) : x_(x) {}
    char getValue() const{
        return x_;
    }
private:
    char x_;
};

int main(){
    int y = 96;
    MyClass x = y;
    MyClass x{y};
    std::cout << x.getValue();
}