#ifndef HIASM_SVN_CLIENT_H
#define HIASM_SVN_CLIENT_H

#include <functional>
#include <memory>
#include <string>

// Subversion runs outside the UI thread. Arguments never pass through a shell.
class SvnClient {
public:
    SvnClient() = default;
    SvnClient(const SvnClient&) = delete;
    SvnClient& operator=(const SvnClient&) = delete;
    enum Action { Update, Log, Diff };
    using Completion = std::function<void(bool, const std::string&)>;
    static bool available();
    static bool isWorkingCopy(const std::string &target);
    bool start(Action action, const std::string &target, Completion completion);
    bool busy() const { return static_cast<bool>(state); }
    ~SvnClient();
private:
    struct State;
    std::shared_ptr<State> state;
};
#endif
