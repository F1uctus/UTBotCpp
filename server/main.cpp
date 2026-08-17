#include "Server.h"
#include "utils/CLIUtils.h"

#include "loguru.h"

#include <grpc/grpc.h>

#include <llvm/Support/Signals.h>

#include <cstdlib>
#include "config.h"

int main(int argc, char **argv) {
    setenv("GRPC_ENABLE_FORK_SUPPORT", "1", 1);
    // BaseForkTask calls grpc_prefork/grpc_postfork around every fork so that
    // gRPC's internal state survives it. Those handlers are only registered by
    // grpc_init, which in server mode the server does -- but the CLI starts no
    // server, so grpc_prefork dispatched through a null pointer and the process
    // died with a bare SIGSEGV before generating anything.
    //
    // grpc_init is refcounted and cheap, and this is what gRPC asks of any
    // process that calls its fork API. It becomes unnecessary once the tasks
    // spawn processes rather than forking.
    grpc_init();
    llvm::sys::PrintStackTraceOnErrorSignal(argv[0]);
    CLI::App app{ PROJECT_DESCRIPTION, PROJECT_NAME };
    std::atexit([]() { std::cout << rang::style::reset; });
    try {
        CLIUtils::parse(argc, argv, app);
    } catch (const CLI::ParseError &e) {
        std::cout << (e.get_exit_code() == 0 ? rang::fg::green : rang::fg::red);
        return app.exit(e);
    } catch (const std::exception &e) {
        // Anything the command threw and did not handle. Without this it
        // reached the runtime's terminate handler, which on Windows prints an
        // exception code and a stack trace and no message at all -- so the one
        // thing that would say what went wrong, the what(), was the one thing
        // not shown.
        std::cout << rang::fg::red;
        LOG_S(ERROR) << e.what();
        std::cout << rang::style::reset;
        return 1;
    }
    return 0;
}
