// Run each GoogleTest executable in a fresh process with a bounded runtime.
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <thread>
#include <vector>
#include <sys/wait.h>
#include <unistd.h>

volatile std::sig_atomic_t interrupted = 0;
void handleSignal(int signal) { interrupted = signal; }

int main(int argc, char** argv)
{
    int separator = 1;
    while(separator < argc && std::strcmp(argv[separator], "--") != 0) ++separator;
    if(separator == 1 || separator == argc)
    {
        std::cerr << "usage: run_tests TEST... -- [GOOGLETEST_ARGUMENTS...]\n";
        return 1;
    }
    const char* timeoutValue = std::getenv("KERNEL_TEST_TIMEOUT");
    char* end = nullptr;
    long timeout = timeoutValue ? std::strtol(timeoutValue, &end, 10) : 30;
    if(timeout <= 0 || (timeoutValue && *end))
    {
        std::cerr << "KERNEL_TEST_TIMEOUT must be a positive number of seconds\n";
        return 1;
    }
    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);
    int failures = 0;
    for(int test = 1; test < separator; ++test)
    {
        std::vector<char*> arguments{argv[test]};
        for(int i = separator + 1; i < argc; ++i) arguments.push_back(argv[i]);
        arguments.push_back(nullptr);
        std::cout << "\n" << argv[test] << std::endl;
        auto child = fork();
        if(child < 0) { std::perror("fork"); return 1; }
        if(child == 0)
        {
            std::signal(SIGINT, SIG_DFL);
            std::signal(SIGTERM, SIG_DFL);
            setpgid(0, 0);
            execv(arguments[0], arguments.data());
            std::perror("execv");
            _exit(127);
        }
        setpgid(child, child);
        auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(timeout);
        int status = 0;
        bool timedOut = false;
        while(true)
        {
            auto result = waitpid(child, &status, WNOHANG);
            if(result == child) break;
            if(result < 0 && errno != EINTR) { std::perror("waitpid"); return 1; }
            if(interrupted || std::chrono::steady_clock::now() >= deadline)
            {
                timedOut = !interrupted;
                kill(-child, SIGKILL);
                while(waitpid(child, &status, 0) < 0 && errno == EINTR) {}
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        if(interrupted) return 128 + interrupted;
        if(timedOut || !WIFEXITED(status) || WEXITSTATUS(status) != 0)
        {
            ++failures;
            std::cerr << "FAILED: " << argv[test];
            if(timedOut) std::cerr << " (timed out after " << timeout << "s)";
            else if(WIFSIGNALED(status)) std::cerr << " (signal " << WTERMSIG(status) << ")";
            else std::cerr << " (exit " << WEXITSTATUS(status) << ")";
            std::cerr << '\n';
        }
    }
    std::cout << "\n" << separator - 1 << " test executables, " << failures << " failed\n";
    return failures ? 1 : 0;
}
