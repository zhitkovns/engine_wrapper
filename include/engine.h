#ifndef ENGINE_H
#define ENGINE_H

#include "command.h"
#include "wrapper.h"
#include <memory>
#include <map>
#include <stdexcept>
#include <sstream>

// движок для управления командами
class Engine {
private:
    std::map<std::string, std::unique_ptr<Command>> commands;

public:
    template<typename T, typename ReturnType, typename... Args>
    void register_command(const std::string& name, T* object, ReturnType (T::*method)(Args...),
                         const std::vector<std::pair<std::string, std::any>>& defaults = {}) {
        if (name.empty()) throw std::runtime_error("Cannot register command with empty name");
        if (commands.find(name) != commands.end()) {
            throw std::runtime_error("Command already registered: " + name);
        }
        commands[name] = std::make_unique<Wrapper<T, ReturnType, Args...>>(object, method, defaults);
    }
    
    template<typename T, typename ReturnType, typename... Args>
    void register_const_command(const std::string& name, T* object, ReturnType (T::*method)(Args...) const,
                               const std::vector<std::pair<std::string, std::any>>& defaults = {}) {
        if (name.empty()) throw std::runtime_error("Cannot register command with empty name");
        if (commands.find(name) != commands.end()) {
            throw std::runtime_error("Command already registered: " + name);
        }
        commands[name] = std::make_unique<Wrapper<T, ReturnType, Args...>>(object, method, defaults);
    }

    // регистрирует существующую команду
    void register_command(std::unique_ptr<Command> command, const std::string& name) {
        if (!command) throw std::runtime_error("Cannot register null command: " + name);
        if (name.empty()) throw std::runtime_error("Cannot register command with empty name");
        if (commands.find(name) != commands.end()) {
            throw std::runtime_error("Command already registered: " + name);
        }
        commands[name] = std::move(command);
    }

    // выполняет команду
    std::any execute(const std::string& commandName, 
                    const std::vector<std::pair<std::string, std::any>>& args = {}) {
        if (commandName.empty()) throw std::runtime_error("Command name cannot be empty");
        
        auto it = commands.find(commandName);
        if (it == commands.end()) throw std::runtime_error("Command not found: " + commandName);
        if (!it->second) throw std::runtime_error("Command is null: " + commandName);
        
        return it->second->execute(args);
    }
    
    // выполняет команду с приведением результата к заданному типу
    template<typename ResultType>
    ResultType execute_as(const std::string& commandName, 
                         const std::vector<std::pair<std::string, std::any>>& args = {}) {
        std::any result = execute(commandName, args);
        try {
            return std::any_cast<ResultType>(result);
        } catch (const std::bad_any_cast&) {
            auto it = commands.find(commandName);
            std::string expectedType = typeid(ResultType).name();
            std::string actualType = it->second->getReturnType();
            
            std::stringstream ss;
            ss << "Type mismatch in command result. Expected: " 
               << expectedType << ", Actual return type: " << actualType;
            throw std::runtime_error(ss.str());
        }
    }
    
    struct CommandInfo {
        std::vector<std::string> paramNames;
        std::vector<std::string> paramTypes;
        std::string returnType;
    };
    
    CommandInfo getCommandInfo(const std::string& commandName) const {
        if (commandName.empty()) throw std::runtime_error("Command name cannot be empty");
        
        auto it = commands.find(commandName);
        if (it == commands.end()) throw std::runtime_error("Command not found: " + commandName);
        if (!it->second) throw std::runtime_error("Command is null: " + commandName);
        
        CommandInfo info;
        info.paramNames = it->second->getParamNames();
        info.paramTypes = it->second->getParamTypes();
        info.returnType = it->second->getReturnType();
        return info;
    }
    
    std::vector<std::string> getCommandParams(const std::string& commandName) const {
        return getCommandInfo(commandName).paramNames;
    }
    
    // проверяет существование команды
    bool has_command(const std::string& commandName) const {
        if (commandName.empty()) return false;
        auto it = commands.find(commandName);
        return it != commands.end() && it->second != nullptr;
    }
    
    void clear() {
        commands.clear();
    }
    
    // получает список имен всех команд
    std::vector<std::string> get_command_list() const {
        std::vector<std::string> result;
        for (const auto& pair : commands) {
            if (pair.second) result.push_back(pair.first);
        }
        return result;
    }
    
    // получает количество команд
    size_t command_count() const {
        size_t count = 0;
        for (const auto& pair : commands) {
            if (pair.second) ++count;
        }
        return count;
    }
};

#endif