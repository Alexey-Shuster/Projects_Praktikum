#pragma once

#include <string>

#include <boost/log/trivial.hpp>     // для BOOST_LOG_TRIVIAL
#include <boost/log/core.hpp>        // для logging::core
#include <boost/log/expressions.hpp> // для выражения, задающего фильтр
#include <boost/log/utility/setup/file.hpp> // boost::log::add_file_log
#include <boost/log/utility/setup/console.hpp>  // boost::log::add_console_log
#include <boost/log/utility/setup/common_attributes.hpp>    // for TimeStamp, ProcessID, ThreadID attributes
#include <boost/log/utility/manipulators/add_value.hpp>     // for boost::log::add_value
#include <boost/date_time.hpp>
#include <boost/json.hpp>

BOOST_LOG_ATTRIBUTE_KEYWORD(line_id, "LineID", unsigned int)
BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)
BOOST_LOG_ATTRIBUTE_KEYWORD(processID, "ProcessID", unsigned int)
BOOST_LOG_ATTRIBUTE_KEYWORD(threadID, "ThreadID", unsigned int)
// атрибут для дополнительных JSON-данных
BOOST_LOG_ATTRIBUTE_KEYWORD(additional_data, "AdditionalData", boost::json::value)

namespace boost_logger {

using namespace std::literals;
namespace attrs = boost::log::attributes;
namespace expr = boost::log::expressions;
namespace keywords = boost::log::keywords;
namespace logging = boost::log;
namespace sinks = boost::log::sinks;
namespace json = boost::json;

// Поля JSON объектов
namespace fields {
    // Общие поля
    constexpr const char* timestamp = "timestamp";
    constexpr const char* message = "message";
    constexpr const char* data = "data";
    constexpr const char* DEBUG = "DEBUG";

    // Поля для server started/exited
    constexpr const char* port = "port";
    constexpr const char* address = "address";
    constexpr const char* code = "code";
    constexpr const char* exception = "exception";

    // Поля для request/response
    constexpr const char* ip = "ip";
    constexpr const char* uri = "URI";
    constexpr const char* method = "method";
    constexpr const char* response_time = "response_time";
    constexpr const char* content_type = "content_type";

    // Поля для ошибок
    constexpr const char* text = "text";
    constexpr const char* where = "where";
}

// Типы сообщений (значения для поля "message")
namespace messages {
    constexpr const char* server_started = "Server has started";
    constexpr const char* server_exited = "Server exited";
    constexpr const char* request_received = "request received";
    constexpr const char* response_sent = "response sent";
    constexpr const char* error = "error";
    }

    // Значения для поля "where" в ошибках
    namespace error_locations {
    constexpr const char* read = "read";
    constexpr const char* write = "write";
    constexpr const char* accept = "accept";
    constexpr const char* close = "close";
    constexpr const char* handle_request = "handle_request";
}

void MyFormatter(logging::record_view const& rec, logging::formatting_ostream& strm);

void MyFormatterJSON(logging::record_view const& rec, logging::formatting_ostream& strm);

// Initialize Logger
void InitBoostLog();

// sets boost::log::severity_level >= severity_level (>= info by default)
void InitBoostLogSetFilter(const logging::trivial::severity_level &level = logging::trivial::info);

void SendBoostLogToStream();

void SendBoostLogToFile();

// Удобные обертки для логирования
void LogServerStarted(int port, const std::string& address);

void LogServerExited(int code = 0, const std::string& exception = "");

void LogRequestReceived(const std::string& ip,
                        const std::string& uri,
                        const std::string& method);

void LogResponseSent(int response_time,
                     int code,
                     const std::string& content_type = "");

void LogError(int code,
              const std::string& text,
              const std::string& where);

void LogDebug(const std::string& where,
              const std::string& text);

void LogInfo(const std::string& text);

} // namespace boost_logger

