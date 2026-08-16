#include "CMakeFileApi.h"

#include "Paths.h"
#include "environment/EnvironmentPaths.h"
#include "exceptions/CompilationDatabaseException.h"
#include "utils/JsonUtils.h"
#include "utils/StringUtils.h"

#include "loguru.h"

#include <algorithm>
#include <map>
#include <string>
#include <vector>

namespace CMakeFileApi {
    namespace {
        // The name a reply is filed under. CMake keys replies by client, so a
        // build tree configured by an IDE that also uses the file API answers
        // both of us without either overwriting the other.
        const std::string CLIENT = "client-utbot";

        fs::path apiDir(const fs::path &buildDirPath) {
            return buildDirPath / ".cmake" / "api" / "v1";
        }

        fs::path replyDir(const fs::path &buildDirPath) {
            return apiDir(buildDirPath) / "reply";
        }

        /// The index is written afresh on every configure and named with a
        /// hash, so the newest one is the only one that describes this tree.
        fs::path findIndexFile(const fs::path &buildDirPath) {
            const fs::path dir = replyDir(buildDirPath);
            if (!fs::exists(dir)) {
                return {};
            }
            fs::path newest;
            fs::file_time_type newestTime{};
            for (const auto &entry : fs::directory_iterator(dir)) {
                const fs::path &candidate = entry.path();
                if (!StringUtils::startsWith(candidate.filename().string(), "index-")) {
                    continue;
                }
                const auto written = fs::last_write_time(candidate);
                if (newest.empty() || written > newestTime) {
                    newest = candidate;
                    newestTime = written;
                }
            }
            return newest;
        }

        fs::path absoluteAgainst(const std::string &path, const fs::path &base) {
            fs::path candidate(path);
            if (fs::is_absolute(candidate)) {
                return candidate.lexically_normal();
            }
            return (base / candidate).lexically_normal();
        }

        /**
         * Which object file each source becomes, according to compile_commands.
         *
         * The file API does not name object files -- it describes targets and
         * sources, and leaves the object layout to the generator. Rather than
         * reproduce that layout per generator, read it back out of the compile
         * command that produces each one.
         */
        std::map<std::string, fs::path> objectsBySource(const fs::path &compileCommandsPath) {
            std::map<std::string, fs::path> objects;
            const nlohmann::json commands = JsonUtils::getJsonFromFile(compileCommandsPath);
            for (const nlohmann::json &command : commands) {
                const fs::path directory = command.at("directory").get<std::string>();
                const fs::path source =
                        absoluteAgainst(command.at("file").get<std::string>(), directory);

                std::vector<std::string> arguments;
                if (command.contains("arguments")) {
                    arguments = command.at("arguments").get<std::vector<std::string>>();
                } else {
                    arguments = StringUtils::splitByWhitespaces(command.at("command").get<std::string>());
                }
                for (size_t i = 0; i + 1 < arguments.size(); ++i) {
                    if (arguments[i] == "-o" || arguments[i] == "/Fo") {
                        objects.emplace(source.string(), absoluteAgainst(arguments[i + 1], directory));
                        break;
                    }
                }
            }
            return objects;
        }

        /**
         * Where in the index the codemodel reply is named.
         *
         * Two shapes, because there are two ways to ask. A stateless query is
         * an empty file named for the object, and is answered directly under
         * the client's key. A query.json -- what this asks with, because it is
         * the form that can report a version it could not satisfy -- is
         * answered under that file's name, in a list of responses that has to
         * be searched by kind.
         */
        std::string findCodemodelFile(const nlohmann::json &index) {
            const auto &reply = index.at("reply");
            if (!reply.contains(CLIENT)) {
                throw CompilationDatabaseException(
                        "CMake answered nothing for " + CLIENT +
                        ". The query has to exist before cmake runs, not after.");
            }
            const auto &answered = reply.at(CLIENT);
            if (answered.contains("query.json")) {
                const auto &stateful = answered.at("query.json");
                if (stateful.contains("error")) {
                    throw CompilationDatabaseException(
                            "CMake rejected the file API query: " +
                            stateful.at("error").get<std::string>());
                }
                for (const nlohmann::json &response : stateful.at("responses")) {
                    if (response.contains("error")) {
                        throw CompilationDatabaseException(
                                "CMake rejected part of the file API query: " +
                                response.at("error").get<std::string>());
                    }
                    if (response.value("kind", std::string{}) == "codemodel") {
                        return response.at("jsonFile").get<std::string>();
                    }
                }
            }
            if (answered.contains("codemodel-v2")) {
                return answered.at("codemodel-v2").at("jsonFile").get<std::string>();
            }
            throw CompilationDatabaseException(
                    "CMake answered no codemodel for " + CLIENT +
                    ". The build directory was configured before UTBot asked for one.");
        }

        bool producesABinary(const std::string &type) {
            return type == "EXECUTABLE" || type == "STATIC_LIBRARY" ||
                   type == "SHARED_LIBRARY" || type == "MODULE_LIBRARY";
        }
    }

    void writeQuery(const fs::path &buildDirPath) {
        const fs::path queryDir = apiDir(buildDirPath) / "query" / CLIENT;
        fs::create_directories(queryDir);
        nlohmann::json query;
        query["requests"] = nlohmann::json::array(
                { nlohmann::json{ { "kind", "codemodel" }, { "version", 2 } } });
        JsonUtils::writeJsonToFile(queryDir / "query.json", query);
        LOG_S(DEBUG) << "Asked CMake for a codemodel reply in " << queryDir;
    }

    bool hasReply(const fs::path &buildDirPath) {
        return !findIndexFile(buildDirPath).empty();
    }

    void writeLinkCommands(const fs::path &buildDirPath) {
        const fs::path indexPath = findIndexFile(buildDirPath);
        if (indexPath.empty()) {
            throw CompilationDatabaseException(
                    "CMake left no file API reply in " + replyDir(buildDirPath).string() +
                    ". The build directory was configured by a CMake older than 3.14, or "
                    "before UTBot asked for one.");
        }
        const fs::path dir = replyDir(buildDirPath);
        const nlohmann::json index = JsonUtils::getJsonFromFile(indexPath);

        const nlohmann::json codemodel =
                JsonUtils::getJsonFromFile(dir / findCodemodelFile(index));

        const fs::path sourceRoot = codemodel.at("paths").at("source").get<std::string>();
        const fs::path buildRoot = codemodel.at("paths").at("build").get<std::string>();
        const auto objects = objectsBySource(buildDirPath / "compile_commands.json");

        // Two passes: a target's link inputs include the artifacts of the
        // targets it depends on, and dependencies are given by id, so every
        // target's artifact has to be known before any command is written.
        std::map<std::string, nlohmann::json> targetsById;
        std::map<std::string, fs::path> artifactById;
        for (const nlohmann::json &configuration : codemodel.at("configurations")) {
            for (const nlohmann::json &entry : configuration.at("targets")) {
                const nlohmann::json target =
                        JsonUtils::getJsonFromFile(dir / entry.at("jsonFile").get<std::string>());
                const std::string id = target.at("id").get<std::string>();
                targetsById.emplace(id, target);
                if (target.contains("artifacts") && !target.at("artifacts").empty()) {
                    artifactById.emplace(id, absoluteAgainst(
                            target.at("artifacts").at(0).at("path").get<std::string>(), buildRoot));
                }
            }
        }

        nlohmann::json linkCommands = nlohmann::json::array();
        for (const auto &[id, target] : targetsById) {
            const std::string type = target.at("type").get<std::string>();
            const std::string name = target.at("name").get<std::string>();
            if (!producesABinary(type) || artifactById.find(id) == artifactById.end()) {
                LOG_S(DEBUG) << "Skipping target " << name << " of type " << type;
                continue;
            }
            const fs::path output = artifactById.at(id);

            std::vector<std::string> files;
            if (target.contains("sources")) {
                for (const nlohmann::json &source : target.at("sources")) {
                    // A source with no compile group is not compiled -- a
                    // header listed for an IDE's benefit, or a generated file
                    // that only exists as a dependency edge.
                    if (!source.contains("compileGroupIndex")) {
                        continue;
                    }
                    const fs::path path =
                            absoluteAgainst(source.at("path").get<std::string>(), sourceRoot);
                    const auto object = objects.find(path.string());
                    if (object == objects.end()) {
                        LOG_S(WARNING) << "No compile command for " << path << " in " << name;
                        continue;
                    }
                    files.emplace_back(object->second.string());
                }
            }
            if (target.contains("dependencies")) {
                for (const nlohmann::json &dependency : target.at("dependencies")) {
                    const std::string dependencyId = dependency.at("id").get<std::string>();
                    const auto artifact = artifactById.find(dependencyId);
                    if (artifact != artifactById.end() &&
                        Paths::isLibraryFile(artifact->second)) {
                        files.emplace_back(artifact->second.string());
                    }
                }
            }
            if (files.empty()) {
                LOG_S(WARNING) << "Target " << name << " links nothing UTBot can see";
                continue;
            }

            // The command is rebuilt rather than quoted from CMake, because
            // what reads it back parses it: the output is found by -o, and an
            // archive is recognised by its tool's name. Flags CMake would pass
            // are deliberately left out -- none of them change which files go
            // into which binary, which is all this database is asked for.
            std::vector<std::string> commandLine;
            if (type == "STATIC_LIBRARY") {
                commandLine.emplace_back(Paths::getAr().string());
                commandLine.emplace_back("qc");
                commandLine.emplace_back(output.string());
                commandLine.insert(commandLine.end(), files.begin(), files.end());
            } else {
                commandLine.emplace_back(Paths::getUTBotClangPP().string());
                if (type != "EXECUTABLE") {
                    commandLine.emplace_back("-shared");
                }
                commandLine.insert(commandLine.end(), files.begin(), files.end());
                commandLine.emplace_back("-o");
                commandLine.emplace_back(output.string());
            }

            nlohmann::json entry;
            entry["directory"] = buildDirPath.string();
            entry["command"] = StringUtils::joinWith(commandLine, " ");
            entry["files"] = files;
            entry["output"] = output.string();
            linkCommands.push_back(entry);
            LOG_S(DEBUG) << "Target " << name << " (" << type << ") links "
                         << files.size() << " files into " << output;
        }

        if (linkCommands.empty()) {
            throw CompilationDatabaseException(
                    "CMake's codemodel describes no linkable target in " + buildDirPath.string());
        }
        JsonUtils::writeJsonToFile(buildDirPath / "link_commands.json", linkCommands);
        LOG_S(INFO) << "Wrote link_commands.json for " << linkCommands.size() << " targets";
    }
}
