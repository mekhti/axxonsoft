//
// Created by Mehdi Mammadov <mekhti@gmai.com> on 08-Jul-23.
//

#include <iostream>
#include <filesystem>
#include <vector>
#include <algorithm>
#include <future>
#include <fstream>

#define NCOUNT_BUFFER_SIZE (1 * 1024 * 1024) // 1 MB for buffer

// Function declarations
std::vector<std::string> parse_cli_options(int argc, char *argv[], std::string &directory);
void print_help();

uint64_t count_lines_getline(const std::filesystem::path &file_path);
uint64_t count_lines_ncount(const std::filesystem::path &file_path);
uint64_t count_buffered_ncount(const std::filesystem::path &file_path);

uint64_t count_getline_async(const std::vector<std::filesystem::directory_entry> &files);
uint64_t count_ncount_async(const std::vector<std::filesystem::directory_entry> &files);
uint64_t count_buffered_ncount_async(const std::vector<std::filesystem::directory_entry> &files);

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_help();
        return 1;
    }

    std::string directory;
    std::vector<std::string> options = parse_cli_options(argc, argv, directory);

    if (std::find(options.begin(), options.end(), "h") != options.end()) {
        print_help();
        return 1;
    }

    if (directory.empty()) {
        std::cout << "No directory provided\n";
        return 1;
    }

    if (!std::filesystem::exists(directory)) {
        std::cout << "Path does not exist\n";
        return 1;
    }

    std::filesystem::path dir_path_from_cli{directory};
    if (!std::filesystem::is_directory(dir_path_from_cli)) {
        std::cout << "Not a directory\n";
        return 1;
    }

    auto dir = std::filesystem::directory_iterator{dir_path_from_cli};
    std::vector<std::filesystem::directory_entry> files;
    for (const auto &entry: dir) {
        if (is_regular_file(entry)) {
            files.push_back(entry);
        }
    }

    if (std::find(options.begin(), options.end(), "b") != options.end()) {
        /**
         * Benchmarking different methods
         *
         * 1. getline method.
         * 2. ncount method.
         * 3. buffered ncount method.
         */

        std::cout << "Benchmarking...\n";

        // getline method
        auto start = std::chrono::steady_clock::now();
        uint64_t lines_count = count_getline_async(files);
        std::cout << "getline method total runing time: "
                  << std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count()
                  << " millisecond \n";
        std::cout << "Total lines: " << lines_count << "\n";

        // ncount method
        start = std::chrono::steady_clock::now();
        lines_count = count_ncount_async(files);
        std::cout << "ncounting method total runing time: "
                  << std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count()
                  << " millisecond \n";
        std::cout << "Total lines: " << lines_count << "\n";

        // buffered ncount method
        start = std::chrono::steady_clock::now();
        lines_count = count_buffered_ncount_async(files);
        std::cout << "buffered ncounting method total runing time: "
                  << std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count()
                  << " millisecond \n";
        std::cout << "Total lines: " << lines_count << "\n";
    } else if (std::find(options.begin(), options.end(), "n") != options.end()) {
        // ncount method
        std::cout << "Lines count using ncount method: " << count_ncount_async(files) << "\n";
    } else if (std::find(options.begin(), options.end(), "g") != options.end()) {
        // getline method
        std::cout << "Lines count using getline method: " << count_getline_async(files) << "\n";
    } else if (std::find(options.begin(), options.end(), "m") != options.end()) {
        // buffered ncount method
        std::cout << "Lines count using buffered ncount method: " << count_buffered_ncount_async(files) << "\n";
    } else {
        // default method, getline method used as a default method
        std::cout << count_getline_async(files) << "\n";
    }

    return 0;
}


/**
 * Three functions below implement parallelization of counting lines using different methods.
 * std::future and std::async are used for parallelization and concurrency. This approach avoids
 * the complexity of manually managing threads and takes advantage of the automatic thread
 * management offered by std::async.
 *
 * The code of each function launches a separate task for each file in the directory using
 * std::async, which automatically takes care of thread management. Each task returns
 * a std::future that will eventually hold the number of lines in the corresponding file.
 * The futures vector stores these std::future objects. The total number of lines is then
 * computed by iterating through the futures vector and calling get on each std::future,
 * which blocks until the result is available.
 *
 * However, a key aspect to remember is that while std::async can enable tasks to be run in parallel,
 * it doesn't necessarily guarantee that they will be. The actual level of concurrency is decided
 * by the system's task scheduler and the C++ runtime environment. So while this method is a best
 * practice for potentially utilizing all available CPU cores, it doesn't provide an absolute guarantee.
 *
 * Comparing this approach to a single-threaded approach, the std::async method could be faster
 * if the workload is large enough and the system has multiple cores, as the work can be divided
 * among the cores. On the other hand, for small workloads or on a single-core system, a single-threaded
 * approach may be faster due to the overhead of thread creation and management with std::async.
 */

uint64_t count_getline_async(const std::vector<std::filesystem::directory_entry> &files) {
    /**
     * Count lines using getline method.
     *
     * @param files vector of files to count lines
     * @return total lines count
     */
    std::vector<std::future<uint64_t>> futures; // vector of futures
    futures.reserve(files.size()); // reserve space for futures
    for (const auto &file: files) {
        // for each file, create a future and push it to the vector
        futures.push_back(std::async(std::launch::async, count_lines_getline, file.path()));
    }

    uint64_t lines_count = 0; // total lines count
    for (auto &future: futures) {
        lines_count += future.get(); // get the value of the future and add it to the total lines count
    }
    return lines_count;
}

uint64_t count_ncount_async(const std::vector<std::filesystem::directory_entry> &files) {
    /**
     * Count lines using ncount method.
     *
     * @param files vector of files to count lines
     * @return total lines count
     */
    std::vector<std::future<uint64_t>> futures;
    futures.reserve(files.size());
    for (const auto &file: files) {
        futures.push_back(std::async(std::launch::async, count_lines_ncount, file.path()));
    }

    uint64_t lines_count = 0;
    for (auto &future: futures) {
        lines_count += future.get();
    }
    return lines_count;
}

uint64_t count_buffered_ncount_async(const std::vector<std::filesystem::directory_entry> &files){
    /**
     * Count lines using buffered ncount method.
     *
     * @param files vector of files to count lines
     * @return total lines count
     */
    std::vector<std::future<uint64_t>> futures;
    futures.reserve(files.size());
    for (const auto &file: files) {
        futures.push_back(std::async(std::launch::async, count_buffered_ncount, file.path()));
    }

    uint64_t lines_count = 0;
    for (auto &future: futures) {
        lines_count += future.get();
    }
    return lines_count;
}

uint64_t count_lines_getline(const std::filesystem::path &file_path) {
    /**
     * Count lines using getline method.
     *
     * Counting lines in a text file can be an I/O bound process, and it could be a performance
     * bottleneck for the given task. The solution provided uses the standard getline function
     * in a line-by-line fashion, which might not be the most efficient way.
     *
     * @param file_path path to the file to count lines
     * @return total lines count
     */
    std::ifstream file{file_path};
    std::string line;
    uint64_t lines_count = 0;
    while (std::getline(file, line)) {
        ++lines_count;
    }
    return lines_count;
}

uint64_t count_lines_ncount(const std::filesystem::path &file_path) {
    /**
     * Count lines using ncount method.
     *
     * @param file_path path to the file to count lines
     * @return total lines count
     */
    std::ifstream file{file_path};
    uint64_t lines_count = std::count(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>(), '\n');
    return lines_count;
}

uint64_t count_buffered_ncount(const std::filesystem::path &file_path){
    /**
     * Count lines using buffered ncount method.
     *
     * @param file_path path to the file to count lines
     * @return total lines count
     */
    std::ifstream file(file_path, std::ios::in);
    std::vector<char> buffer(NCOUNT_BUFFER_SIZE);
    uint64_t lines_count = 0;

    while (file.read(buffer.data(), buffer.size())) {
        lines_count += std::count(buffer.begin(), buffer.end(), '\n');
    }

    // Count remaining characters after last read
    lines_count += std::count(buffer.begin(), buffer.begin() + file.gcount(), '\n');

    // Update global line lines_count
    return lines_count;
}

/**
 * Function to parse command line options implemented from scratch due there is no any ready to
 * using implementation of command line options parser in the STL.
 *
 * This solution has a few limitations. It assumes that there is at most one directory argument
 * and that it is the last non-option argument. If more than one directory is provided, only the
 * last one is recorded. Similarly, if a directory is followed by an option, the option will be
 * treated as part of the directory's value. More advanced parsing strategies or dedicated libraries
 * can handle these situations more robustly.
 *
 * Parse command line options.
 * @param argc
 * @param argv
 * @return
 */

std::vector<std::string> parse_cli_options(int argc, char *argv[], std::string &directory) {
    std::vector<std::string> options;

    for (int i = 1; i < argc; ++i) { // Start at 1 to skip the program name
        std::string arg = argv[i];

        if (arg[0] == '-') { // If it starts with '-', it's an option
            options.push_back(arg.substr(1));
        } else {
            directory = arg; // If it does not start with '-', treat it as a directory
        }
    }
    return options;
}

void print_help() {
    std::cout << "Usage: axxonsoft_test [options] directory\n"
              << "Options:\n"
              << "  -g   use getline method. Used by default. \n"
              << "  -n   use \\n counting \n"
              << "  -m   use buffered \\n counting \n"
              << "  -b   benchmark two methods \n"
              << "  -h   print this help message \n"
              << "directory: The path to the directory to process. \n"
                 "           This argument must not be prefixed with '-'.\n";
}