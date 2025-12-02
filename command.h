#ifndef COMMAND_H
#define COMMAND_H

#include <vector>
#include <string>
#include <functional>
#include <any>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <iostream>
#include <sstream>
#include <utility>
#include <map>

// Базовый класс для всех команд
class Command {
public:
    virtual ~Command() = default;
    virtual std::any execute(const std::vector<std::pair<std::string, std::any>>& args) = 0;
    virtual std::vector<std::string> getParamNames() const = 0;
    virtual std::vector<std::string> getParamTypes() const = 0;
    virtual std::string getReturnType() const = 0;
};

// Обертка для методов класса с произвольной сигнатурой
template<typename T, typename ReturnType, typename... Args>
class Wrapper : public Command {
private:
    T* object;
    std::function<ReturnType(T*, Args...)> method;
    std::vector<std::pair<std::string, std::any>> defaults;
    std::vector<std::string> paramNames;
    std::vector<std::string> paramTypes;
    std::string returnTypeName;
    
    template<typename U>
    static std::string getTypeName() {
        return typeid(U).name();
    }
    
    void initTypesImpl() {}
    
    template<typename First, typename... Rest>
    void initTypesImpl() {
        paramTypes.push_back(getTypeName<First>());
        if constexpr (sizeof...(Rest) > 0) {
            initTypesImpl<Rest...>();
        }
    }

    template<size_t... Is>
    std::any call(const std::vector<std::pair<std::string, std::any>>& args, std::index_sequence<Is...>) {
        // Проверяем, что все обязательные параметры переданы
        if (args.size() < paramNames.size()) {
            for (size_t i = args.size(); i < paramNames.size(); ++i) {
                bool hasDefault = false;
                for (const auto& def : defaults) {
                    if (def.first == paramNames[i]) {
                        hasDefault = true;
                        break;
                    }
                }
                if (!hasDefault) {
                    throw std::runtime_error("Missing required argument: " + paramNames[i]);
                }
            }
        }
        
        std::tuple<Args...> tupleArgs;
        
        // Инициализируем значениями по умолчанию
        if constexpr (sizeof...(Args) > 0) {
            initDefaults(tupleArgs, std::make_index_sequence<sizeof...(Args)>{});
        }
        
        // Заполняем переданными значениями
        fillArgs(tupleArgs, args, std::make_index_sequence<sizeof...(Args)>{});
        
        return method(object, std::get<Is>(tupleArgs)...);
    }
    
    template<typename Tuple, size_t... Is>
    void initDefaults(Tuple& tuple, std::index_sequence<Is...>) {
        (initDefault<Is>(tuple), ...);
    }
    
    template<size_t I, typename Tuple>
    void initDefault(Tuple& tuple) {
        if (I < defaults.size()) {
            try {
                std::get<I>(tuple) = std::any_cast<typename std::tuple_element<I, std::tuple<Args...>>::type>(
                    defaults[I].second
                );
            } catch (const std::bad_any_cast&) {
                throw std::runtime_error("Default value type mismatch for parameter " + 
                                        std::to_string(I) + " (" + paramNames[I] + ")");
            }
        }
    }
    
    template<typename Tuple, size_t... Is>
    void fillArgs(Tuple& tuple, const std::vector<std::pair<std::string, std::any>>& args, std::index_sequence<Is...>) {
        (fillArg<Is>(tuple, args), ...);
    }
    
    template<size_t I, typename Tuple>
    void fillArg(Tuple& tuple, const std::vector<std::pair<std::string, std::any>>& args) {
        if (I >= paramNames.size()) return;
        
        const std::string& paramName = paramNames[I];
        
        for (const auto& arg : args) {
            if (arg.first == paramName) {
                try {
                    std::get<I>(tuple) = std::any_cast<
                        typename std::tuple_element<I, std::tuple<Args...>>::type
                    >(arg.second);
                } catch (const std::bad_any_cast&) {
                    std::stringstream ss;
                    ss << "Type mismatch for argument '" << paramName 
                       << "'. Expected: " << paramTypes[I]
                       << ", got different type.";
                    throw std::runtime_error(ss.str());
                }
                return;
            }
        }
    }

public:
    // Конструктор для неконстантных методов
    Wrapper(T* obj, ReturnType (T::*m)(Args...), 
           const std::vector<std::pair<std::string, std::any>>& defaults_vec = {})
        : object(obj), defaults(defaults_vec) {
        
        if (!object) {
            throw std::runtime_error("Wrapper: object pointer cannot be null");
        }
        
        method = [m](T* obj, Args... args) { return (obj->*m)(args...); };
        
        initParamInfo();
    }
    
    // Конструктор для константных методов
    Wrapper(T* obj, ReturnType (T::*m)(Args...) const, 
           const std::vector<std::pair<std::string, std::any>>& defaults_vec = {})
        : object(obj), defaults(defaults_vec) {
        
        if (!object) {
            throw std::runtime_error("Wrapper: object pointer cannot be null");
        }
        
        method = [m](T* obj, Args... args) { return (obj->*m)(args...); };
        
        initParamInfo();
    }
    
    void initParamInfo() {
        paramNames.clear();
        paramTypes.clear();
        
        // Если defaults не предоставлены, генерируем имена по умолчанию
        if (defaults.empty() && sizeof...(Args) > 0) {
            for (size_t i = 0; i < sizeof...(Args); ++i) {
                paramNames.push_back("param" + std::to_string(i + 1));
            }
        } else {
            for (const auto& def : defaults) {
                paramNames.push_back(def.first);
            }
        }
        
        if constexpr (sizeof...(Args) > 0) {
            initTypesImpl<Args...>();
        }
        
        returnTypeName = getTypeName<ReturnType>();
        
        // Defaults должны быть либо пустыми, либо содержать все параметры
        if (!defaults.empty() && defaults.size() != sizeof...(Args)) {
            throw std::runtime_error("Parameter count mismatch in Wrapper constructor. "
                                   "Defaults should contain all " + std::to_string(sizeof...(Args)) + 
                                   " parameters or be empty.");
        }
    }

    std::any execute(const std::vector<std::pair<std::string, std::any>>& args) override {
        // Проверяем дубликаты имен аргументов
        std::map<std::string, int> argCount;
        for (const auto& arg : args) {
            argCount[arg.first]++;
            if (argCount[arg.first] > 1) {
                throw std::runtime_error("Duplicate argument name: " + arg.first);
            }
        }
        
        if constexpr (sizeof...(Args) == 0) {
            return method(object);
        } else {
            return call(args, std::make_index_sequence<sizeof...(Args)>{});
        }
    }

    std::vector<std::string> getParamNames() const override {
        return paramNames;
    }
    
    std::vector<std::string> getParamTypes() const override {
        return paramTypes;
    }
    
    std::string getReturnType() const override {
        return returnTypeName;
    }
};

// Движок для регистрации и выполнения команд
class Engine {
private:
    std::map<std::string, std::unique_ptr<Command>> commands;

public:
    void register_command(std::unique_ptr<Command> command, const std::string& name) {
        if (!command) {
            throw std::runtime_error("Cannot register null command: " + name);
        }
        
        if (name.empty()) {
            throw std::runtime_error("Cannot register command with empty name");
        }
        
        if (commands.find(name) != commands.end()) {
            throw std::runtime_error("Command already registered: " + name);
        }
        
        commands[name] = std::move(command);
    }

    std::any execute(const std::string& commandName, 
                    const std::vector<std::pair<std::string, std::any>>& args = {}) {
        if (commandName.empty()) {
            throw std::runtime_error("Command name cannot be empty");
        }
        
        auto it = commands.find(commandName);
        if (it == commands.end()) {
            throw std::runtime_error("Command not found: " + commandName);
        }
        
        if (!it->second) {
            throw std::runtime_error("Command is null: " + commandName);
        }
        
        return it->second->execute(args);
    }
    
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
        if (commandName.empty()) {
            throw std::runtime_error("Command name cannot be empty");
        }
        
        auto it = commands.find(commandName);
        if (it == commands.end()) {
            throw std::runtime_error("Command not found: " + commandName);
        }
        
        if (!it->second) {
            throw std::runtime_error("Command is null: " + commandName);
        }
        
        CommandInfo info;
        info.paramNames = it->second->getParamNames();
        info.paramTypes = it->second->getParamTypes();
        info.returnType = it->second->getReturnType();
        return info;
    }
    
    std::vector<std::string> getCommandParams(const std::string& commandName) const {
        return getCommandInfo(commandName).paramNames;
    }
    
    bool has_command(const std::string& commandName) const {
        if (commandName.empty()) {
            return false;
        }
        auto it = commands.find(commandName);
        return it != commands.end() && it->second != nullptr;
    }
    
    void clear() {
        commands.clear();
    }
    
    std::vector<std::string> get_command_list() const {
        std::vector<std::string> result;
        for (const auto& pair : commands) {
            if (pair.second) {
                result.push_back(pair.first);
            }
        }
        return result;
    }
    
    size_t command_count() const {
        size_t count = 0;
        for (const auto& pair : commands) {
            if (pair.second) {
                ++count;
            }
        }
        return count;
    }
};

#endif