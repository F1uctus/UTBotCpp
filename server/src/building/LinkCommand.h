#ifndef UNITTESTBOT_LINKCOMMAND_H
#define UNITTESTBOT_LINKCOMMAND_H

#include "BaseCommand.h"

#include "utils/path/FileSystemPath.h"
#include <list>
#include <string>

namespace utbot {
    class LinkCommand : public BaseCommand {
    private:
        void initOutput();

    public:
        LinkCommand() = default;

        LinkCommand(LinkCommand const &other);

        LinkCommand &operator=(LinkCommand const &other);

        LinkCommand(LinkCommand &&other) noexcept;

        LinkCommand &operator=(LinkCommand &&other) noexcept;

        LinkCommand(std::list<std::string> commandLine, fs::path directory, bool shouldChangeDirectory = false);

        LinkCommand(std::vector<std::string> commandLine, fs::path directory, bool shouldChangeDirectory = false);

        LinkCommand(std::initializer_list<std::string> commandLine, fs::path directory,
                    bool shouldChangeDirectory = false);

        friend void swap(LinkCommand &a, LinkCommand &b) noexcept;

        [[nodiscard]] bool isArchiveCommand() const override;

        [[nodiscard]] bool isSharedLibraryCommand() const;

        /// Rewrites an archive command to run the given archiver in ar's own
        /// grammar: `ar <operation><modifiers> <archive> <members>`, where the
        /// archive is positional.
        ///
        /// initOutput inserts a -o before the archive so that the output can be
        /// tracked as an iterator; that -o is not ar's, and only GNU ar let it
        /// pass, reading it as the 'o' modifier and ignoring it for 'r'.
        /// llvm-ar checks that a modifier applies to the operation and refuses.
        void useArchiver(fs::path archiver);
    };
}


#endif //UNITTESTBOT_LINKCOMMAND_H
