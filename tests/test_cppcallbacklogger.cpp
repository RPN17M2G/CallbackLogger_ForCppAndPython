#include "gtest/gtest.h"
#include <CallbackLogger.hpp>
#include <thread>
#include <atomic>
#include <set>
#include <unordered_map>
#include <chrono>
#include <fstream>
#include <vector>
#include <string>
#include <cstdio>

enum class TestComponent { A, B, C, D, E };

namespace {

ComponentEnumEntry make_entry(TestComponent c) {
    return make_component_entry(c);
}

std::string temp_log_file() {
    static int counter = 0;
    return "test_log_" + std::to_string(counter++) + ".txt";
}

}

TEST(CppCallbackLogger, TrivialRegisterAndLog) {
    CallbackLogger logger(0);
    std::vector<std::string> received;
    auto cb = [&](const LogEntry& entry) { received.push_back(entry.message); };
    auto handle = logger.register_function_callback(cb, Severity::Info);
    logger.log(Severity::Info, make_entry(TestComponent::A), "msg", "f.cpp", 1);
    ASSERT_EQ(received.size(), 1);
    ASSERT_EQ(received[0], "msg");
    logger.unregister_function_callback(handle);
}

TEST(CppCallbackLogger, RegisterFunctionCallbackSeverityVariant) {
    CallbackLogger logger(0);
    std::vector<std::string> received;
    auto cb = [&](const LogEntry& entry) { received.push_back(entry.message); };
    logger.register_function_callback(cb, Severity::Warning);
    logger.log(Severity::Info, make_entry(TestComponent::A), "should not appear", "f.cpp", 1);
    logger.log(Severity::Warning, make_entry(TestComponent::A), "should appear", "f.cpp", 2);
    ASSERT_EQ(received.size(), 1);
    ASSERT_EQ(received[0], "should appear");
}

TEST(CppCallbackLogger, RegisterFunctionCallbackComponentMap) {
    CallbackLogger logger(0);
    std::vector<std::string> received;
    std::unordered_map<ComponentEnumEntry, Severity, ComponentEnumEntryHasher> filter;
    filter[make_entry(TestComponent::A)] = Severity::Info;
    filter[make_entry(TestComponent::B)] = Severity::Error;
    auto cb = [&](const LogEntry& entry) { received.push_back(entry.message); };
    logger.register_function_callback(cb, filter);
    logger.log(Severity::Info, make_entry(TestComponent::A), "info A", "f.cpp", 1);
    logger.log(Severity::Error, make_entry(TestComponent::B), "error B", "f.cpp", 2);
    logger.log(Severity::Warning, make_entry(TestComponent::B), "warn B", "f.cpp", 3);
    ASSERT_EQ(received.size(), 2);
    ASSERT_EQ(received[0], "info A");
    ASSERT_EQ(received[1], "error B");
}

TEST(CppCallbackLogger, RegisterFunctionCallbackComponentSet) {
    CallbackLogger logger(0);
    std::vector<std::string> received;
    std::set<ComponentEnumEntry> filter = {make_entry(TestComponent::C)};
    auto cb = [&](const LogEntry& entry) { received.push_back(entry.message); };
    logger.register_function_callback(cb, filter);
    logger.log(Severity::Info, make_entry(TestComponent::C), "should appear", "f.cpp", 1);
    logger.log(Severity::Info, make_entry(TestComponent::A), "should not appear", "f.cpp", 2);
    ASSERT_EQ(received.size(), 1);
    ASSERT_EQ(received[0], "should appear");
}

TEST(CppCallbackLogger, RegisterFunctionCallbackSingleComponent) {
    CallbackLogger logger(0);
    std::vector<std::string> received;
    auto cb = [&](const LogEntry& entry) { received.push_back(entry.message); };
    logger.register_function_callback(cb, make_entry(TestComponent::D));
    logger.log(Severity::Info, make_entry(TestComponent::D), "should appear", "f.cpp", 1);
    logger.log(Severity::Info, make_entry(TestComponent::A), "should not appear", "f.cpp", 2);
    ASSERT_EQ(received.size(), 1);
    ASSERT_EQ(received[0], "should appear");
}

TEST(CppCallbackLogger, RegisterFunctionCallbackEmptyMap) {
    CallbackLogger logger(0);
    std::vector<std::string> received;
    std::unordered_map<ComponentEnumEntry, Severity, ComponentEnumEntryHasher> filter;
    auto cb = [&](const LogEntry& entry) { received.push_back(entry.message); };
    logger.register_function_callback(cb, filter);
    logger.log(Severity::Info, make_entry(TestComponent::A), "should appear", "f.cpp", 1);
    ASSERT_EQ(received.size(), 1);
}

TEST(CppCallbackLogger, RegisterFunctionCallbackUninitializedSeverity) {
    CallbackLogger logger(0);
    std::vector<std::string> received;
    std::unordered_map<ComponentEnumEntry, Severity, ComponentEnumEntryHasher> filter;
    filter[make_entry(TestComponent::A)] = Severity::Uninitialized;
    auto cb = [&](const LogEntry& entry) { received.push_back(entry.message); };
    logger.register_function_callback(cb, filter);
    logger.log(Severity::Debug, make_entry(TestComponent::A), "should appear", "f.cpp", 1);
    ASSERT_EQ(received.size(), 1);
}

TEST(CppCallbackLogger, RegisterFunctionCallbackWithLargeComponentEnum) {
    CallbackLogger logger(0);
    std::vector<std::string> received;
    for (int i = 0; i < 10; ++i) {
        logger.register_function_callback([&](const LogEntry& entry) { received.push_back(entry.message); }, make_entry(static_cast<TestComponent>(i % 5)));
    }
    logger.log(Severity::Info, make_entry(TestComponent::A), "msg", "f.cpp", 1);
    ASSERT_GE(received.size(), 2); // At least two callbacks for TestComponent::A
}

TEST(CppCallbackLogger, RegisterFunctionCallbackWithDuplicateRegistration) {
    CallbackLogger logger(0);
    std::vector<std::string> received;
    auto cb = [&](const LogEntry& entry) { received.push_back(entry.message); };
    auto h1 = logger.register_function_callback(cb, Severity::Info);
    auto h2 = logger.register_function_callback(cb, Severity::Info);
    logger.log(Severity::Info, make_entry(TestComponent::A), "msg", "f.cpp", 1);
    logger.unregister_function_callback(h1);
    logger.log(Severity::Info, make_entry(TestComponent::A), "msg2", "f.cpp", 2);
    ASSERT_EQ(std::count(received.begin(), received.end(), "msg"), 2);
    ASSERT_EQ(std::count(received.begin(), received.end(), "msg2"), 1);
}

TEST(CppCallbackLogger, RegisterFunctionCallbackWithInvalidHandleThrows) {
    CallbackLogger logger(0);
    ASSERT_THROW(logger.unregister_function_callback(999999), std::runtime_error);
}

TEST(CppCallbackLogger, RegisterFunctionCallbackWithLargeNumberOfCallbacks) {
    CallbackLogger logger(0);
    std::atomic<int> count{0};
    for (int i = 0; i < 100; ++i) {
        logger.register_function_callback([&](const LogEntry&) { count.fetch_add(1, std::memory_order_relaxed); }, Severity::Debug);
    }
    logger.log(Severity::Info, make_entry(TestComponent::A), "broadcast", "f.cpp", 1);
    ASSERT_EQ(count.load(), 100);
}

TEST(CppCallbackLogger, RegisterFunctionCallbackWithLargeNumberOfMessages) {
    CallbackLogger logger(0);
    std::vector<std::string> received;
    logger.register_function_callback([&](const LogEntry& entry) { received.push_back(entry.message); }, Severity::Debug);
    for (int i = 0; i < 1000; ++i) {
        logger.log(Severity::Info, make_entry(TestComponent::A), "msg" + std::to_string(i), "f.cpp", i);
    }
    ASSERT_EQ(received.size(), 1000);
}

TEST(CppCallbackLogger, RegisterFileCallbackSeverityAndComponent) {
    CallbackLogger logger(0);
    std::string tmpfile = temp_log_file();
    logger.register_file_callback(tmpfile, Severity::Error);
    logger.register_file_callback(tmpfile, make_entry(TestComponent::B));
    logger.log(Severity::Error, make_entry(TestComponent::B), "should appear", "f.cpp", 1);
    logger.log(Severity::Info, make_entry(TestComponent::B), "should appear", "f.cpp", 2);
    logger.log(Severity::Error, make_entry(TestComponent::A), "should appear", "f.cpp", 3);
    std::ifstream f(tmpfile);
    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    ASSERT_EQ(std::count(content.begin(), content.end(), '\n'), 4);
    std::remove(tmpfile.c_str());
}

TEST(CppCallbackLogger, RegisterFileCallbackWithInvalidPath) {
    CallbackLogger logger(0);
    std::string invalid_path = "/invalid/path/to/log.txt";
    ASSERT_NO_THROW(logger.register_file_callback(invalid_path, Severity::Info));
    ASSERT_NO_THROW(logger.log(Severity::Info, make_entry(TestComponent::A), "should not crash", "f.cpp", 1));
}

TEST(CppCallbackLogger, RegisterFunctionCallbackWithNoneMessage) {
    CallbackLogger logger(0);
    std::vector<std::string> received;
    logger.register_function_callback([&](const LogEntry& entry) { received.push_back(entry.message); }, Severity::Debug);
    logger.log(Severity::Info, make_entry(TestComponent::A), "", "f.cpp", 1);
    ASSERT_EQ(received[0], "");
}

TEST(CppCallbackLogger, RegisterFunctionCallbackWithMultipleSameComponentDifferentSeverity) {
    CallbackLogger logger(0);
    std::vector<std::pair<std::string, Severity>> received;
    logger.register_function_callback([&](const LogEntry& entry) { received.emplace_back("cb1", entry.severity); }, {{make_entry(TestComponent::A), Severity::Info}});
    logger.register_function_callback([&](const LogEntry& entry) { received.emplace_back("cb2", entry.severity); }, {{make_entry(TestComponent::A), Severity::Error}});
    logger.log(Severity::Info, make_entry(TestComponent::A), "info", "f.cpp", 1);
    logger.log(Severity::Error, make_entry(TestComponent::A), "error", "f.cpp", 2);
    ASSERT_TRUE(std::find(received.begin(), received.end(), std::make_pair(std::string("cb1"), Severity::Info)) != received.end());
    ASSERT_TRUE(std::find(received.begin(), received.end(), std::make_pair(std::string("cb2"), Severity::Error)) != received.end());
}

TEST(CppCallbackLogger, RegisterFunctionCallbackWithEmptySet) {
    CallbackLogger logger(0);
    std::vector<std::string> received;
    logger.register_function_callback([&](const LogEntry& entry) { received.push_back(entry.message); }, std::set<ComponentEnumEntry>{});
    logger.log(Severity::Info, make_entry(TestComponent::A), "should appear", "f.cpp", 1);
    ASSERT_EQ(received.size(), 1);
}

TEST(CppCallbackLogger, RegisterFunctionCallbackWithLargeFilterMap) {
    CallbackLogger logger(0);
    std::vector<std::string> received;
    std::unordered_map<ComponentEnumEntry, Severity, ComponentEnumEntryHasher> filter;
    for (int i = 0; i < 5; ++i)
        filter[make_entry(static_cast<TestComponent>(i))] = Severity::Info;
    logger.register_function_callback([&](const LogEntry& entry) { received.push_back(entry.message); }, filter);
    for (int i = 0; i < 5; ++i)
        logger.log(Severity::Info, make_entry(static_cast<TestComponent>(i)), "msg" + std::to_string(i), "f.cpp", i);
    ASSERT_EQ(received.size(), 5);
}

TEST(CppCallbackLogger, RegisterFileCallbackWithLargeNumberOfFiles) {
    CallbackLogger logger(0);
    std::vector<std::string> files;
    try {
        for (int i = 0; i < 10; ++i) {
            std::string fname = temp_log_file();
            files.push_back(fname);
            logger.register_file_callback(fname, Severity::Info);
        }
        for (int i = 0; i < 10; ++i) {
            logger.log(Severity::Info, make_entry(TestComponent::A), "filemsg" + std::to_string(i), "f.cpp", i);
        }
        for (int i = 0; i < 10; ++i) {
            std::ifstream f(files[i]);
            std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
            ASSERT_TRUE(content.find("filemsg" + std::to_string(i)) != std::string::npos);
        }
    } catch (...) {
        for (const auto& f : files) std::remove(f.c_str());
        throw;
    }
    for (const auto& f : files) std::remove(f.c_str());
}

TEST(CppCallbackLogger, RegisterFunctionCallbackWithLargeFilterMapAndManyLogs) {
    CallbackLogger logger(0);
    std::vector<std::string> received;
    std::unordered_map<ComponentEnumEntry, Severity, ComponentEnumEntryHasher> filter;
    for (int i = 0; i < 3; ++i)
        filter[make_entry(static_cast<TestComponent>(i))] = Severity::Info;
    logger.register_function_callback([&](const LogEntry& entry) { received.push_back(entry.message); }, filter);
    for (int i = 0; i < 100; ++i)
        logger.log(Severity::Info, make_entry(TestComponent::A), "msg" + std::to_string(i), "f.cpp", i);
    ASSERT_TRUE(std::find(received.begin(), received.end(), "msg0") != received.end());
}

TEST(CppCallbackLogger, RegisterFunctionCallbackConcurrentPerformance) {
    CallbackLogger logger(4);
    std::atomic<int> count{0};
    auto cb = [&](const LogEntry&) { count.fetch_add(1, std::memory_order_relaxed); };
    logger.register_function_callback(cb, Severity::Debug);

    auto log_many = [&]() {
        for (int i = 0; i < 1000; ++i)
            logger.log(Severity::Info, make_entry(TestComponent::A), "msg", "f.cpp", i);
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < 8; ++i)
        threads.emplace_back(log_many);
    for (auto& t : threads)
        t.join();

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    ASSERT_EQ(count.load(), 8000);
}

TEST(CppCallbackLogger, RegisterFunctionCallbackWithInvalidEnumValue) {
    CallbackLogger logger(0);
    std::vector<std::string> received;
    struct FakeComponent { int value = 99; };
    // This will not match any real component
    auto fake_entry = ComponentEnumEntry(std::type_index(typeid(FakeComponent)), 99);
    logger.register_function_callback([&](const LogEntry& entry) { received.push_back(entry.message); }, fake_entry);
    logger.log(Severity::Info, make_entry(TestComponent::A), "should not appear", "f.cpp", 1);
    ASSERT_TRUE(received.empty());
}

TEST(CppCallbackLogger, RegisterFunctionCallbackWithNoneFilter) {
    CallbackLogger logger(0);
    std::vector<std::string> received;
    // Simulate None filter by using empty map (matches all)
    logger.register_function_callback([&](const LogEntry& entry) { received.push_back(entry.message); }, std::unordered_map<ComponentEnumEntry, Severity, ComponentEnumEntryHasher>{});
    logger.log(Severity::Info, make_entry(TestComponent::A), "should appear", "f.cpp", 1);
    ASSERT_FALSE(received.empty());
}

TEST(CppCallbackLogger, RegisterFunctionCallbackWithEmptyMessageAndNone) {
    CallbackLogger logger(0);
    std::vector<std::string> received;
    logger.register_function_callback([&](const LogEntry& entry) { received.push_back(entry.message); }, Severity::Debug);
    logger.log(Severity::Info, make_entry(TestComponent::A), "", "f.cpp", 1);
    logger.log(Severity::Info, make_entry(TestComponent::A), std::string(), "f.cpp", 2);
    ASSERT_EQ(received[0], "");
    ASSERT_EQ(received[1], "");
}

TEST(CppCallbackLogger, RegisterFunctionCallbackWithMultipleFilters) {
    CallbackLogger logger(0);
    std::vector<std::string> received;
    logger.register_function_callback([&](const LogEntry& entry) { received.push_back(entry.message); }, Severity::Info);
    logger.register_function_callback([&](const LogEntry& entry) { received.push_back(entry.message); }, std::set<ComponentEnumEntry>{make_entry(TestComponent::A)});
    logger.log(Severity::Info, make_entry(TestComponent::A), "should appear", "f.cpp", 1);
    ASSERT_EQ(std::count(received.begin(), received.end(), "should appear"), 2);
}

TEST(CppCallbackLogger, ConcurrentRegisterAndLog) {
    CallbackLogger logger(2);
    std::atomic<int> count{0};
    auto cb = [&](const LogEntry&) { count.fetch_add(1, std::memory_order_relaxed); };

    std::vector<std::thread> threads;
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&]() {
            auto handle = logger.register_function_callback(cb, Severity::Info);
            for (int j = 0; j < 100; ++j) {
                logger.log(Severity::Info, make_entry(TestComponent::A), "msg", "f.cpp", j);
            }
            logger.unregister_function_callback(handle);
        });
    }
    for (auto& t : threads) t.join();
    // Each thread registers, logs 100 times, then unregisters
    ASSERT_GE(count.load(), 1000);
}

TEST(CppCallbackLogger, ConcurrentLoggingMultipleCallbacks) {
    CallbackLogger logger(4);
    std::atomic<int> count1{0}, count2{0};
    logger.register_function_callback([&](const LogEntry&) { count1.fetch_add(1, std::memory_order_relaxed); }, Severity::Info);
    logger.register_function_callback([&](const LogEntry&) { count2.fetch_add(1, std::memory_order_relaxed); }, Severity::Info);

    std::vector<std::thread> threads;
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < 200; ++j) {
                logger.log(Severity::Info, make_entry(TestComponent::B), "msg", "f.cpp", j);
            }
        });
    }
    for (auto& t : threads) t.join();
    // Each callback should receive all messages
    ASSERT_EQ(count1.load(), 1000);
    ASSERT_EQ(count2.load(), 1000);
}

// Trivial: Concurrent log with no callbacks
TEST(CppCallbackLogger, ConcurrentLogNoCallbacks) {
    CallbackLogger logger(2);
    std::vector<std::thread> threads;
    for (int i = 0; i < 8; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < 100; ++j)
                logger.log(Severity::Info, make_entry(TestComponent::A), "msg", "f.cpp", j);
        });
    }
    for (auto& t : threads) t.join();
    // Should not crash
    SUCCEED();
}

// Trivial: Concurrent register and unregister, no log
TEST(CppCallbackLogger, ConcurrentRegisterUnregisterNoLog) {
    CallbackLogger logger(2);
    std::atomic<int> handles{0};
    std::vector<std::thread> threads;
    for (int i = 0; i < 8; ++i) {
        threads.emplace_back([&]() {
            auto h = logger.register_function_callback([](const LogEntry&) {}, Severity::Info);
            handles.fetch_add(1, std::memory_order_relaxed);
            logger.unregister_function_callback(h);
        });
    }
    for (auto& t : threads) t.join();
    ASSERT_EQ(handles.load(), 8);
}

// Edge: Concurrent register, log, and unregister
TEST(CppCallbackLogger, ConcurrentRegisterLogUnregister) {
    CallbackLogger logger(2);
    std::atomic<int> count{0};
    std::vector<std::thread> threads;
    for (int i = 0; i < 8; ++i) {
        threads.emplace_back([&]() {
            auto h = logger.register_function_callback([&](const LogEntry&) { count.fetch_add(1, std::memory_order_relaxed); }, Severity::Info);
            for (int j = 0; j < 50; ++j)
                logger.log(Severity::Info, make_entry(TestComponent::A), "msg", "f.cpp", j);
            logger.unregister_function_callback(h);
        });
    }
    for (auto& t : threads) t.join();
    ASSERT_GE(count.load(), 400);
}

// Edge: Concurrent log with callbacks registered in other threads
TEST(CppCallbackLogger, ConcurrentLogWithAsyncRegister) {
    CallbackLogger logger(2);
    std::atomic<int> count{0};
    std::thread reg([&]() {
        for (int i = 0; i < 10; ++i) {
            auto h = logger.register_function_callback([&](const LogEntry&) { count.fetch_add(1, std::memory_order_relaxed); }, Severity::Info);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            logger.unregister_function_callback(h);
        }
    });
    std::vector<std::thread> loggers;
    for (int i = 0; i < 4; ++i) {
        loggers.emplace_back([&]() {
            for (int j = 0; j < 100; ++j)
                logger.log(Severity::Info, make_entry(TestComponent::A), "msg", "f.cpp", j);
        });
    }
    reg.join();
    for (auto& t : loggers) t.join();
    ASSERT_GE(count.load(), 0);
}

// Edge: Concurrent file callback registration and logging
TEST(CppCallbackLogger, ConcurrentFileCallbackRegisterAndLog) {
    CallbackLogger logger(2);
    std::string fname = temp_log_file();
    std::vector<std::thread> threads;
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([&]() {
            logger.register_file_callback(fname, Severity::Info);
            for (int j = 0; j < 50; ++j)
                logger.log(Severity::Info, make_entry(TestComponent::A), "msg", "f.cpp", j);
        });
    }
    for (auto& t : threads) t.join();
    std::ifstream f(fname);
    ASSERT_TRUE(f.good());
    f.close();
    std::remove(fname.c_str());
}

// Edge: Concurrent unregister with invalid handles
TEST(CppCallbackLogger, ConcurrentUnregisterInvalidHandles) {
    CallbackLogger logger(2);
    std::vector<std::thread> threads;
    for (int i = 0; i < 8; ++i) {
        threads.emplace_back([&]() {
            try {
                logger.unregister_function_callback(99999 + i);
            } catch (...) {}
        });
    }
    for (auto& t : threads) t.join();
    SUCCEED();
}

// Edge: Concurrent register same callback multiple times
TEST(CppCallbackLogger, ConcurrentRegisterSameCallback) {
    CallbackLogger logger(2);
    std::atomic<int> count{0};
    auto cb = [&](const LogEntry&) { count.fetch_add(1, std::memory_order_relaxed); };
    std::vector<std::thread> threads;
    for (int i = 0; i < 8; ++i) {
        threads.emplace_back([&]() {
            auto h = logger.register_function_callback(cb, Severity::Info);
            logger.log(Severity::Info, make_entry(TestComponent::A), "msg", "f.cpp", 1);
            logger.unregister_function_callback(h);
        });
    }
    for (auto& t : threads) t.join();
    logger.shutdown();
    ASSERT_EQ(count.load(), 8);
}

// Edge: Concurrent log with empty message
TEST(CppCallbackLogger, ConcurrentLogEmptyMessage) {
    CallbackLogger logger(2);
    std::atomic<int> count{0};
    logger.register_function_callback([&](const LogEntry& e) { if (e.message.empty()) count.fetch_add(1, std::memory_order_relaxed); }, Severity::Info);
    std::vector<std::thread> threads;
    for (int i = 0; i < 8; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < 10; ++j)
                logger.log(Severity::Info, make_entry(TestComponent::A), "", "f.cpp", j);
        });
    }
    for (auto& t : threads) t.join();
    ASSERT_EQ(count.load(), 80);
}

// Edge: Concurrent log with negative line numbers
TEST(CppCallbackLogger, ConcurrentLogNegativeLine) {
    CallbackLogger logger(2);
    std::atomic<int> count{0};
    logger.register_function_callback([&](const LogEntry& e) { if (e.line < 0) count.fetch_add(1, std::memory_order_relaxed); }, Severity::Info);
    std::vector<std::thread> threads;
    for (int i = 0; i < 8; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < 10; ++j)
                logger.log(Severity::Info, make_entry(TestComponent::A), "msg", "f.cpp", -j);
        });
    }
    for (auto& t : threads) t.join();
    ASSERT_EQ(count.load(), 80);
}

// Edge: Concurrent register with different filters
TEST(CppCallbackLogger, ConcurrentRegisterDifferentFilters) {
    CallbackLogger logger(2);
    std::atomic<int> count{0};
    std::vector<std::thread> threads;
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([&, i]() {
            auto h = logger.register_function_callback([&](const LogEntry&) { count.fetch_add(1, std::memory_order_relaxed); }, make_entry(static_cast<TestComponent>(i % 5)));
            logger.log(Severity::Info, make_entry(static_cast<TestComponent>(i % 5)), "msg", "f.cpp", i);
            logger.unregister_function_callback(h);
        });
    }
    for (auto& t : threads) t.join();
    ASSERT_EQ(count.load(), 5);
}

// Edge: Concurrent log with all severities
TEST(CppCallbackLogger, ConcurrentLogAllSeverities) {
    CallbackLogger logger(2);
    std::atomic<int> count{0};
    logger.register_function_callback([&](const LogEntry&) { count.fetch_add(1, std::memory_order_relaxed); }, Severity::Debug);
    std::vector<std::thread> threads;
    for (int s = (int)Severity::Debug; s <= (int)Severity::Error; ++s) {
        threads.emplace_back([&, s]() {
            for (int j = 0; j < 10; ++j)
                logger.log(static_cast<Severity>(s), make_entry(TestComponent::A), "msg", "f.cpp", j);
        });
    }
    for (auto& t : threads) t.join();
    ASSERT_EQ(count.load(), 50);
}

// Edge: Concurrent log with large messages
TEST(CppCallbackLogger, ConcurrentLogLargeMessages) {
    CallbackLogger logger(2);
    std::atomic<int> count{0};
    std::string big(10000, 'x');
    logger.register_function_callback([&](const LogEntry& e) { if (e.message.size() == 10000) count.fetch_add(1, std::memory_order_relaxed); }, Severity::Info);
    std::vector<std::thread> threads;
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < 10; ++j)
                logger.log(Severity::Info, make_entry(TestComponent::A), big, "f.cpp", j);
        });
    }
    for (auto& t : threads) t.join();
    ASSERT_EQ(count.load(), 40);
}

// Edge: Concurrent register and log with empty set/map filters
TEST(CppCallbackLogger, ConcurrentRegisterEmptyFilters) {
    CallbackLogger logger(2);
    std::atomic<int> count{0};
    std::vector<std::thread> threads;
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([&]() {
            auto h = logger.register_function_callback([&](const LogEntry&) { count.fetch_add(1, std::memory_order_relaxed); }, std::set<ComponentEnumEntry>{});
            logger.log(Severity::Info, make_entry(TestComponent::A), "msg", "f.cpp", i);
            logger.unregister_function_callback(h);
        });
    }
    for (auto& t : threads) t.join();
    ASSERT_EQ(count.load(), 4);
}

// Edge: Concurrent log with unmatched filters
TEST(CppCallbackLogger, ConcurrentLogUnmatchedFilters) {
    CallbackLogger logger(2);
    std::atomic<int> count{0};
    std::set<ComponentEnumEntry> filter = {make_entry(TestComponent::B)};
    logger.register_function_callback([&](const LogEntry&) { count.fetch_add(1, std::memory_order_relaxed); }, filter);
    std::vector<std::thread> threads;
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < 10; ++j)
                logger.log(Severity::Info, make_entry(TestComponent::A), "msg", "f.cpp", j);
        });
    }
    for (auto& t : threads) t.join();
    ASSERT_EQ(count.load(), 0);
}

// Edge: Concurrent register, log, and unregister with random order
TEST(CppCallbackLogger, ConcurrentRandomRegisterLogUnregister) {
    CallbackLogger logger(2);
    std::atomic<int> count{0};
    std::vector<std::thread> threads;
    for (int i = 0; i < 8; ++i) {
        threads.emplace_back([&]() {
            auto h = logger.register_function_callback([&](const LogEntry&) { count.fetch_add(1, std::memory_order_relaxed); }, Severity::Info);
            logger.log(Severity::Info, make_entry(TestComponent::A), "msg", "f.cpp", i);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            logger.unregister_function_callback(h);
        });
    }
    for (auto& t : threads) t.join();
    ASSERT_EQ(count.load(), 8);
}

// Edge: Concurrent file callback with invalid path
TEST(CppCallbackLogger, ConcurrentFileCallbackInvalidPath) {
    CallbackLogger logger(2);
    std::string fname = "/invalid/path/file.txt";
    std::vector<std::thread> threads;
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([&]() {
            logger.register_file_callback(fname, Severity::Info);
            logger.log(Severity::Info, make_entry(TestComponent::A), "msg", "f.cpp", i);
        });
    }
    for (auto& t : threads) t.join();
    SUCCEED();
}

// Edge: Concurrent register/unregister same handle
TEST(CppCallbackLogger, ConcurrentRegisterUnregisterSameHandle) {
    CallbackLogger logger(2);
    std::atomic<int> count{0};
    auto cb = [&](const LogEntry&) { count.fetch_add(1, std::memory_order_relaxed); };
    auto h = logger.register_function_callback(cb, Severity::Info);
    std::vector<std::thread> threads;
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([&]() {
            try { logger.unregister_function_callback(h); } catch (...) {}
        });
    }
    for (auto& t : threads) t.join();
    SUCCEED();
}

// Edge: Concurrent log with Severity::Uninitialized
TEST(CppCallbackLogger, ConcurrentLogUninitializedSeverity) {
    CallbackLogger logger(2);
    std::atomic<int> count{0};
    logger.register_function_callback([&](const LogEntry& e) { if (e.severity == Severity::Uninitialized) count.fetch_add(1, std::memory_order_relaxed); }, Severity::Uninitialized);
    std::vector<std::thread> threads;
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < 10; ++j)
                logger.log(Severity::Uninitialized, make_entry(TestComponent::A), "msg", "f.cpp", j);
        });
    }
    for (auto& t : threads) t.join();
    ASSERT_EQ(count.load(), 40);
}

// Edge: Concurrent register/unregister and log with large number of threads
TEST(CppCallbackLogger, ConcurrentManyThreadsRegisterUnregisterLog) {
    CallbackLogger logger(4);
    std::atomic<int> count{0};
    std::vector<std::thread> threads;
    for (int i = 0; i < 20; ++i) {
        threads.emplace_back([&]() {
            auto h = logger.register_function_callback([&](const LogEntry&) { count.fetch_add(1, std::memory_order_relaxed); }, Severity::Info);
            for (int j = 0; j < 5; ++j)
                logger.log(Severity::Info, make_entry(TestComponent::A), "msg", "f.cpp", j);
            logger.unregister_function_callback(h);
        });
    }
    for (auto& t : threads) t.join();
    ASSERT_EQ(count.load(), 100);
}
