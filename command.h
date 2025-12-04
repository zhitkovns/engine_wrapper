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

// базовый интерфейс для всех команд
class Command {
public:
    virtual ~Command() = default;
    
    // выполняет команду с заданными аргументами
    virtual std::any execute(const std::vector<std::pair<std::string, std::any>>& args) = 0;
    
    // получает имена параметров команды
    virtual std::vector<std::string> getParamNames() const = 0;
    
    // получает типы параметров команды
    virtual std::vector<std::string> getParamTypes() const = 0;
    
    // получает тип возвращаемого значения
    virtual std::string getReturnType() const = 0;
};

// обертка для методов класса с произвольной сигнатурой
template<typename T, typename ReturnType, typename... Args>
class Wrapper : public Command {
private:
    T* object;
    std::function<ReturnType(T*, Args...)> method;
    std::vector<std::pair<std::string, std::any>> defaults;
    std::vector<std::string> paramNames;
    std::vector<std::string> paramTypes;
    std::string returnTypeName;
    
    // получает имя типа в виде строки
    template<typename U>
    static std::string getTypeName() {
        return typeid(U).name();
    }
    
    // инициализирует информацию о типах параметров
    void initTypes() {
        if constexpr (sizeof...(Args) > 0) {
            // fold expression: обрабатывает все типы Args
            (paramTypes.push_back(getTypeName<Args>()), ...);
        }
    }
    
    // проверяет типы значений по умолчанию
    template<size_t... Is>
    void validateDefaultsImpl(std::index_sequence<Is...>) {
        if constexpr (sizeof...(Args) > 0) {
            // проверяет каждый параметр с помощью fold expression
            ([&]() {
                try {
                    // пытается преобразовать значение по умолчанию к нужному типу
                    std::ignore = std::any_cast<
                        typename std::tuple_element<Is, std::tuple<Args...>>::type
                    >(defaults[Is].second);
                } catch (const std::bad_any_cast&) {
                    throw std::runtime_error("Default value type mismatch for parameter " + 
                                            std::to_string(Is) + " (" + paramNames[Is] + ")");
                }
            }(), ...);
        }
    }
    
    // вызывает метод с обработкой аргументов
    template<size_t... Is>
    std::any call(const std::vector<std::pair<std::string, std::any>>& args, std::index_sequence<Is...>) {
        // проверяет, что переданы все обязательные аргументы
        if (args.size() < paramNames.size() && defaults.empty()) {
            for (size_t i = args.size(); i < paramNames.size(); ++i) {
                throw std::runtime_error("Missing required argument: " + paramNames[i]);
            }
        }
        
        std::tuple<Args...> tupleArgs;
        
        // инициализирует значения по умолчанию
        if constexpr (sizeof...(Args) > 0) {
            if (!defaults.empty()) {
                ([&]() {
                    std::get<Is>(tupleArgs) = 
                        std::any_cast<typename std::tuple_element<Is, std::tuple<Args...>>::type>(
                            defaults[Is].second
                        );
                }(), ...);
            }
        }
        
        // заполняет аргументами из входных данных
        ((fillArg<Is>(tupleArgs, args)), ...);
        
        // вызывает метод с распакованными аргументами
        return method(object, std::get<Is>(tupleArgs)...);
    }
    
    // заполняет один аргумент в кортеже
    template<size_t I, typename Tuple>
    void fillArg(Tuple& tuple, const std::vector<std::pair<std::string, std::any>>& args) {
        if (I >= paramNames.size()) return;
        
        const std::string& paramName = paramNames[I];
        
        // ищет аргумент с нужным именем
        for (const auto& arg : args) {
            if (arg.first == paramName) {
                try {
                    // преобразует значение к нужному типу
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
        
        // если аргумент не найден и нет значения по умолчанию
        if (defaults.empty()) {
            throw std::runtime_error("Argument not provided and no default value: " + paramName);
        }
        // если есть значение по умолчанию, оно уже установлено
    }

public:
    // конструктор для неконстантных методов
    Wrapper(T* obj, ReturnType (T::*m)(Args...), 
           const std::vector<std::pair<std::string, std::any>>& defaults_vec = {})
        : object(obj), defaults(defaults_vec) {
        
        if (!object) throw std::runtime_error("Wrapper: object pointer cannot be null");
        
        // сохраняет метод в std::function
        method = [m](T* obj, Args... args) { return (obj->*m)(args...); };
        initParamInfo();
        
        // проверяет типы значений по умолчанию
        if constexpr (sizeof...(Args) > 0 && !defaults.empty()) {
            validateDefaultsImpl(std::make_index_sequence<sizeof...(Args)>{});
        }
    }
    
    // конструктор для константных методов
    Wrapper(T* obj, ReturnType (T::*m)(Args...) const, 
           const std::vector<std::pair<std::string, std::any>>& defaults_vec = {})
        : object(obj), defaults(defaults_vec) {
        
        if (!object) throw std::runtime_error("Wrapper: object pointer cannot be null");
        
        method = [m](T* obj, Args... args) { return (obj->*m)(args...); };
        initParamInfo();
        
        if constexpr (sizeof...(Args) > 0 && !defaults.empty()) {
            validateDefaultsImpl(std::make_index_sequence<sizeof...(Args)>{});
        }
    }
    
    // инициализирует информацию о параметрах
    void initParamInfo() {
        paramNames.clear();
        paramTypes.clear();
        
        constexpr size_t paramCount = sizeof...(Args);
        
        if (!defaults.empty()) {
            // использует имена из значений по умолчанию
            for (const auto& def : defaults) {
                paramNames.push_back(def.first);
            }
            
            // проверяет, что количество значений по умолчанию совпадает с количеством параметров
            if (defaults.size() != paramCount) {
                throw std::runtime_error("Parameter count mismatch. Defaults should contain all " + 
                                       std::to_string(paramCount) + " parameters or be empty.");
            }
        } else if (paramCount > 0) {
            // генерирует имена по умолчанию
            for (size_t i = 0; i < paramCount; ++i) {
                paramNames.push_back("param" + std::to_string(i + 1));
            }
        }
        
        initTypes();
        returnTypeName = getTypeName<ReturnType>();
    }

    // выполняет команду с аргументами
    std::any execute(const std::vector<std::pair<std::string, std::any>>& args) override {
        // проверяет на дубликаты имен аргументов
        std::map<std::string, int> argCount;
        for (const auto& arg : args) {
            if (++argCount[arg.first] > 1) {
                throw std::runtime_error("Duplicate argument name: " + arg.first);
            }
        }
        
        if constexpr (sizeof...(Args) == 0) {
            // метод без параметров
            return method(object);
        } else {
            // метод с параметрами
            return call(args, std::make_index_sequence<sizeof...(Args)>{});
        }
    }

    // получает имена параметров
    std::vector<std::string> getParamNames() const override {
        return paramNames;
    }
    
    // получает типы параметров
    std::vector<std::string> getParamTypes() const override {
        return paramTypes;
    }
    
    // получает тип возвращаемого значения
    std::string getReturnType() const override {
        return returnTypeName;
    }
};

// движок для управления командами
class Engine {
private:
    std::map<std::string, std::unique_ptr<Command>> commands;

public:
    // регистрирует команду
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
            // пытается привести результат
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
    
    // информация о команде
    struct CommandInfo {
        std::vector<std::string> paramNames;
        std::vector<std::string> paramTypes;
        std::string returnType;
    };
    
    // получает информацию о команде
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
    
    // получает параметры команды
    std::vector<std::string> getCommandParams(const std::string& commandName) const {
        return getCommandInfo(commandName).paramNames;
    }
    
    // проверяет существование команды
    bool has_command(const std::string& commandName) const {
        if (commandName.empty()) return false;
        auto it = commands.find(commandName);
        return it != commands.end() && it->second != nullptr;
    }
    
    // очищает все команды
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