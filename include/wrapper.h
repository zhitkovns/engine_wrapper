#ifndef WRAPPER_H
#define WRAPPER_H

#include "command.h"
#include <functional>
#include <type_traits>
#include <iostream>
#include <sstream>
#include <utility>
#include <map>
#include <tuple>

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
            (paramTypes.push_back(getTypeName<Args>()), ...);
        }
    }
    
    // проверяет типы значений по умолчанию
    template<size_t... Is>
    void validateDefaultsImpl(std::index_sequence<Is...>) {
        if constexpr (sizeof...(Args) > 0) {
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
        
        method = [m](T* obj, Args... args) { return (obj->*m)(args...); };
        initParamInfo();
        
        // проверяет типы значений по умолчанию
        if constexpr (sizeof...(Args) > 0) {
            // проверяем в runtime, есть ли значения по умолчанию
            if (!defaults.empty()) {
                validateDefaultsImpl(std::make_index_sequence<sizeof...(Args)>{});
            }
        }
    }
    
    // конструктор для константных методов
    Wrapper(T* obj, ReturnType (T::*m)(Args...) const, 
           const std::vector<std::pair<std::string, std::any>>& defaults_vec = {})
        : object(obj), defaults(defaults_vec) {
        
        if (!object) throw std::runtime_error("Wrapper: object pointer cannot be null");
        
        method = [m](T* obj, Args... args) { return (obj->*m)(args...); };
        initParamInfo();
        
        if constexpr (sizeof...(Args) > 0) {
            if (!defaults.empty()) {
                validateDefaultsImpl(std::make_index_sequence<sizeof...(Args)>{});
            }
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

#endif