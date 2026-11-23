#ifndef HIASM_GIT_CLIENT_H
#define HIASM_GIT_CLIENT_H

#include <functional>
#include <memory>
#include <string>
#include <vector>

// Git commands run asynchronously and never pass through a shell.
class GitClient {
public:
    GitClient() = default;
    GitClient(const GitClient&) = delete;
    GitClient& operator=(const GitClient&) = delete;
    enum Action { Pull, Log, Diff, Status, Init };
    using Completion = std::function<void(bool, const std::string&)>;
    static bool available();
    static bool isWorkingCopy(const std::string &target);
    static std::string workingCopyRoot(const std::string &target);
    bool start(Action action, const std::string &target, Completion completion);
    bool clone(const std::string &url, const std::string &directory, Completion completion);
    bool busy() const { return static_cast<bool>(state); }
    ~GitClient();
private:
    struct State;
    std::shared_ptr<State> state;
    bool launch(const std::vector<std::string> &arguments, const std::string &cwd, Completion completion);
};
#endif
