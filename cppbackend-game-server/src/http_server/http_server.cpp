#include <iostream>

#include "http_server.h"

namespace http_server {

using namespace std::literals;

void ReportError(beast::error_code ec, std::string_view what) {
    std::cerr << what << ": "sv << ec.message() << std::endl;
}

void SessionBase::Run() {
    // Воспользуемся функцией boost::asio::dispatch, чтобы вызвать метод SessionBase::Read
    // внутри strand сокета. Не забудем захватить std::shared_ptr на текущий объект.
    // Вызываем метод Read, используя executor объекта stream_.
    // Таким образом вся работа со stream_ будет выполняться, используя его executor
    net::dispatch(stream_.get_executor(),
                  beast::bind_front_handler(&SessionBase::Read, GetSharedThis()));
}

void SessionBase::Read() {
    // Очищаем запрос от прежнего значения (метод Read может быть вызван несколько раз)
    request_ = {};
    stream_.expires_after(30s);
    // Считываем request_ из stream_, используя buffer_ для хранения считанных данных
    http::async_read(stream_, buffer_, request_,
                     // По окончании операции будет вызван метод OnRead
                     beast::bind_front_handler(&SessionBase::OnRead, GetSharedThis()));
}

void SessionBase::OnRead(beast::error_code ec, std::size_t bytes_read) {
    if (ec == http::error::end_of_stream) {
        // Нормальная ситуация - клиент закрыл соединение
        return Close();
    }
    if (ec) {
        // return ReportError(ec, "read"sv);
        boost_logger::LogError(EXIT_FAILURE, ec.message(), boost_logger::error_locations::read);
        return;
    }
    HandleRequest(std::move(request_));
}

void SessionBase::OnWrite(bool close, beast::error_code ec, std::size_t bytes_written) {
    if (ec) {
        // return ReportError(ec, "write"sv);
        boost_logger::LogError(EXIT_FAILURE, ec.message(), boost_logger::error_locations::write);
        return;
    }

    if (close) {
        // Семантика ответа требует закрыть соединение
        return Close();
    }

    // Считываем следующий запрос
    Read();
}

void SessionBase::Close() {
    beast::error_code ec;
    stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
    if (ec) {
        // return ReportError(ec, "SessionBase::Close"sv);
        boost_logger::LogError(EXIT_FAILURE, ec.message(), boost_logger::error_locations::close);
        return;
    }
}

}  // namespace http_server
