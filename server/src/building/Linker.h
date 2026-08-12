#ifndef UNITTESTBOT_LINKER_H
#define UNITTESTBOT_LINKER_H

#include "BuildResult.h"
#include "IRParser.h"
#include "KleeGenerator.h"
#include "RunCommand.h"
#include "printers/DefaultMakefilePrinter.h"
#include "printers/NativeMakefilePrinter.h"
#include "printers/TestMakefilesPrinter.h"
#include "testgens/BaseTestGen.h"
#include "utils/CollectionUtils.h"
#include "utils/MakefileUtils.h"
#include "utils/Void.h"
#include "stubs/StubGen.h"

#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class Linker {
public:
    Linker(BaseTestGen &testGen,
           StubGen stubGen,
           std::shared_ptr<LineInfo> lineInfo,
           std::shared_ptr<KleeGenerator> kleeGenerator);

    void prepareArtifacts();

    std::vector<tests::TestMethod> getTestMethods();

    CollectionUtils::MapFileTo<fs::path> getSelectedTargets();

    BuildResult
    addLinkTargetRecursively(const fs::path &fileToBuild,
                             const CollectionUtils::FileSet &stubSources,
                             const CollectionUtils::MapFileTo<fs::path> &bitcodeFiles,
                             std::string const &suffixForParentOfStubs,
                             bool hasParent,
                             const std::optional<fs::path> &testedFilePath,
                             bool shouldChangeDirectory = false);

    struct LinkResult {
        fs::path bitcodeOutput;
        CollectionUtils::FileSet stubsSet;
        CollectionUtils::FileSet presentedFiles;
    };

    /**
     * One step of the link, recorded where it used to be written as a makefile
     * recipe.
     *
     * The steps are declared in dependency order already -- every archive
     * before the module that consumes it -- so running them in that order is
     * the whole of the build, and needs neither make nor a shell to read
     * recipes with. A step either deletes a file or runs a program; the
     * deletions were "rm -f" in the recipes, which is a file operation rather
     * than a build step.
     */
    struct BuildStep {
        std::optional<fs::path> fileToRemove;
        std::optional<ShellExecTask::ExecutionParameters> command;
        fs::path workDir;
        fs::path outputDirectory;
    };

    /**
     * The final llvm-link, which cannot be recorded like the rest.
     *
     * It consumes the stub bitcode, and that list is only known after the first
     * pass has run and the missing symbols have been stubbed -- which is why
     * the makefile referred to it as $(STUB_BITCODE_FILES), a variable a second
     * makefile defined later. Keeping the inputs rather than the command lets
     * it be rebuilt against whatever the stub list holds at the time it runs.
     */
    struct RootLinkPlan {
        bool valid = false;
        fs::path prefixPath;
        fs::path archive;
        fs::path rootOutput;
        bool shouldChangeDirectory = false;
    };
private:
    BaseTestGen &testGen;
    std::shared_ptr<KleeGenerator> kleeGenerator;
    StubGen stubGen;
    std::shared_ptr<LineInfo> lineInfo;

    std::vector<BuildStep> buildPlan;
    RootLinkPlan rootLinkPlan;
    /** Empty until stubs have been generated; usually empty throughout. */
    std::vector<fs::path> stubBitcodeFiles;

    void planRemove(const fs::path &file);
    void planCommand(const utbot::BaseCommand &command);
    /** Runs the recorded steps in order. Returns a message on failure. */
    std::string runBuildPlan();

    CollectionUtils::FileSet testedFiles;
    CollectionUtils::MapFileTo<fs::path> bitcodeFileName;
    CollectionUtils::MapFileTo<fs::path> selectedTargets;
    CollectionUtils::FileSet brokenLinkFiles;

    IRParser irParser;

    fs::path getSourceFilePath();

    bool isForOneFile();

    Result<Linker::LinkResult> linkForTarget(const fs::path &target, const fs::path &sourceFilePath,
                                             const std::shared_ptr<const BuildDatabase::ObjectFileInfo> &compilationUnitInfo,
                                             const fs::path &objectFile);

    Result<Linker::LinkResult> linkWholeTarget(const fs::path &target);
    void linkForOneFile(const fs::path &sourceFilePath);
    void linkForProject();
    Result<Linker::LinkResult> link(const CollectionUtils::MapFileTo<fs::path> &bitcodeFiles,
                                    const fs::path &root,
                                    std::string const &suffixForParentOfStubs,
                                    const std::optional<fs::path> &testedFilePath,
                                    const CollectionUtils::FileSet &stubSources,
                                    bool errorOnMissingBitcode = true);

    void checkSiblingsExist(const CollectionUtils::FileSet &archivedFiles) const;
    void addToGenerated(const CollectionUtils::FileSet &objectFiles, const fs::path &output);
    fs::path getPrefixPath(const std::vector<fs::path> &dependencies, fs::path defaultPath) const;

    /** Generates the stubs and records their compiles into the build plan. */
    Result<CollectionUtils::FileSet> generateStubs(const fs::path &root, const fs::path &outputFile);
    Result<utbot::Void> linkWithStubsIfNeeded(const fs::path &targetBitcode);

    fs::path declareRootLibraryTarget(const fs::path &output,
                                      const std::vector<fs::path> &bitcodeDependencies,
                                      const fs::path &prefixPath,
                                      std::vector<utbot::LinkCommand> archiveActions,
                                      bool shouldChangeDirectory = false);

    std::string getLinkArgument(const std::string &argument,
                                const fs::path &workingDir,
                                const CollectionUtils::MapFileTo<fs::path> &dependencies,
                                const BuildDatabase::TargetInfo &linkUnitInfo,
                                const fs::path &output);

    std::vector<utbot::LinkCommand>
    getLinkActionsForExecutable(const fs::path &workingDir,
                                const CollectionUtils::MapFileTo<fs::path> &dependencies,
                                const BuildDatabase::TargetInfo &linkUnitInfo,
                                const fs::path &output,
                                bool shouldChangeDirectory = true);
};


#endif //UNITTESTBOT_LINKER_H
