#include <iostream>
#include <fstream>

#include "request_handler.h"
#include "../common/utils.h"

namespace http_handler {

/**
 * @brief Debug helper to print request details
 *
 * Output format:
 * GET /api/v1/maps
 *   Host: localhost:8080
 *   User-Agent: curl/7.68.0
 *   Accept: *//*
 */
void RequestHandler::DumpRequest(const StringRequest &req) const {
    std::cout << req.method_string() << ' ' << req.target() << std::endl;
    for (const auto& header : req) {
        std::cout << "  "sv << header.name_string() << ": "sv << header.value() << std::endl;
    }
}

/**
 * @brief Serve static files from the filesystem
 *
 * Process:
 * 1. Remove leading slash from request target
 * 2. URL-decode the path (convert %20 to space, etc.)
 * 3. Build absolute path by joining with www_root
 * 4. Normalize path (remove . and .., resolve symlinks)
 * 5. If path points to directory, append index.html
 * 6. Verify the final path is still within www_root (security)
 * 7. Check if file exists and is readable
 * 8. Read entire file and send with correct MIME type
 *
 * Security considerations:
 * - Uses weakly_canonical to resolve symlinks and normalize paths
 * - Explicitly checks that resolved path is within www_root
 * - Rejects paths that escape the root directory (path traversal attacks)
 * - Validates file existence and type before reading
 *
 * @param req The HTTP request
 * @return StringResponse containing file contents or error
 */
StringResponse RequestHandler::HandleFileRequest(const StringRequest &req) const {
    std::string target = std::string(req.target());

    // Step 1: Remove leading slash
    if (!target.empty() && target[0] == '/') {
        target = target.substr(1);
    }

    // Step 2: URL-decode the path
    std::string decoded_path = utils::http::DecodeUrl(target);

    // Step 3-4: Build and normalize path
    std::filesystem::path file_path = static_files_root_ / decoded_path;
    file_path = fs::weakly_canonical(file_path);

    // Step 5: Handle directory requests (serve index.html)
    if (std::filesystem::is_directory(file_path)) {
        file_path /= api_paths::INDEX_HTML;
        file_path = fs::weakly_canonical(file_path);  // Снова нормализуем
    }

    // Step 6: Security check - prevent path traversal attacks
    if (!utils::IsSubPath(file_path, fs::weakly_canonical(static_files_root_))) {
        return response::Builder::MakeError(req, http::status::bad_request,
                                            error_codes::BAD_REQUEST, error_messages::INVALID_PATH);
    }

    // Step 7: Check if file exists and is a regular file (not a directory/symlink)
    if (!fs::exists(file_path) || !fs::is_regular_file(file_path)) {
        return response::Builder::From(req)
        .WithStatus(http::status::not_found)
            .WithBody(error_messages::SOURCE_FAIL)
            .WithContentType(ContentType::TEXT_PLAIN)
            .WithKeepAlive(req.keep_alive())
            .Build();
    }

    // Step 8: Read and serve the file
    try {
        // Open file in binary mode to preserve exact content
        std::ifstream file(file_path.string(), std::ios::binary);
        if (!file) {
            return response::Builder::MakeError(req, http::status::internal_server_error,
                                                error_codes::INTERNAL_ERROR, error_messages::SOURCE_FAIL);
        }

        // Read entire file into string (efficient for small files)
        std::string content((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());

        // Build successful response with correct MIME type
        return response::Builder::From(req)
            .WithStatus(http::status::ok)
            .WithBody(content)
            .WithContentType(utils::http::GetMimeType(file_path))
            .WithKeepAlive(req.keep_alive())
            .Build();
    }
    catch (const std::exception& e) {
        // Log error for debugging
        std::cerr << "Exception serving file: " << e.what() << std::endl;
        return response::Builder::MakeError(req, http::status::internal_server_error,
                                            error_codes::INTERNAL_ERROR, error_messages::SOURCE_FAIL);
    }
}

}  // namespace http_handler
