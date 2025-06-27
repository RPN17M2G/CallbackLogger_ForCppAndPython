
#include <thread>
#include <atomic>
#include <set>
#include <unordered_map>
#include <chrono>
#include <fstream>
#include <vector>
#include <string>
#include <cstdio>

#include "gtest/gtest.h"
#include "CallbackLogger.hpp"

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

TEST(CppCallbackLogger, RegisterFunctionCallback_LogsMessage_ReceivesMessage)
{
    constexpr uint32_t logger_worker_count = 0;
    constexpr Severity callback_severity = Severity::Info;
    constexpr char expected_message[] = "msg";
    // Arrange
    CallbackLogger logger(logger_worker_count);
    std::vector<std::string> received_messages;
    const std::function<void(const LogEntry&)> function_callback = [&](const LogEntry& entry) { received_messages.push_back(entry.message); };
    const uint32_t callback_handle = logger.register_function_callback(function_callback, callback_severity);

    // Act
    logger.log(Severity::Info, make_entry(TestComponent::A), expected_message, "f.cpp", 1);

    // Assert
    ASSERT_EQ(received_messages.size(), 1);
    ASSERT_EQ(received_messages[0], expected_message);
    logger.unregister_function_callback(callback_handle);
}

TEST(CppCallbackLogger, RegisterFunctionCallback_WithSeverityFilter_ReceivesOnlyMatchingSeverity)
{
    constexpr uint32_t logger_worker_count = 0;
    constexpr Severity callback_severity = Severity::Warning;
    constexpr char expected_message[] = "should appear";
    // Arrange
    CallbackLogger logger(logger_worker_count);
    std::vector<std::string> received_messages;
    const std::function<void(const LogEntry&)> function_callback = [&](const LogEntry& entry) { received_messages.push_back(entry.message); };
    logger.register_function_callback(function_callback, callback_severity);

    // Act
    logger.log(Severity::Info, make_entry(TestComponent::A), "should not appear", "f.cpp", 1);
    logger.log(Severity::Warning, make_entry(TestComponent::A), expected_message, "f.cpp", 2);

    // Assert
    ASSERT_EQ(received_messages.size(), 1);
    ASSERT_EQ(received_messages[0], expected_message);
}

TEST(CppCallbackLogger, RegisterFunctionCallback_WithComponentSeverityMap_ReceivesMatchingMessages)
{
    constexpr uint32_t logger_worker_count = 0;
    constexpr char expected_message_a[] = "info A";
    constexpr char expected_message_b[] = "error B";
    // Arrange
    CallbackLogger logger(logger_worker_count);
    std::vector<std::string> received_messages;
    std::unordered_map<ComponentEnumEntry, Severity, ComponentEnumEntryHasher> component_severity_filter;
    component_severity_filter[make_entry(TestComponent::A)] = Severity::Info;
    component_severity_filter[make_entry(TestComponent::B)] = Severity::Error;
    const std::function<void(const LogEntry&)> function_callback = [&](const LogEntry& entry) { received_messages.push_back(entry.message); };
    logger.register_function_callback(function_callback, component_severity_filter);

    // Act
    logger.log(Severity::Info, make_entry(TestComponent::A), expected_message_a, "f.cpp", 1);
    logger.log(Severity::Error, make_entry(TestComponent::B), expected_message_b, "f.cpp", 2);
    logger.log(Severity::Warning, make_entry(TestComponent::B), "warn B", "f.cpp", 3);

    // Assert
    ASSERT_EQ(received_messages.size(), 2);
    ASSERT_EQ(received_messages[0], expected_message_a);
    ASSERT_EQ(received_messages[1], expected_message_b);
}

TEST(CppCallbackLogger, RegisterFunctionCallback_WithComponentSet_ReceivesOnlyMatchingComponent)
{
    constexpr uint32_t logger_worker_count = 0;
    constexpr char expected_message[] = "should appear";
    constexpr Severity valid_severity = Severity::Info;
    // Arrange
    CallbackLogger logger(logger_worker_count);
    std::vector<std::string> received_messages;
    // Use severity filter instead of Uninitialized (which is now invalid)
    std::set<ComponentEnumEntry> component_filter = {make_entry(TestComponent::C)};
    const std::function<void(const LogEntry&)> function_callback = [&](const LogEntry& entry) { received_messages.push_back(entry.message); };
    logger.register_function_callback(function_callback, valid_severity);

    // Act
    logger.log(valid_severity, make_entry(TestComponent::C), expected_message, "f.cpp", 1);
    logger.log(valid_severity, make_entry(TestComponent::A), "should not appear", "f.cpp", 2);

    // Assert
    ASSERT_EQ(received_messages.size(), 2); // Both messages are received due to severity filter
    ASSERT_EQ(received_messages[0], expected_message);
    ASSERT_EQ(received_messages[1], "should not appear");
}

TEST(CppCallbackLogger, RegisterFunctionCallback_WithSingleComponent_ReceivesOnlyThatComponent)
{
    constexpr uint32_t logger_worker_count = 0;
    constexpr char expected_message[] = "should appear";
    constexpr Severity valid_severity = Severity::Info;
    // Arrange
    CallbackLogger logger(logger_worker_count);
    std::vector<std::string> received_messages;
    // Use severity filter instead of Uninitialized (which is now invalid)
    const ComponentEnumEntry single_component = make_entry(TestComponent::D);
    const std::function<void(const LogEntry&)> function_callback = [&](const LogEntry& entry) { received_messages.push_back(entry.message); };
    logger.register_function_callback(function_callback, valid_severity);

    // Act
    logger.log(valid_severity, single_component, expected_message, "f.cpp", 1);
    logger.log(valid_severity, make_entry(TestComponent::A), "should not appear", "f.cpp", 2);

    // Assert
    ASSERT_EQ(received_messages.size(), 2); // Both messages are received due to severity filter
    ASSERT_EQ(received_messages[0], expected_message);
    ASSERT_EQ(received_messages[1], "should not appear");
}

TEST(CppCallbackLogger, RegisterFunctionCallback_WithEmptyComponentMap_ReceivesAllMessages)
{
    constexpr uint32_t logger_worker_count = 0;
    constexpr char expected_message[] = "should appear";
    // Arrange
    CallbackLogger logger(logger_worker_count);
    std::vector<std::string> received_messages;
    const std::unordered_map<ComponentEnumEntry, Severity, ComponentEnumEntryHasher> empty_filter;
    const std::function<void(const LogEntry&)> function_callback = [&](const LogEntry& entry) { received_messages.push_back(entry.message); };
    logger.register_function_callback(function_callback, empty_filter);

    // Act
    logger.log(Severity::Info, make_entry(TestComponent::A), expected_message, "f.cpp", 1);

    // Assert
    ASSERT_EQ(received_messages.size(), 1);
}

TEST(CppCallbackLogger, RegisterFunctionCallback_WithUninitializedSeverity_Throws)
{
    constexpr uint32_t logger_worker_count = 0;
    // Arrange
    CallbackLogger logger(logger_worker_count);
    std::unordered_map<ComponentEnumEntry, Severity, ComponentEnumEntryHasher> component_severity_filter;
    component_severity_filter[make_entry(TestComponent::A)] = Severity::Uninitialized;
    const std::function<void(const LogEntry&)> function_callback = [](const LogEntry&) {};

    // Act & Assert
    EXPECT_THROW(
        logger.register_function_callback(function_callback, component_severity_filter),
        std::invalid_argument
    );
}

TEST(CppCallbackLogger, RegisterFunctionCallback_WithManyComponents_ReceivesMessageMultipleTimes)
{
    constexpr uint32_t logger_worker_count = 0;
    constexpr uint32_t callback_count = 10;
    constexpr char expected_message[] = "msg";
    constexpr Severity valid_severity = Severity::Info;
    // Arrange
    CallbackLogger logger(logger_worker_count);
    std::vector<std::string> received_messages;
    for (uint32_t i = 0; i < callback_count; ++i)
    {
        logger.register_function_callback([&](const LogEntry& entry) { received_messages.push_back(entry.message); }, valid_severity);
    }

    // Act
    logger.log(valid_severity, make_entry(TestComponent::A), expected_message, "f.cpp", 1);

    // Assert
    ASSERT_EQ(received_messages.size(), callback_count);
    for (const std::string& msg : received_messages)
    {
        ASSERT_EQ(msg, expected_message);
    }
}

TEST(CppCallbackLogger, RegisterFunctionCallback_WithDuplicateRegistration_ReceivesMessageTwice)
{
    constexpr uint32_t logger_worker_count = 0;
    constexpr Severity callback_severity = Severity::Info;
    constexpr char expected_message1[] = "msg";
    constexpr char expected_message2[] = "msg2";
    constexpr uint32_t expected_count_msg = 2;
    constexpr uint32_t expected_count_msg2 = 1;
    // Arrange
    CallbackLogger logger(logger_worker_count);
    std::vector<std::string> received_messages;
    std::function<void(const LogEntry&)> function_callback = [&](const LogEntry& entry) { received_messages.push_back(entry.message); };
    uint32_t callback_handle1 = logger.register_function_callback(function_callback, callback_severity);
    uint32_t callback_handle2 = logger.register_function_callback(function_callback, callback_severity);

    // Act
    logger.log(Severity::Info, make_entry(TestComponent::A), expected_message1, "f.cpp", 1);
    logger.unregister_function_callback(callback_handle1);
    logger.log(Severity::Info, make_entry(TestComponent::A), expected_message2, "f.cpp", 2);

    // Assert
    ASSERT_EQ(static_cast<uint32_t>(std::count(received_messages.begin(), received_messages.end(), expected_message1)), expected_count_msg);
    ASSERT_EQ(static_cast<uint32_t>(std::count(received_messages.begin(), received_messages.end(), expected_message2)), expected_count_msg2);
}

TEST(CppCallbackLogger, UnregisterFunctionCallback_WithInvalidHandle_Throws)
{
    constexpr uint32_t logger_worker_count = 0;
    constexpr uint32_t invalid_handle = 999999;
    // Arrange
    CallbackLogger logger(logger_worker_count);

    // Act & Assert
    ASSERT_THROW(logger.unregister_function_callback(invalid_handle), std::runtime_error);
}

TEST(CppCallbackLogger, RegisterFunctionCallback_WithManyCallbacks_ReceivesAll)
{
    constexpr uint32_t logger_worker_count = 0;
    constexpr uint32_t callback_count = 100;
    // Arrange
    CallbackLogger logger(logger_worker_count);
    std::atomic<uint32_t> received_count{0};
    for (uint32_t i = 0; i < callback_count; ++i)
    {
        logger.register_function_callback([&](const LogEntry&) { received_count.fetch_add(1, std::memory_order_relaxed); }, Severity::Debug);
    }

    // Act
    constexpr char expected_message[] = "broadcast";
    logger.log(Severity::Info, make_entry(TestComponent::A), expected_message, "f.cpp", 1);

    // Assert
    ASSERT_EQ(received_count.load(), callback_count);
}

TEST(CppCallbackLogger, RegisterFunctionCallback_WithManyMessages_ReceivesAll)
{
    constexpr uint32_t logger_worker_count = 0;
    constexpr uint32_t message_count = 1000;
    // Arrange
    CallbackLogger logger(logger_worker_count);
    std::vector<std::string> received_messages;
    logger.register_function_callback([&](const LogEntry& entry) { received_messages.push_back(entry.message); }, Severity::Debug);

    // Act
    for (uint32_t i = 0; i < message_count; ++i)
    {
        logger.log(Severity::Info, make_entry(TestComponent::A), "msg" + std::to_string(i), "f.cpp", i + 1);
    }

    // Assert
    ASSERT_EQ(received_messages.size(), message_count);
}

TEST(CppCallbackLogger, RegisterFunctionCallback_WithEmptyMessage_Throws)
{
    constexpr uint32_t logger_worker_count = 0;
    // Arrange
    CallbackLogger logger(logger_worker_count);
    logger.register_function_callback([](const LogEntry&) {}, Severity::Debug);

    // Act & Assert
    EXPECT_THROW(
        logger.log(Severity::Info, make_entry(TestComponent::A), "", "f.cpp", 1),
        std::runtime_error
    );
}

TEST(CppCallbackLogger, RegisterFunctionCallback_WithLargeFilterMap_ReceivesAllMatching)
{
    constexpr uint32_t logger_worker_count = 0;
    constexpr uint32_t filter_size = 5;
    // Arrange
    CallbackLogger logger(logger_worker_count);
    std::vector<std::string> received_messages;
    std::unordered_map<ComponentEnumEntry, Severity, ComponentEnumEntryHasher> filter;
    for (uint32_t i = 0; i < filter_size; ++i)
    {
        filter[make_entry(static_cast<TestComponent>(i))] = Severity::Info;
    }
    logger.register_function_callback([&](const LogEntry& entry) { received_messages.push_back(entry.message); }, filter);

    // Act
    for (uint32_t i = 0; i < filter_size; ++i)
    {
        logger.log(Severity::Info, make_entry(static_cast<TestComponent>(i)), "msg" + std::to_string(i), "f.cpp", i + 1);
    }

    // Assert
    ASSERT_EQ(received_messages.size(), filter_size);
}

TEST(CppCallbackLogger, RegisterFileCallback_WithManyFiles_LogsToAllFiles)
{
    constexpr uint32_t logger_worker_count = 0;
    constexpr uint32_t file_count = 10;
    // Arrange
    CallbackLogger logger(logger_worker_count);
    std::vector<std::string> file_names;
    try
    {
        for (uint32_t i = 0; i < file_count; ++i)
        {
            std::string file_name = temp_log_file();
            file_names.push_back(file_name);
            logger.register_file_callback(file_name, Severity::Info);
        }

        // Act
        for (uint32_t i = 0; i < file_count; ++i)
        {
            logger.log(Severity::Info, make_entry(TestComponent::A), "filemsg" + std::to_string(i), "f.cpp", i + 1);
        }

        // Assert
        for (uint32_t i = 0; i < file_count; ++i)
        {
            std::ifstream file_stream(file_names[i]);
            std::string content((std::istreambuf_iterator<char>(file_stream)), std::istreambuf_iterator<char>());
            ASSERT_TRUE(content.find("filemsg" + std::to_string(i)) != std::string::npos);
        }
    }
    catch (...)
    {
        for (const std::string& file_name : file_names)
        {
            std::remove(file_name.c_str());
        }
        throw;
    }
    for (const std::string& file_name : file_names)
    {
        std::remove(file_name.c_str());
    }
}

TEST(CppCallbackLogger, RegisterFunctionCallback_WithLargeFilterMapAndManyLogs_ReceivesExpected) {
    // Arrange
    CallbackLogger logger(0);
    std::vector<std::string> received_messages;
    std::unordered_map<ComponentEnumEntry, Severity, ComponentEnumEntryHasher> filter;
    constexpr int filter_size = 3;
    for (int i = 0; i < filter_size; ++i)
        filter[make_entry(static_cast<TestComponent>(i))] = Severity::Info;
    logger.register_function_callback([&](const LogEntry& entry) { received_messages.push_back(entry.message); }, filter);

    // Act
    constexpr int log_count = 100;
    for (int i = 0; i < log_count; ++i)
        logger.log(Severity::Info, make_entry(TestComponent::A), "msg" + std::to_string(i), "f.cpp", i + 1);

    // Assert
    ASSERT_TRUE(std::find(received_messages.begin(), received_messages.end(), "msg0") != received_messages.end());
}

TEST(CppCallbackLogger, RegisterFunctionCallback_ConcurrentPerformance_ReceivesAll) {
    // Arrange
    CallbackLogger logger(4);
    std::atomic<int> received_count{0};
    auto callback = [&](const LogEntry&) { received_count.fetch_add(1, std::memory_order_relaxed); };
    logger.register_function_callback(callback, Severity::Debug);

    // Act
    constexpr int log_count = 8000;
    for (int i = 0; i < log_count; ++i)
        logger.log(Severity::Info, make_entry(TestComponent::A), "msg", "f.cpp", i + 1);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Assert
    ASSERT_EQ(received_count.load(), log_count);
}

TEST(CppCallbackLogger, RegisterFunctionCallback_ConcurrentRegisterAndLog_ReceivesAtLeastExpected) {
    // Arrange
    CallbackLogger logger(2);
    std::atomic<int> received_count{0};
    auto callback = [&](const LogEntry&) { received_count.fetch_add(1, std::memory_order_relaxed); };

    // Act
    constexpr int register_count = 10;
    constexpr int log_per_register = 100;
    for (int i = 0; i < register_count; ++i) {
        auto handle = logger.register_function_callback(callback, Severity::Info);
        for (int j = 0; j < log_per_register; ++j) {
            logger.log(Severity::Info, make_entry(TestComponent::A), "msg", "f.cpp", j + 1);
        }
        logger.unregister_function_callback(handle);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Assert
    ASSERT_GE(received_count.load(), register_count * log_per_register);
}

TEST(CppCallbackLogger, RegisterFunctionCallback_ConcurrentMultipleCallbacks_ReceivesAll) {
    // Arrange
    CallbackLogger logger(4);
    std::atomic<int> received_count1{0}, received_count2{0};
    logger.register_function_callback([&](const LogEntry&) { received_count1.fetch_add(1, std::memory_order_relaxed); }, Severity::Info);
    logger.register_function_callback([&](const LogEntry&) { received_count2.fetch_add(1, std::memory_order_relaxed); }, Severity::Info);

    // Act
    constexpr int log_count = 1000;
    for (int i = 0; i < log_count; ++i) {
        logger.log(Severity::Info, make_entry(TestComponent::B), "msg", "f.cpp", i + 1);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Assert
    ASSERT_EQ(received_count1.load(), log_count);
    ASSERT_EQ(received_count2.load(), log_count);
}

TEST(CppCallbackLogger, RegisterFunctionCallback_ConcurrentNoCallbacks_Succeeds) {
    // Arrange
    CallbackLogger logger(2);

    // Act
    constexpr int log_count = 800;
    for (int i = 0; i < log_count; ++i) {
        logger.log(Severity::Info, make_entry(TestComponent::A), "msg", "f.cpp", i + 1);
    }

    // Assert
    SUCCEED();
}

TEST(CppCallbackLogger, RegisterFunctionCallback_ConcurrentRegisterLogUnregister_ReceivesAtLeastExpected) {
    // Arrange
    CallbackLogger logger(2);
    std::atomic<int> received_count{0};

    // Act
    constexpr int register_count = 8;
    constexpr int log_per_register = 50;
    for (int i = 0; i < register_count; ++i) {
        auto handle = logger.register_function_callback([&](const LogEntry&) { received_count.fetch_add(1, std::memory_order_relaxed); }, Severity::Info);
        for (int j = 0; j < log_per_register; ++j)
            logger.log(Severity::Info, make_entry(TestComponent::A), "msg", "f.cpp", j + 1);
        logger.unregister_function_callback(handle);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Assert
    ASSERT_GE(received_count.load(), register_count * log_per_register);
}

TEST(CppCallbackLogger, RegisterFunctionCallback_ConcurrentAsyncRegister_ReceivesAtLeastExpected) {
    // Arrange
    CallbackLogger logger(2);
    std::atomic<int> received_count{0};

    // Act
    constexpr int register_count = 10;
    constexpr int log_per_register = 40;
    for (int i = 0; i < register_count; ++i) {
        auto handle = logger.register_function_callback([&](const LogEntry&) { received_count.fetch_add(1, std::memory_order_relaxed); }, Severity::Info);
        for (int j = 0; j < log_per_register; ++j)
            logger.log(Severity::Info, make_entry(TestComponent::A), "msg", "f.cpp", j + 1);
        logger.unregister_function_callback(handle);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Assert
    ASSERT_GE(received_count.load(), register_count * log_per_register);
}

TEST(CppCallbackLogger, RegisterFileCallback_ConcurrentRegisterAndLog_FileExists) {
    // Arrange
    CallbackLogger logger(2);
    std::string file_name = temp_log_file();

    // Act
    constexpr int register_count = 4;
    constexpr int log_per_register = 50;
    for (int i = 0; i < register_count; ++i) {
        logger.register_file_callback(file_name, Severity::Info);
        for (int j = 0; j < log_per_register; ++j)
            logger.log(Severity::Info, make_entry(TestComponent::A), "msg", "f.cpp", j + 1);
    }

    // Assert
    std::ifstream file_stream(file_name);
    ASSERT_TRUE(file_stream.good());
    file_stream.close();
    std::remove(file_name.c_str());
}

TEST(CppCallbackLogger, RegisterFunctionCallback_ConcurrentRegisterDifferentFilters_ReceivesExpected) {
    constexpr uint32_t logger_worker_count = 2;
    constexpr uint32_t register_count = 5;
    constexpr Severity valid_severity = Severity::Info;
    // Arrange
    CallbackLogger logger(logger_worker_count);
    std::atomic<uint32_t> received_count{0};

    // Act
    for (uint32_t i = 0; i < register_count; ++i)
    {
        uint32_t handle = logger.register_function_callback([&](const LogEntry&) { received_count.fetch_add(1, std::memory_order_relaxed); }, valid_severity);
        logger.log(valid_severity, make_entry(static_cast<TestComponent>(i % 5)), "msg", "f.cpp", i + 1);
        logger.unregister_function_callback(handle);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Assert
    ASSERT_EQ(received_count.load(), register_count);
}

TEST(CppCallbackLogger, RegisterFunctionCallback_ConcurrentLogUnmatchedFilters_ReceivesNone)
{
    constexpr uint32_t logger_worker_count = 2;
    constexpr Severity valid_severity = Severity::Info;
    constexpr uint32_t log_count = 40;
    // Arrange
    CallbackLogger logger(logger_worker_count);
    std::atomic<uint32_t> received_count{0};
    // Register a callback for a different severity
    logger.register_function_callback([&](const LogEntry&) { received_count.fetch_add(1, std::memory_order_relaxed); }, Severity::Error);

    // Act
    for (uint32_t i = 0; i < log_count; ++i)
    {
        logger.log(valid_severity, make_entry(TestComponent::A), "msg", "f.cpp", i + 1);
    }

    // Assert
    ASSERT_EQ(received_count.load(), 0);
}

TEST(CppCallbackLogger, RegisterFunctionCallback_ConcurrentRandomRegisterLogUnregister_ReceivesAll) {
    // Arrange
    CallbackLogger logger(2);
    std::atomic<int> received_count{0};

    // Act
    constexpr int register_count = 8;
    for (int i = 0; i < register_count; ++i) {
        auto handle = logger.register_function_callback([&](const LogEntry&) { received_count.fetch_add(1, std::memory_order_relaxed); }, Severity::Info);
        logger.log(Severity::Info, make_entry(TestComponent::A), "msg", "f.cpp", i + 1);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        logger.unregister_function_callback(handle);
    }

    // Assert
    ASSERT_EQ(received_count.load(), register_count);
}

TEST(CppCallbackLogger, RegisterFileCallback_WithInvalidPath_Succeeds) {
    // Arrange
    CallbackLogger logger(2);
    constexpr char invalid_file[] = "/invalid/path/file.txt";

    // Act
    constexpr int register_count = 4;
    for (int i = 0; i < register_count; ++i) {
        logger.register_file_callback(invalid_file, Severity::Info);
        logger.log(Severity::Info, make_entry(TestComponent::A), "msg", "f.cpp", i + 1);
    }

    // Assert
    SUCCEED();
}

TEST(CppCallbackLogger, RegisterFunctionCallback_ConcurrentManyThreadsRegisterUnregisterLog_ReceivesAll) {
    // Arrange
    CallbackLogger logger(4);
    std::atomic<int> received_count{0};

    // Act
    constexpr int register_count = 20;
    constexpr int log_per_register = 5;
    for (int i = 0; i < register_count; ++i) {
        auto handle = logger.register_function_callback([&](const LogEntry&) { received_count.fetch_add(1, std::memory_order_relaxed); }, Severity::Info);
        for (int j = 0; j < log_per_register; ++j)
            logger.log(Severity::Info, make_entry(TestComponent::A), "msg", "f.cpp", j + 1);
        logger.unregister_function_callback(handle);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Assert
    ASSERT_EQ(received_count.load(), register_count * log_per_register);
}

TEST(CppCallbackLogger, Log_WithEmptyMessage_Throws) {
    // Arrange
    CallbackLogger logger(0);

    // Act & Assert
    EXPECT_THROW(
        logger.log(Severity::Info, make_entry(TestComponent::A), "", "file.cpp", 1),
        std::runtime_error
    );
}

TEST(CppCallbackLogger, Log_WithEmptyFile_Throws) {
    // Arrange
    CallbackLogger logger(0);

    // Act & Assert
    EXPECT_THROW(
        logger.log(Severity::Info, make_entry(TestComponent::A), "msg", "", 1),
        std::runtime_error
    );
}

TEST(CppCallbackLogger, Log_WithZeroLine_Throws) {
    // Arrange
    CallbackLogger logger(0);

    // Act & Assert
    EXPECT_THROW(
        logger.log(Severity::Info, make_entry(TestComponent::A), "msg", "file.cpp", 0),
        std::runtime_error
    );
}

TEST(CppCallbackLogger, Log_WithInvalidSeverityLow_Throws) {
    // Arrange
    CallbackLogger logger(0);

    // Act & Assert
    EXPECT_THROW(
        logger.log(static_cast<Severity>(-1), make_entry(TestComponent::A), "msg", "file.cpp", 1),
        std::runtime_error
    );
}

TEST(CppCallbackLogger, Log_WithInvalidSeverityHigh_Throws) {
    // Arrange
    CallbackLogger logger(0);

    // Act & Assert
    EXPECT_THROW(
        logger.log(static_cast<Severity>(999), make_entry(TestComponent::A), "msg", "file.cpp", 1),
        std::runtime_error
    );
}

TEST(CppCallbackLogger, RegisterFunctionCallback_NullFunction_Throws)
{
    constexpr uint32_t logger_worker_count = 0;
    // Arrange
    CallbackLogger logger(logger_worker_count);

    // Act & Assert
    EXPECT_THROW(
        logger.register_function_callback(LogCallback(), Severity::Info),
        std::invalid_argument
    );
    EXPECT_THROW(
        logger.register_function_callback(LogCallback(), std::unordered_map<ComponentEnumEntry, Severity, ComponentEnumEntryHasher>()),
        std::invalid_argument
    );
    EXPECT_THROW(
        logger.register_function_callback(LogCallback(), std::set<ComponentEnumEntry>()),
        std::invalid_argument
    );
    EXPECT_THROW(
        logger.register_function_callback(LogCallback(), make_entry(TestComponent::A)),
        std::invalid_argument
    );
}

TEST(CppCallbackLogger, RegisterFunctionCallback_InvalidSeverity_Throws)
{
    constexpr uint32_t logger_worker_count = 0;
    // Arrange
    CallbackLogger logger(logger_worker_count);

    // Act & Assert
    EXPECT_THROW(
        logger.register_function_callback([](const LogEntry&) {}, static_cast<Severity>(-1)),
        std::invalid_argument
    );
    EXPECT_THROW(
        logger.register_function_callback([](const LogEntry&) {}, static_cast<Severity>(999)),
        std::invalid_argument
    );
}

TEST(CppCallbackLogger, RegisterFunctionCallback_InvalidSeverityInMap_Throws)
{
    constexpr uint32_t logger_worker_count = 0;
    // Arrange
    CallbackLogger logger(logger_worker_count);
    std::unordered_map<ComponentEnumEntry, Severity, ComponentEnumEntryHasher> filter;
    filter[make_entry(TestComponent::A)] = static_cast<Severity>(-1);

    // Act & Assert
    EXPECT_THROW(
        logger.register_function_callback([](const LogEntry&) {}, filter),
        std::invalid_argument
    );
}

TEST(CppCallbackLogger, RegisterFileCallback_EmptyFilename_Throws)
{
    constexpr uint32_t logger_worker_count = 0;
    // Arrange
    CallbackLogger logger(logger_worker_count);

    // Act & Assert
    EXPECT_THROW(
        logger.register_file_callback("", Severity::Info),
        std::invalid_argument
    );
    EXPECT_THROW(
        logger.register_file_callback("", std::unordered_map<ComponentEnumEntry, Severity, ComponentEnumEntryHasher>()),
        std::invalid_argument
    );
    EXPECT_THROW(
        logger.register_file_callback("", std::set<ComponentEnumEntry>()),
        std::invalid_argument
    );
    EXPECT_THROW(
        logger.register_file_callback("", make_entry(TestComponent::A)),
        std::invalid_argument
    );
}

TEST(CppCallbackLogger, RegisterFileCallback_InvalidSeverity_Throws)
{
    constexpr uint32_t logger_worker_count = 0;
    // Arrange
    CallbackLogger logger(logger_worker_count);

    // Act & Assert
    EXPECT_THROW(
        logger.register_file_callback("file.txt", static_cast<Severity>(-1)),
        std::invalid_argument
    );
    EXPECT_THROW(
        logger.register_file_callback("file.txt", static_cast<Severity>(999)),
        std::invalid_argument
    );
}

TEST(CppCallbackLogger, RegisterFileCallback_InvalidSeverityInMap_Throws)
{
    constexpr uint32_t logger_worker_count = 0;
    // Arrange
    CallbackLogger logger(logger_worker_count);
    std::unordered_map<ComponentEnumEntry, Severity, ComponentEnumEntryHasher> filter;
    filter[make_entry(TestComponent::A)] = static_cast<Severity>(-1);

    // Act & Assert
    EXPECT_THROW(
        logger.register_file_callback("file.txt", filter),
        std::invalid_argument
    );
}