#include <cmath>
#include <iostream>

float function(double x) {
    return 4 / (1 + pow(x, 2));
}

int main() {
    function(3);
    std::cout << "hej\n";
}