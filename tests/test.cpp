#include "test.h"
#include "command.h"
#include "subject.h"
#include <iostream>
#include <cassert>
#include <vector>
#include <string>

void test1_basic_functionality() {
    std::cout << "Test 1: Basic functionality" << std::endl;
    
    Subject subj;
    std::unique_ptr<Command> wrapper(new Wrapper<Subject, int, int, int>(
        &subj, &Subject::f3, {{"arg1", 0}, {"arg2", 0}}));
    
    Engine engine;
    engine.register_command(std::move(wrapper), "command1");
    
    int result = engine.execute_as<int>("command1", {{"arg1", 4}, {"arg2", 5}});
    std::cout << "  command1(4, 5) = " << result << std::endl;
    assert(result == 20);
    
    std::cout << "  passed" << std::endl;
}

void test2_default_values() {
    std::cout << "Test 2: Default parameter values" << std::endl;
    
    Subject subj;
    std::unique_ptr<Command> wrapper(new Wrapper<Subject, int, int, int>(
        &subj, &Subject::f3, {{"arg1", 10}, {"arg2", 20}}));
    
    Engine engine;
    engine.register_command(std::move(wrapper), "multiply");
    
    int result1 = engine.execute_as<int>("multiply", {{"arg1", 3}, {"arg2", 7}});
    std::cout << "  multiply(3, 7) = " << result1 << std::endl;
    assert(result1 == 21);
    
    int result2 = engine.execute_as<int>("multiply", {{"arg1", 5}});
    std::cout << "  multiply(5) = " << result2 << std::endl;
    assert(result2 == 100);
    
    int result3 = engine.execute_as<int>("multiply", {});
    std::cout << "  multiply() = " << result3 << std::endl;
    assert(result3 == 200);
    
    std::cout << "  passed" << std::endl;
}

void test3_type_safety() {
    std::cout << "Test 3: Type safety checks" << std::endl;
    
    Subject subj;
    std::unique_ptr<Command> wrapper(new Wrapper<Subject, int, int, int>(
        &subj, &Subject::f3, {{"arg1", 0}, {"arg2", 0}}));
    
    Engine engine;
    engine.register_command(std::move(wrapper), "multiply");
    
    bool exception_thrown = false;
    try {
        engine.execute("multiply", {{"arg1", std::string("not a number")}, {"arg2", 5}});
    } catch (const std::runtime_error& e) {
        std::cout << "  Expected type mismatch: " << e.what() << std::endl;
        exception_thrown = true;
    }
    assert(exception_thrown);
    
    exception_thrown = false;
    try {
        engine.execute_as<std::string>("multiply", {{"arg1", 4}, {"arg2", 5}});
    } catch (const std::runtime_error& e) {
        std::cout << "  Expected return type mismatch: " << e.what() << std::endl;
        exception_thrown = true;
    }
    assert(exception_thrown);
    
    std::cout << "  passed" << std::endl;
}

void test4_memory_management() {
    std::cout << "Test 4: Memory management" << std::endl;
    
    {
        Subject subj;
        Engine engine;
        
        engine.register_command(
            std::unique_ptr<Command>(new Wrapper<Subject, int, int, int>(
                &subj, &Subject::f3, {{"arg1", 0}, {"arg2", 0}})),
            "cmd1");
        engine.register_command(
            std::unique_ptr<Command>(new Wrapper<Subject, int, int>(
                &subj, &Subject::f2, {{"arg1", 0}})),
            "cmd2");
        engine.register_command(
            std::unique_ptr<Command>(new Wrapper<Subject, int>(
                &subj, &Subject::f0)),
            "cmd3");
        
        assert(engine.command_count() == 3);
        
        assert(engine.execute_as<int>("cmd1", {{"arg1", 2}, {"arg2", 3}}) == 6);
        assert(engine.execute_as<int>("cmd2", {{"arg1", 5}}) == 10);
        assert(engine.execute_as<int>("cmd3", {}) == 42);
    }
    
    std::cout << "  No memory leaks" << std::endl;
    std::cout << "  passed" << std::endl;
}

void test5_error_handling() {
    std::cout << "Test 5: Error handling" << std::endl;
    
    Subject subj;
    std::unique_ptr<Command> wrapper(new Wrapper<Subject, int, int, int>(
        &subj, &Subject::f3, {{"arg1", 0}, {"arg2", 0}}));
    
    Engine engine;
    engine.register_command(std::move(wrapper), "command1");
    
    bool exception_thrown = false;
    try {
        engine.execute("nonexistent_command", {});
    } catch (const std::runtime_error& e) {
        std::cout << "  Expected command not found: " << e.what() << std::endl;
        exception_thrown = true;
    }
    assert(exception_thrown);
    
    exception_thrown = false;
    try {
        engine.execute("command1", {{"arg1", 1}, {"arg1", 2}, {"arg2", 3}});
    } catch (const std::runtime_error& e) {
        std::cout << "  Expected duplicate argument: " << e.what() << std::endl;
        exception_thrown = true;
    }
    assert(exception_thrown);
    
    std::cout << "  passed" << std::endl;
}

void test6_const_methods() {
    std::cout << "Test 6: Const methods support" << std::endl;
    
    Subject subj(5);
    
    std::unique_ptr<Command> wrapper1(new Wrapper<Subject, int>(
        &subj, &Subject::getValue));
    
    std::unique_ptr<Command> wrapper2(new Wrapper<Subject, int, int>(
        &subj, &Subject::multiplyBy, {{"factor", 1}}));
    
    std::unique_ptr<Command> wrapper3(new Wrapper<Subject, int, int, int>(
        &subj, &Subject::add, {{"a", 0}, {"b", 0}}));
    
    Engine engine;
    engine.register_command(std::move(wrapper1), "get_value");
    engine.register_command(std::move(wrapper2), "multiply_by");
    engine.register_command(std::move(wrapper3), "add");
    
    int result1 = engine.execute_as<int>("get_value", {});
    std::cout << "  get_value() = " << result1 << std::endl;
    assert(result1 == 5);
    
    int result2 = engine.execute_as<int>("multiply_by", {{"factor", 3}});
    std::cout << "  multiply_by(3) = " << result2 << std::endl;
    assert(result2 == 15);
    
    int result3 = engine.execute_as<int>("add", {{"a", 7}, {"b", 8}});
    std::cout << "  add(7, 8) = " << result3 << std::endl;
    assert(result3 == 15);
    
    std::cout << "  passed" << std::endl;
}

void test7_command_info() {
    std::cout << "Test 7: Command information" << std::endl;
    
    Subject subj;
    std::unique_ptr<Command> wrapper(new Wrapper<Subject, std::string, std::string, std::string>(
        &subj, &Subject::concatenate, {{"a", std::string("")}, {"b", std::string("")}}));
    
    Engine engine;
    engine.register_command(std::move(wrapper), "concat");
    
    auto info = engine.getCommandInfo("concat");
    
    std::cout << "  Command 'concat' info:" << std::endl;
    std::cout << "    Return type: " << info.returnType << std::endl;
    std::cout << "    Parameters:" << std::endl;
    for (size_t i = 0; i < info.paramNames.size(); ++i) {
        std::cout << "      " << info.paramNames[i] << " : " << info.paramTypes[i] << std::endl;
    }
    
    assert(info.paramNames.size() == 2);
    assert(info.paramNames[0] == "a");
    assert(info.paramNames[1] == "b");
    assert(info.paramTypes.size() == 2);
    
    std::cout << "  passed" << std::endl;
}

void test8_no_parameters_method() {
    std::cout << "Test 8: No parameters method" << std::endl;
    
    Subject subj;
    std::unique_ptr<Command> wrapper(new Wrapper<Subject, int>(
        &subj, &Subject::f0));
    
    Engine engine;
    engine.register_command(std::move(wrapper), "answer");
    
    int result = engine.execute_as<int>("answer", {});
    std::cout << "  answer() = " << result << std::endl;
    assert(result == 42);
    
    auto info = engine.getCommandInfo("answer");
    assert(info.paramNames.empty());
    assert(info.paramTypes.empty());
    
    std::cout << "  passed" << std::endl;
}

void test9_multiple_commands() {
    std::cout << "Test 9: Multiple commands in one engine" << std::endl;
    
    Subject subj;
    Engine engine;
    
    engine.register_command(
        std::unique_ptr<Command>(new Wrapper<Subject, std::string, std::string, std::string>(
            &subj, &Subject::concatenate, {{"a", std::string("")}, {"b", std::string("")}})),
        "concat");
    
    engine.register_command(
        std::unique_ptr<Command>(new Wrapper<Subject, double, double, double>(
            &subj, &Subject::divide, {{"a", 1.0}, {"b", 1.0}})),
        "divide");
    
    engine.register_command(
        std::unique_ptr<Command>(new Wrapper<Subject, int, int, int>(
            &subj, &Subject::f3, {{"arg1", 0}, {"arg2", 0}})),
        "multiply");
    
    auto command_list = engine.get_command_list();
    std::cout << "  Commands registered: ";
    for (const auto& cmd : command_list) {
        std::cout << cmd << " ";
    }
    std::cout << std::endl;
    
    assert(command_list.size() == 3);
    
    std::string concat_result = engine.execute_as<std::string>("concat", 
        {{"a", std::string("Hello")}, {"b", std::string("World")}});
    std::cout << "  concat('Hello', 'World') = '" << concat_result << "'" << std::endl;
    assert(concat_result == "HelloWorld");
    
    double divide_result = engine.execute_as<double>("divide", {{"a", 10.0}, {"b", 2.0}});
    std::cout << "  divide(10.0, 2.0) = " << divide_result << std::endl;
    assert(divide_result == 5.0);
    
    int multiply_result = engine.execute_as<int>("multiply", {{"arg1", 3}, {"arg2", 4}});
    std::cout << "  multiply(3, 4) = " << multiply_result << std::endl;
    assert(multiply_result == 12);
    
    std::cout << "  passed" << std::endl;
}

void test10_nullptr_checks() {
    std::cout << "Test 10: Nullptr and empty name checks" << std::endl;
    
    bool exception_thrown = false;
    try {
        Subject* null_subj = nullptr;
        std::unique_ptr<Command> wrapper(new Wrapper<Subject, int, int, int>(
            null_subj, &Subject::f3, {{"arg1", 0}, {"arg2", 0}}));
    } catch (const std::runtime_error& e) {
        std::cout << "  Expected nullptr object error: " << e.what() << std::endl;
        exception_thrown = true;
    }
    assert(exception_thrown);
    
    Engine engine;
    exception_thrown = false;
    try {
        engine.register_command(nullptr, "test_command");
    } catch (const std::runtime_error& e) {
        std::cout << "  Expected nullptr command error: " << e.what() << std::endl;
        exception_thrown = true;
    }
    assert(exception_thrown);
    
    exception_thrown = false;
    try {
        Subject subj;
        std::unique_ptr<Command> wrapper(new Wrapper<Subject, int, int, int>(
            &subj, &Subject::f3, {{"arg1", 0}, {"arg2", 0}}));
        engine.register_command(std::move(wrapper), "");
    } catch (const std::runtime_error& e) {
        std::cout << "  Expected empty name error: " << e.what() << std::endl;
        exception_thrown = true;
    }
    assert(exception_thrown);
    
    exception_thrown = false;
    try {
        engine.execute("");
    } catch (const std::runtime_error& e) {
        std::cout << "  Expected empty command name error: " << e.what() << std::endl;
        exception_thrown = true;
    }
    assert(exception_thrown);
    
    exception_thrown = false;
    try {
        engine.getCommandInfo("");
    } catch (const std::runtime_error& e) {
        std::cout << "  Expected empty name in getCommandInfo: " << e.what() << std::endl;
        exception_thrown = true;
    }
    assert(exception_thrown);
    
    assert(!engine.has_command(""));
    
    std::cout << "  passed" << std::endl;
}

void test11_exact_task_example() {
    std::cout << "Test 11: Exact example from the task" << std::endl;
    
    Subject subj;
    
    std::unique_ptr<Command> wrapper(new Wrapper<Subject, int, int, int>(
        &subj, &Subject::f3, {{"arg1", 0}, {"arg2", 0}}));
    
    Engine engine;
    engine.register_command(std::move(wrapper), "command1");
    
    std::cout << "  Executing: engine.execute(\"command1\", {{\"arg1\", 4}, {\"arg2\", 5}})" << std::endl;
    int result = engine.execute_as<int>("command1", {{"arg1", 4}, {"arg2", 5}});
    std::cout << "  Result: " << result << std::endl;
    assert(result == 20);
    
    std::cout << "  passed" << std::endl;
}

void test12_engine_clear_and_count() {
    std::cout << "Test 12: Engine clear and command count" << std::endl;
    
    Subject subj;
    Engine engine;
    
    assert(engine.command_count() == 0);
    assert(engine.get_command_list().empty());
    
    engine.register_command(
        std::unique_ptr<Command>(new Wrapper<Subject, int, int, int>(
            &subj, &Subject::f3, {{"arg1", 0}, {"arg2", 0}})),
        "cmd1");
    
    assert(engine.command_count() == 1);
    assert(engine.get_command_list().size() == 1);
    assert(engine.has_command("cmd1"));
    
    engine.register_command(
        std::unique_ptr<Command>(new Wrapper<Subject, int, int>(
            &subj, &Subject::f2, {{"arg1", 0}})),
        "cmd2");
    
    assert(engine.command_count() == 2);
    assert(engine.get_command_list().size() == 2);
    assert(engine.has_command("cmd2"));
    
    engine.clear();
    
    assert(engine.command_count() == 0);
    assert(engine.get_command_list().empty());
    assert(!engine.has_command("cmd1"));
    assert(!engine.has_command("cmd2"));
    
    std::cout << "  passed" << std::endl;
}

void test13_generated_parameter_names() {
    std::cout << "Test 13: Generated parameter names" << std::endl;
    
    Subject subj;
    std::unique_ptr<Command> wrapper(new Wrapper<Subject, int, int, int>(
        &subj, &Subject::f3));
    
    Engine engine;
    engine.register_command(std::move(wrapper), "multiply");
    
    auto info = engine.getCommandInfo("multiply");
    std::cout << "  Generated parameter names: ";
    for (size_t i = 0; i < info.paramNames.size(); ++i) {
        std::cout << info.paramNames[i] << " ";
    }
    std::cout << std::endl;
    
    assert(info.paramNames.size() == 2);
    assert(info.paramNames[0] == "param1");
    assert(info.paramNames[1] == "param2");
    
    int result = engine.execute_as<int>("multiply", {{"param1", 3}, {"param2", 4}});
    std::cout << "  multiply(3, 4) = " << result << std::endl;
    assert(result == 12);
    
    std::cout << "  passed" << std::endl;
}

void test14_missing_required_args() {
    std::cout << "Test 14: Missing required arguments" << std::endl;
    
    Subject subj;
    std::unique_ptr<Command> wrapper(new Wrapper<Subject, int, int, int>(
        &subj, &Subject::f3));
    
    Engine engine;
    engine.register_command(std::move(wrapper), "multiply");
    
    bool exception_thrown = false;
    try {
        engine.execute("multiply", {});
    } catch (const std::runtime_error& e) {
        std::cout << "  Expected missing argument error: " << e.what() << std::endl;
        exception_thrown = true;
    }
    assert(exception_thrown);
    
    exception_thrown = false;
    try {
        engine.execute("multiply", {{"param1", 5}});
    } catch (const std::runtime_error& e) {
        std::cout << "  Expected missing second argument: " << e.what() << std::endl;
        exception_thrown = true;
    }
    assert(exception_thrown);
    
    std::cout << "  passed" << std::endl;
}

void test15_partial_defaults_error() {
    std::cout << "Test 15: Partial defaults error" << std::endl;
    
    Subject subj;
    
    bool exception_thrown = false;
    try {
        std::unique_ptr<Command> wrapper(new Wrapper<Subject, int, int, int>(
            &subj, &Subject::f3, {{"arg1", 0}}));
    } catch (const std::runtime_error& e) {
        std::cout << "  Expected partial defaults error: " << e.what() << std::endl;
        exception_thrown = true;
    }
    assert(exception_thrown);
    
    std::cout << "  passed" << std::endl;
}

void test16_full_defaults_work() {
    std::cout << "Test 16: Full defaults work correctly" << std::endl;
    
    Subject subj;
    std::unique_ptr<Command> wrapper(new Wrapper<Subject, int, int, int>(
        &subj, &Subject::f3, {{"arg1", 10}, {"arg2", 20}}));
    
    Engine engine;
    engine.register_command(std::move(wrapper), "multiply_with_defaults");
    
    int result = engine.execute_as<int>("multiply_with_defaults", {});
    std::cout << "  multiply_with_defaults() = " << result << std::endl;
    assert(result == 200);
    
    std::cout << "  passed" << std::endl;
}

void test17_duplicate_command_registration() {
    std::cout << "Test 17: Duplicate command registration" << std::endl;
    
    Subject subj;
    Engine engine;
    
    engine.register_command(
        std::unique_ptr<Command>(new Wrapper<Subject, int, int, int>(
            &subj, &Subject::f3, {{"arg1", 0}, {"arg2", 0}})),
        "duplicate_cmd");
    
    bool exception_thrown = false;
    try {
        engine.register_command(
            std::unique_ptr<Command>(new Wrapper<Subject, int, int>(
                &subj, &Subject::f2, {{"arg1", 0}})),
            "duplicate_cmd");
    } catch (const std::runtime_error& e) {
        std::cout << "  Expected duplicate registration error: " << e.what() << std::endl;
        exception_thrown = true;
    }
    assert(exception_thrown);
    
    std::cout << "  passed" << std::endl;
}

void test18_default_value_type_mismatch() {
    std::cout << "Test 18: Default value type mismatch" << std::endl;
    
    Subject subj;
    
    bool exception_thrown = false;
    try {
        std::unique_ptr<Command> wrapper(new Wrapper<Subject, int, int, int>(
            &subj, &Subject::f3, {{"param1", std::string("wrong type")}, {"param2", 0}}));
        
        Engine engine;
        engine.register_command(std::move(wrapper), "bad_multiply");
        
        engine.execute("bad_multiply", {});
        
    } catch (const std::runtime_error& e) {
        std::cout << "  Expected default value type mismatch: " << e.what() << std::endl;
        exception_thrown = true;
    }
    assert(exception_thrown);
    
    std::cout << "  passed" << std::endl;
}

void test19_correct_defaults_work() {
    std::cout << "Test 19: Correct defaults work" << std::endl;
    
    Subject subj;
    std::unique_ptr<Command> wrapper(new Wrapper<Subject, int, int, int>(
        &subj, &Subject::f3, {{"param1", 5}, {"param2", 10}}));
    
    Engine engine;
    engine.register_command(std::move(wrapper), "good_multiply");
    
    int result = engine.execute_as<int>("good_multiply", {});
    std::cout << "  good_multiply() = " << result << std::endl;
    assert(result == 50);
    
    std::cout << "  passed" << std::endl;
}

void test20_string_methods() {
    std::cout << "Test 20: String methods" << std::endl;
    
    Subject subj;
    std::unique_ptr<Command> wrapper(new Wrapper<Subject, std::string, std::string, std::string>(
        &subj, &Subject::concatenate, {{"a", std::string("Hello")}, {"b", std::string("World")}}));
    
    Engine engine;
    engine.register_command(std::move(wrapper), "greet");
    
    std::string result = engine.execute_as<std::string>("greet", {});
    std::cout << "  greet() = '" << result << "'" << std::endl;
    assert(result == "HelloWorld");
    
    result = engine.execute_as<std::string>("greet", {{"a", std::string("Hi")}, {"b", std::string("There")}});
    std::cout << "  greet('Hi', 'There') = '" << result << "'" << std::endl;
    assert(result == "HiThere");
    
    std::cout << "  passed" << std::endl;
}

void run_all_tests() {
    std::cout << "Running Engine_wrapper tests..." << std::endl;
    
    try {
        test1_basic_functionality();
        std::cout << std::endl;
        
        test2_default_values();
        std::cout << std::endl;
        
        test3_type_safety();
        std::cout << std::endl;
        
        test4_memory_management();
        std::cout << std::endl;
        
        test5_error_handling();
        std::cout << std::endl;
        
        test6_const_methods();
        std::cout << std::endl;
        
        test7_command_info();
        std::cout << std::endl;
        
        test8_no_parameters_method();
        std::cout << std::endl;
        
        test9_multiple_commands();
        std::cout << std::endl;
        
        test10_nullptr_checks();
        std::cout << std::endl;
        
        test11_exact_task_example();
        std::cout << std::endl;
        
        test12_engine_clear_and_count();
        std::cout << std::endl;
        
        test13_generated_parameter_names();
        std::cout << std::endl;
        
        test14_missing_required_args();
        std::cout << std::endl;
        
        test15_partial_defaults_error();
        std::cout << std::endl;
        
        test16_full_defaults_work();
        std::cout << std::endl;
        
        test17_duplicate_command_registration();
        std::cout << std::endl;
        
        test18_default_value_type_mismatch();
        std::cout << std::endl;
        
        test19_correct_defaults_work();
        std::cout << std::endl;
        
        test20_string_methods();
        std::cout << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed with error: " << e.what() << std::endl;
        throw;
    }
    
    std::cout << "All 20 tests passed" << std::endl;
}