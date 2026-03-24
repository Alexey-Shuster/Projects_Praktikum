cmake_minimum_required(VERSION 3.11)

set(MY_PROJECT_NAME "game_server")

project(${MY_PROJECT_NAME} CXX)
set(CMAKE_CXX_STANDARD 20)

if (WIN32)
	# Skip compiler verification step
	# Conan profile uses a "GCC trick" (compiler=gcc, but real CXX=clang)
	# this forces Boost build system (b2) to use the MinGW toolchain logic instead of falling back to MSVC's clang-cl
	if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
		set(CONAN_DISABLE_CHECK_COMPILER ON)
	endif ()
endif()

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

# Silence options applied before any target created
if(WIN32 AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
	# Silence warning from deprecated <ciso646> internal header in libpqxx
	if(TARGET CONAN_PKG::libpq)
		add_compile_options(-Wno-cpp)
	endif ()
endif()

#find_package(Boost REQUIRED COMPONENTS
#		log log_setup
#		system filesystem
#		program_options
#		serialization json
#)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(COMMON_LIB_NAME "Common_Lib")
# ===== Common library =====
add_library(${COMMON_LIB_NAME} STATIC
		src/common/sdk.h
		src/common/tagged.h
		src/common/constants.h
		src/common/game_utils/geometry.h
		src/common/game_utils/loot_generator.h
		src/common/game_utils/loot_generator.cpp
		src/common/game_utils/collision_detector.h
		src/common/game_utils/collision_detector.cpp
		src/common/boost_logger.h
		src/common/boost_logger.cpp
		src/common/json_loader.h
		src/common/json_loader.cpp
		src/common/utils.h
		src/common/utils.cpp
        src/common/ticker.h
		src/common/cmd_parser.h
		src/common/main_utils.h
)
target_link_libraries(${COMMON_LIB_NAME} PUBLIC
		CONAN_PKG::boost
		Threads::Threads
)

if(WIN32)
	target_compile_definitions(${COMMON_LIB_NAME} PUBLIC
			_WIN32_WINNT=0x0A00
			WINVER=0x0A00
			WIN32_LEAN_AND_MEAN
	)

	if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
		target_link_options(${COMMON_LIB_NAME} PUBLIC
				-static-libgcc
				-static-libstdc++
		)
	endif()

#	target_link_libraries(${COMMON_LIB_NAME} PUBLIC
#		bcrypt		# Boost UUID library requires Windows Bcrypt functions
#	)

	# Windows sockets
	target_link_libraries(${COMMON_LIB_NAME} PUBLIC
			ws2_32
			mswsock
	)

	# Debug builds disable many compiler optimizations, causing binary to bloat and expose linking issues
	if(CMAKE_BUILD_TYPE STREQUAL "Debug")
		target_link_libraries(${COMMON_LIB_NAME} PUBLIC
				synchronization
		)
	endif()
endif()

# ===== Game bots library =====
add_library(Game_Bots_Lib STATIC
		src/game_bots/bot_ai.h
		src/game_bots/bot_ai.cpp
		src/game_bots/bot_manager.h
		src/game_bots/bot_manager.cpp
)
target_link_libraries(Game_Bots_Lib PUBLIC ${COMMON_LIB_NAME})

# ===== Game model library =====
add_library(Game_Model_Lib STATIC
		src/game_model/game_model.h
		src/game_model/game_model.cpp
		src/game_model/road_engine/road_engine.h
		src/game_model/road_engine/road_engine.cpp
		src/game_model/road_engine/road_graph.h
		src/game_model/road_engine/road_graph.cpp
		src/game_model/game_map.h
		src/game_model/game_session.h
		src/game_model/game_session.cpp
		src/game_model/game_extra_data.h
		src/game_model/loot_storage.h
		src/game_model/loot_storage.cpp
		src/game_model/dog.h
		src/game_model/dog.cpp
		src/game_model/road.h
)
target_link_libraries(Game_Model_Lib PUBLIC Game_Bots_Lib)

# ===== Game database library =====
add_library(Game_DB_Lib STATIC
		src/game_db/dummy.cpp
		src/game_db/player_db.h
		src/game_db/repository_impls.h
		src/game_db/unit_of_work.h
		src/game_db/tagged_uuid.h
		src/game_db/connection_pool.h
		src/game_db/pooled_database.h
		src/game_db/db_migrations.h
		src/game_db/database_interface.h
		src/game_db/mock_database.h
		src/game_db/local_database.h
)
target_link_libraries(Game_DB_Lib PUBLIC
		Common_Lib
		CONAN_PKG::libpqxx
		CONAN_PKG::libpq
)

# ===== Game application library =====
add_library(Game_App_Lib STATIC
		src/game_app/application.h
		src/game_app/players.h
		src/game_app/players.cpp
		src/game_app/token.h
		src/game_app/game_clock.h
		src/game_app/game_state_persistence.h
		src/game_app/auto_save_manager.h
		src/game_app/player_score_recorder.h
)
target_link_libraries(Game_App_Lib PUBLIC
		Game_Model_Lib
		Game_DB_Lib
)

# ===== Game representation library =====
add_library(Game_Repr_Lib STATIC
		src/game_repr/game_session_repr.h
		src/game_repr/loot_repr.h
		src/game_repr/players_repr.h
		src/game_repr/game_repr.h
)
target_link_libraries(Game_Repr_Lib PUBLIC
		Game_App_Lib
)

# ===== HTTP server library =====
add_library(Http_Server_Lib STATIC
		src/http_server/http_server.h
		src/http_server/http_server.cpp
		src/http_server/request_handler.h
		src/http_server/request_handler.cpp
		src/http_server/http_response.h
		src/http_server/http_response.cpp
		src/http_server/logging_request_handler.h
		src/http_server/api_handler.h
		src/http_server/api_handler.cpp
		src/http_server/api_router.h
		src/http_server/api_router.cpp
		src/http_server/serialize_api.h
		src/http_server/serialize_api.cpp
)
target_link_libraries(Http_Server_Lib PUBLIC
		Game_App_Lib
)

# ===== Executables =====
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(${MY_PROJECT_NAME}
		src/main.cpp)
target_link_libraries(${MY_PROJECT_NAME} PRIVATE
		Http_Server_Lib
		Game_Repr_Lib
)

if (WIN32)
	# LLD (the LLVM linker) handles large Windows binaries significantly better than the default GNU ld
	target_link_options(${MY_PROJECT_NAME} PRIVATE
		"-fuse-ld=lld"
	)
endif ()



if(CMAKE_BUILD_TYPE MATCHES "Debug|RelWithDebInfo")
		add_executable(game_model_tests
				tests/game-model-tests.cpp
		)
		target_link_libraries(game_model_tests PRIVATE
				CONAN_PKG::catch2
				Game_Model_Lib
		)

		add_executable(loot_generator_tests
				tests/loot-generator-tests.cpp
		)
		target_link_libraries(loot_generator_tests PRIVATE
				CONAN_PKG::catch2
				Common_Lib)

		add_executable(collision_detection_tests
				tests/collision-detector-tests.cpp
		)
		target_link_libraries(collision_detection_tests PRIVATE
				CONAN_PKG::catch2
				Common_Lib)

		add_executable(state-serialization-tests
				tests/state-serialization-tests.cpp
		)
		target_link_libraries(state-serialization-tests PRIVATE
				CONAN_PKG::catch2
				Game_Repr_Lib)

		add_executable(database_tests_local
				tests/test_database.h
				tests/database_tests_local.cpp
		)
		target_link_libraries(database_tests_local PRIVATE
				CONAN_PKG::catch2
				Game_DB_Lib)
endif()

