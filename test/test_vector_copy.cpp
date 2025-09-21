#include <vector>
#include <iostream>
#include <string>

std::vector<std::string> getVector() {
    std::vector<std::string> v = {"hello", "world", "test"};
    std::cout << "Vector address in function: " << &v << std::endl;
    std::cout << "First element address in function: " << &v[0] << std::endl;
    return v;
}

int main() {
    std::cout << "Without move:" << std::endl;
    std::vector<std::string> v1 = getVector();
    std::cout << "Vector address after assignment: " << &v1 << std::endl;
    std::cout << "First element address after assignment: " << &v1[0] << std::endl;

    std::cout << "\nWith move:" << std::endl;
    std::vector<std::string> v2 = std::move(getVector());
    std::cout << "Vector address after move: " << &v2 << std::endl;
    std::cout << "First element address after move: " << &v2[0] << std::endl;

    return 0;
}