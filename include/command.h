#ifndef COMMAND_H
#define COMMAND_H

#include <vector>
#include <string>
#include <any>

// базовый интерфейс для всех команд
class Command {
public:
    virtual ~Command() = default;
    
    // выполняет команду с заданными аргументами
    virtual std::any execute(const std::vector<std::pair<std::string, std::any>>& args) = 0;
    
    virtual std::vector<std::string> getParamNames() const = 0;
    
    // получает типы параметров команды
    virtual std::vector<std::string> getParamTypes() const = 0;
    
    virtual std::string getReturnType() const = 0;
};

#endif