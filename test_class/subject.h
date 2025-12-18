#ifndef SUBJECT_H
#define SUBJECT_H

#include <string>
#include <iostream>

class Subject {
private:
    int value = 0;
    
public:
    Subject() = default;
    explicit Subject(int val) : value(val) {}
    
    // неконстантные методы
    int f3(int arg1, int arg2) {
        return arg1 * arg2;
    }
    
    int f2(int arg1) {
        return arg1 * 2;
    }
    
    int f0() {
        return 42;
    }
    
    std::string concatenate(std::string a, std::string b) {
        return a + b;
    }
    
    double divide(double a, double b) {
        if (b == 0.0) throw std::runtime_error("Division by zero");
        return a / b;
    }
    
    void setValue(int val) {
        value = val;
    }
    
    // константные методы
    int getValue() const {
        return value;
    }
    
    int multiplyBy(int factor) const {
        return value * factor;
    }
    
    std::string getDescription() const {
        return "Subject with value: " + std::to_string(value);
    }
    
    int add(int a, int b) const {
        return a + b;
    }
};

#endif