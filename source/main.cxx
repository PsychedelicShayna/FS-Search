#include <experimental/filesystem>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cstdint>
#include <string>
#include <memory>
#include <tuple>
#include <cmath>

namespace fs = std::experimental::filesystem;

#ifdef WINDOWS
    #include <direct.h>
    #define CwdFunction _getcwd
#else
    #include <unistd.h>
    #define CwdFunction getcwd
#endif

#ifdef WINARGSTYLE
    #define ArgFilenamePattern "/fp"
    #define ArgTargetDirectory "/d"
    #define ArgContentPattern "/cp"
    #define ArgMaxFileCount "/mc"
    #define ArgMaxFileSize "/ms"
    #define ArgVerbose "/v"
    #define ArgDebug "/dbg"
#else
    #define ArgFilenamePattern "-fp"
    #define ArgTargetDirectory "-d"
    #define ArgContentPattern "-cp"
    #define ArgMaxFileCount "-mc"
    #define ArgMaxFileSize "-ms"
    #define ArgDebug "-dbg"
    #define ArgVerbose "-v"
#endif

inline const std::string FetchCwd() {
    char current_working_directory[FILENAME_MAX];
    memset(current_working_directory, 0x00, FILENAME_MAX);

    CwdFunction(current_working_directory, FILENAME_MAX);

    return std::string(current_working_directory);
}

struct Settings {
    std::vector<std::string> ContentPatterns;
    std::vector<std::string> FilenamePatterns;
    std::string TargetDirectory;

    uint32_t MaxFileSize;
    uint32_t MaxFileCount;

    bool Verbose;

    friend std::ostream& operator<<(std::ostream&, const Settings&);

    Settings(int argc, char** argv, bool* valid) {
        this->TargetDirectory = FetchCwd();
        this->MaxFileSize = 0;
        this->MaxFileCount = 0;
        this->Verbose = false;

        *valid = true;

        bool debug_print = false;

        for(int32_t i=0; i<argc; ++i) {
            char* current = argv[i];

            if(!strcmp(current, ArgVerbose)) this->Verbose = true;

            if(!strcmp(current, ArgDebug)) debug_print = true;

            if(!strcmp(current, ArgTargetDirectory) && i+1 < argc) {
                this->TargetDirectory = std::string(argv[++i]);

                if(!fs::exists(this->TargetDirectory) || !fs::is_directory(this->TargetDirectory)) {
                    std::cerr << "Invalid target directory." << std::endl;
                    std::cerr << "Dir: " << this->TargetDirectory << std::endl;
                    *valid = false;
                    return;
                } 
            }

            if(!strcmp(current, ArgFilenamePattern) && i+1 < argc) {
                for(auto [stream, pattern] = std::tuple{std::istringstream(argv[++i]), std::string()}; std::getline(stream, pattern, ':'); this->FilenamePatterns.emplace_back(pattern));
            }

            if(!strcmp(current, ArgMaxFileSize) && i+1 < argc) {
                try {
                    this->MaxFileSize = std::stoi(std::string(argv[++i]));    
                } catch(const std::invalid_argument& exception) {
                    std::cerr << "Invalid maximum file size; " << exception.what() << std::endl;
                    *valid = false;
                    return;
                }
            }

            if(!strcmp(current, ArgMaxFileCount) && i+1 < argc) {
                try {
                    this->MaxFileCount = std::stoi(std::string(argv[++i]));    
                } catch(const std::invalid_argument& exception) {
                    std::cerr << "Invalid maximum file count; " << exception.what() << std::endl;
                    *valid = false;
                    return;
                }
            }

            if(!strcmp(current, ArgContentPattern) && i+1 < argc) {
                for(int32_t c=(i+1); c<argc; ++c) ContentPatterns.emplace_back(argv[c]);
                break;
            }
        }
    
        if(debug_print) std::clog << *this;
    }
};

std::ostream& operator<<(std::ostream& outputStream, const Settings& settings) {
    outputStream << std::endl;

    std::cout << "Target Directory: " << settings.TargetDirectory << std::endl;

    std::cout << "Maximum File Size: " << settings.MaxFileSize << std::endl;
    std::cout << "Maximum File Count: " << settings.MaxFileCount << std::endl << std::endl;

    outputStream << "Content Patterns: {" << std::endl;
    for(std::size_t i=0; i<settings.ContentPatterns.size(); ++i) {
        if(i != 0) outputStream << std::endl;
        outputStream << "    " << settings.ContentPatterns.at(i);
    }
    outputStream << std::endl << "}" << std::endl << std::endl;

    outputStream << "Filename Patterns: {" << std::endl;
    for(std::size_t i=0; i<settings.FilenamePatterns.size(); ++i) {
        if(i != 0) outputStream << std::endl;
        outputStream << "    " << settings.FilenamePatterns.at(i);
    }
    outputStream << std::endl << "}" << std::endl << std::endl;

    return outputStream;
}

int main(int argc, char* argv[]) {
    bool error_free;

    Settings settings(argc, argv, &error_free);
    if(!error_free) return 1;

    if(!settings.ContentPatterns.size()) {
        std::cerr << "No content patterns have been specified!" << std::endl << std::endl;
        std::cout << "Argument Reference" << std::endl;
        std::cout << "Content pattern: (Required) -cp \"int main()\" | The pattern(s) used to match files. Every argument past -cp is considered a pattern, and thus -cp must be placed at the end." << std::endl;
        std::cout << "Target directory: -d \"C:\\Program Files\" | Specifies the target directory. If not specified, the current directory is used. Do NOT place a backslack at the end of the path if the path contains a quote, or it will cancel it, e.g. do \"C:\\Program Files\" and not \"C:\\Program Files\\ \"" << std::endl;
        std::cout << "File pattern: -fp .txt:.cxx:.hxx | Only includes files who's absolute path matches one or more patterns, e.g. \"test_main.cxx\" will match \"-fp test\"" << std::endl;
        std::cout << "Maximum size: -ms 10000 | Skips files that exceed the specified maximum size in bytes." << std::endl;
        std::cout << "Maximum count: -mc 1000 | Will stop the search after N amount of searched files. Skipped files not included." << std::endl;
        std::cout << "Verbose: -v | Displays a list of skipped files, stream errord files, and files that caused exceptions." << std::endl;
        std::cout << "Debug: -dbg | Displays debug information on how the command parser managed to parse the command line arguments." << std::endl;

        return 1;
    }

    uint32_t files_checked = 0;

    std::vector<std::pair<std::string, std::size_t>> excess_size;
    std::vector<std::pair<std::string, std::string>> exceptions;
    std::vector<std::string> stream_failures;

    std::vector<std::pair<std::string, std::vector<std::string>>> matches;

    const uint32_t distance = std::distance(fs::recursive_directory_iterator(settings.TargetDirectory), fs::recursive_directory_iterator());
    uint32_t iterations = 0;

    double previous_completion_percentage = 0;

    for(const auto& entry : fs::recursive_directory_iterator(settings.TargetDirectory)) {
        ++iterations;

        double current_completion_percentage = (static_cast<double>(iterations) / static_cast<double>(distance)) * 100;

        if(std::round(current_completion_percentage) != previous_completion_percentage) {
            std::cout << iterations << " / " << distance << " | " <<  current_completion_percentage << "% \r";
            previous_completion_percentage = std::round(current_completion_percentage);
        }

        // Skips entry if it's a directory and not a file.
        if(fs::is_directory(entry)) continue;

        // Skips entry if the path doesn't pass pattern checking.
        if (settings.FilenamePatterns.size() && std::none_of(settings.FilenamePatterns.begin(), settings.FilenamePatterns.end(), [&entry](const std::string& pattern) {
            return entry.path().string().find(pattern) != std::string::npos;
        })) continue;

        // Skips entry if the file size exceeds the maximum file size.
        if(settings.MaxFileSize && fs::file_size(entry) > settings.MaxFileSize) {
            excess_size.emplace_back(std::make_pair(entry.path().string(), fs::file_size(entry)));
            continue;
        }

        // Checks if the current file count exceeds the maximum file count.
        if(settings.MaxFileCount && ++files_checked > settings.MaxFileCount) break;

        try {
            std::ifstream input_stream(entry.path().string(), std::ios::binary);

            if(!input_stream.good()) {
                stream_failures.emplace_back(entry.path().string());
                continue;
            } else {
                std::string file_contents((std::istreambuf_iterator<char>(input_stream)), (std::istreambuf_iterator<char>()));
                input_stream.close();

                ++files_checked;
                std::vector<std::string> matched_patterns;

                if(std::any_of(settings.ContentPatterns.begin(), settings.ContentPatterns.end(), [&file_contents, &matched_patterns](const std::string& pattern){
                    if(file_contents.find(pattern) != std::string::npos) {
                        matched_patterns.emplace_back(pattern);
                        return true;
                    } else {
                        return false;
                    }
                })) {
                    matches.emplace_back(std::make_pair(entry.path().string(), matched_patterns));
                };
            }
        } catch(const std::exception& exception) {
            exceptions.emplace_back(std::make_pair(entry.path().string(), exception.what()));
            continue;
        }
    }
    
    std::cout << std::endl << std::endl;
    
    if(settings.Verbose) { 
        if(exceptions.size()) {
            std::cerr << "Skipped the following " << exceptions.size() << " files due to exceptions." << std::endl;
            std::cerr << "--------------------------------------------------" << std::endl;

            for(const auto& pair : exceptions) {
                std::cerr << pair.second << " | " << pair.first << std::endl;
            }

            std::cerr << "--------------------------------------------------" << std::endl << std::endl;
        }

        if(stream_failures.size()) {
            std::cerr << "Skipped the following " << stream_failures.size() << " files due to stream failures." << std::endl;
            std::cerr << "--------------------------------------------------" << std::endl;

            for(const auto& file : stream_failures) {
                std::cerr << file << std::endl;
            }

            std::cerr << "--------------------------------------------------" << std::endl << std::endl;
        }

        if(excess_size.size()) {
            std::cerr << "Skipped the following " << excess_size.size() << " files as they exceeded the maximum file size." << std::endl;
            std::cerr << "--------------------------------------------------" << std::endl;

            for(const auto& pair : excess_size) {
                std::cerr << pair.second << " Bytes | " << pair.first << std::endl;
            }

            std::cerr << "--------------------------------------------------" << std::endl << std::endl;
        }
    }

    if(matches.size()) {
        std::cout << "The following files matched one or more patterns." << std::endl;
        std::cout << "==================================================" << std::endl;

        for(const auto& pair : matches) {
            std::cout << "{";
            for(std::size_t i=0; i<pair.second.size(); ++i) {
                if(i != 0) std::cout << ", ";
                std::cout << pair.second.at(i);
            }
            std::cout << "} | Matched in > " << pair.first << std::endl;
        }

        std::cout << "==================================================" << std::endl << std::endl;
    }

    std::cout << "Searched through " << files_checked << " file(s) for " << settings.ContentPatterns.size() << " pattern(s)." << std::endl;
    std::cout << "Skipped " << excess_size.size() << " file(s) due to excess size." << std::endl;
    std::cout << "Skipped " << exceptions.size() << " file(s) due to exception(s)." << std::endl;
    std::cout << "Skipped " << stream_failures.size() << " file(s) due to stream failure(s)." << std::endl << std::endl;

    std::cout << "Matched " << matches.size() << " file(s)." << std::endl;  
}