# Создать образ на основе базового слоя gcc (там будет ОС и сам компилятор).
#===================================================== build
# 11.3 — используемая версия gcc.
# first container - build
FROM gcc:11.3 AS build

# Выполнить установку зависимостей внутри контейнера.
RUN apt update && \
    apt install -y \
      python3-pip \
      cmake \
    && \
    pip3 install conan==1.*

# копируем conanfile.txt в контейнер и запускаем conan install
COPY conanfile.txt /app/
RUN mkdir /app/build && cd /app/build && \
    conan install .. --build=missing -s build_type=Release -s compiler.libcxx=libstdc++11

# только после этого копируем остальные исходники
COPY ./src /app/src
COPY ./tests /app/tests
COPY CMakeLists.txt /app/

RUN cd /app/build && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && \
    cmake --build . -- -j 4 && \
    ls -R .

#===================================================== run
# Второй контейнер в том же докерфайле - run
# USER - переключается на выбранного пользователя
FROM ubuntu:22.04 AS run
RUN groupadd -r www && useradd -r -g www www
USER www

COPY --from=build /app/build/bin/game_server /app/
COPY ./data /app/data
COPY ./static /app/static
ENV GAME_DB_URL="Docker_Dummy"

# Запускаем игровой сервер
ENTRYPOINT ["/app/game_server"]
CMD ["--tick-period", "50", "--config-file", "/app/data/config.json", "--www-root", "/app/static"]