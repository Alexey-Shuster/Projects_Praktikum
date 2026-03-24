#include "boost_logger.h"

#include "constants.h"

namespace boost_logger {

void InitBoostLog() {
    // logging::trivial::logger::get();
}

void InitBoostLogSetFilter(const boost::log::trivial::severity_level &level) {
    // This adds TimeStamp, ProcessID, ThreadID attributes
    logging::add_common_attributes();

    logging::core::get()->set_filter(
        logging::trivial::severity >= level
        );
}

void MyFormatter(const boost::log::record_view &rec, boost::log::formatting_ostream &strm) {
    strm << rec[line_id] << ": ";
    auto ts = rec[timestamp];
    strm << to_iso_extended_string(*ts) << ": ";

    // Выводим уровень, заключая его в угловые скобки
    strm << "<" << rec[logging::trivial::severity] << "> ";
    // Выводим само сообщение
    strm << rec[expr::smessage];
}

void MyFormatterJSON(const boost::log::record_view &rec, boost::log::formatting_ostream &strm) {
    json::object log_entry;

    // Добавляем timestamp
    if (auto ts = rec[timestamp]) {
        log_entry[fields::timestamp] = to_iso_extended_string(*ts);
    }

    // Добавляем data (additional_data), если есть
    if (auto data_attr = rec[additional_data]) {
        log_entry[fields::data] = *data_attr;
    } else {
        // Если данных нет, создаем пустой объект
        log_entry[fields::data] = json::object{};
    }

    // Добавляем message
    if (auto msg = rec[expr::smessage]) {
        log_entry[fields::message] = msg.get<std::string>();
    }

    // Выводим JSON в поток
    strm << json::serialize(json::value(std::move(log_entry)));
}

void SendBoostLogToStream() {
    // This adds TimeStamp, ProcessID, ThreadID attributes
    logging::add_common_attributes();

    logging::add_console_log(
        std::cout,
        keywords::format = &MyFormatterJSON,
        keywords::auto_flush = true
        );
}

void SendBoostLogToFile() {
    // This adds TimeStamp, ProcessID, ThreadID attributes
    logging::add_common_attributes();

    logging::add_file_log(
        keywords::file_name = "sample_%N.log",
        keywords::format = &MyFormatter,
        // ротируем по достижению размера 10 мегабайт
        keywords::rotation_size = 10 * 1024 * 1024,
        // ротируем ежедневно в полдень
        keywords::time_based_rotation = sinks::file::rotation_at_time_point(12, 0, 0)
        );
}

void LogServerStarted(int port, const std::string &address) {
    BOOST_LOG_TRIVIAL(info)
    << logging::add_value(additional_data,
                          json::object{
                              {fields::port, port},
                              {fields::address, address}
                          })
    << messages::server_started;
}

void LogServerExited(int code, const std::string &exception) {
    json::object data{{fields::code, code}};
    if (!exception.empty()) {
        data[fields::exception] = exception;
    }

    auto severity = (code == 0) ? logging::trivial::info : logging::trivial::error;
    BOOST_LOG_SEV(logging::trivial::logger::get(), severity)
        << logging::add_value(additional_data, std::move(data))
        << messages::server_exited;
}

void LogRequestReceived(const std::string &ip, const std::string &uri, const std::string &method) {
    BOOST_LOG_TRIVIAL(info)
    << logging::add_value(additional_data,
                          json::object{
                              {fields::ip, ip},
                              {fields::uri, uri},
                              {fields::method, method}
                          })
    << messages::request_received;
}

void LogResponseSent(int response_time, int code, const std::string &content_type) {
    json::object data{
        {fields::response_time, response_time},
        {fields::code, code}
    };

    if (!content_type.empty()) {
        data[fields::content_type] = content_type;
    } else {
        data[fields::content_type] = nullptr;
    }

    BOOST_LOG_TRIVIAL(info)
        << logging::add_value(additional_data, std::move(data))
        << messages::response_sent;
}

void LogError(int code, const std::string &text, const std::string &where) {
    BOOST_LOG_TRIVIAL(error)
    << logging::add_value(additional_data,
                          json::object{
                              {fields::code, code},
                              {fields::text, text},
                              {fields::where, where}
                          })
    << messages::error;
}

void LogDebug(const std::string &where, const std::string &text) {
    BOOST_LOG_TRIVIAL(debug)
    << logging::add_value(additional_data,
                          json::object{
                              {fields::DEBUG, json::object{{fields::method, where}}}
                          })
    << text;
}

void LogInfo(const std::string &text) {
    BOOST_LOG_TRIVIAL(info)
    << logging::add_value(additional_data,
                          json::object{
                              {fields::message, json_fields::INFO_LOG}
                          })
    << text;
}

} // namespace boost_logger
