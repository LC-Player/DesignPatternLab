#include <iostream>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <memory>

#include "Editor.h"
#include "Workspace.h"

void TestCommand();
void TestEditor();
void TestWorkspace();
void TestTreeDrawer();
void TestLogger();

int main() {
    std::cout << "  ######## Starting tests ########" << std::endl << std::endl;

    TestCommand();
    TestLogger();
    TestEditor();
    TestWorkspace();

    std::cout << " ######## All tests passed! ########" << std::endl << std::endl;
}

void TestCommand() {
    std::cout << "======== Testing Command ========" << std::endl;

    Command emptyCommand("  ");
    assert(!emptyCommand.Validate());
    std::cout << "Passed: empty command" << std::endl;

    Command illegalCommand("Wow! Command");
    assert(!illegalCommand.Validate());
    std::cout << "Passed: illegal command" << std::endl;

    Command wrongArgCommand("init");
    assert(!wrongArgCommand.Validate());
    std::cout << "Passed: command with wrong argument" << std::endl;

    Command replaceCommand("replace 3:5 2 \"hello  world!\"");
    assert(replaceCommand.Validate());
    std::cout << "Passed: replace command with 3 arguments and quotes" << std::endl;

    Command logCommand("log-show");
    assert(logCommand.GetType() == Command::Type::LogShow);
    assert(logCommand.GetHandler() == HandlerType::Workspace);
    std::cout << "Passed: log-show command with type check" << std::endl;

    std::cout << "======== End of Command Tests =========" << std::endl << std::endl;
}

void TestLogger() {
    std::cout << "======== Testing Logger ========" << std::endl;
    Logger* testLogger = new Logger(".test.log");
    Command command("show");
    testLogger->Log(command);
    assert(testLogger->GetBuffer().find("session start at ") != std::string::npos);
    assert(testLogger->GetBuffer().find("show") != std::string::npos);
    testLogger->Save();
    delete testLogger;
    std::cout << "Passed: logger" << std::endl;

    Logger* existingFileLogger = new Logger(".test.log");
    Command initCommand("init a");
    existingFileLogger->Log(initCommand);
    existingFileLogger->Save();
    delete existingFileLogger;
    std::ifstream logFile(".test.log");
    std::stringstream buffer;
    buffer << logFile.rdbuf();
    assert(buffer.str().find("show") != std::string::npos);
    assert(buffer.str().find("init") != std::string::npos);
    logFile.close();
    std::cout << "Passed: logger with exising file" << std::endl;

    std::filesystem::remove((".test.log"));

    std::cout << "======== End of Logger Testing ========" << std::endl << std::endl;
}

void TestEditor() {
    std::cout << "======== Testing Editor ========" << std::endl;
    Ref<Editor> emptyFileEditor = CreateRef<Editor>("testfile/emptyfile");
    assert(emptyFileEditor->GetLines().empty());
    assert(emptyFileEditor->GetLogMode() != LogMode::WithLog);
    assert(emptyFileEditor->IsModified() == false);
    std::cout << "Passed: open existing empty file in Editor" << std::endl;

    Ref<Editor> logFileEditor = CreateRef<Editor>("testfile/logstatedfile");
    assert(logFileEditor->GetLines().size() == 2);
    assert(logFileEditor->GetLogMode() == LogMode::WithLog);
    std::cout << "Passed: open existing `# log` file in Editor" << std::endl;

    Ref<Editor> tempFileEditor = CreateRef<Editor>("testfile/tempeditorfile");
    std::cout << "Passed: open new file" << std::endl;

    Command appendCommand("append \"append test!\"");
    tempFileEditor->Handle(appendCommand);
    assert(tempFileEditor->GetLines()[0] == "append test!");
    std::cout << "Passed: append" << std::endl;
    Command replaceCommand("replace 1:8 4 replace");
    tempFileEditor->Handle(replaceCommand);
    assert(tempFileEditor->GetLines()[0] == "append replace!");
    std::cout << "Passed: replace" << std::endl;

    tempFileEditor->SetLogMode(LogMode::WithLog);

    Command insertCommand("insert 1:1 insert\\ninsert\\n");
    tempFileEditor->Handle(insertCommand);
    assert(tempFileEditor->GetLines()[0] == "insert");
    assert(tempFileEditor->GetLines()[1] == "insert");
    std::cout << "Passed: insert" << std::endl;

    Command undoCommand("undo");
    tempFileEditor->Handle(undoCommand);
    assert(tempFileEditor->GetLines()[0] == "append replace!");
    std::cout << "Passed: undo" << std::endl;

    Command redoCommand("redo");
    tempFileEditor->Handle(redoCommand);
    assert(tempFileEditor->GetLines()[0] == "insert");
    assert(tempFileEditor->GetLines()[1] == "insert");
    std::cout << "Passed: redo" << std::endl;

    assert(tempFileEditor->GetLogger()->GetBuffer().find("insert") != std::string::npos);
    assert(tempFileEditor->GetLogger()->GetBuffer().find("append") == std::string::npos);
    std::cout << "Passed: editor log mode and logging";

    tempFileEditor->Save();
    assert(!tempFileEditor->IsModified());
    std::cout << "Passed: saving";

    std::filesystem::remove("testfile/tempeditorfile");

    std::cout << "======== End of Editor Testing ========" << std::endl << std::endl;
}

void TestWorkspace() {
    std::cout << "======== Testing Workspace ========" << std::endl;

    Ref<Workspace> testWorkspace = CreateRef<Workspace>("");
    assert(testWorkspace->GetCurrentEditor() == nullptr);
    std::cout << "Passed: create empty workspace" << std::endl;

    Command loadCommand("load testfile/logstatedfile");
    testWorkspace->Handle(loadCommand);
    assert(testWorkspace->GetCurrentEditorIndex() == 0);
    assert(testWorkspace->GetLogMode() == LogMode::WithLog);
    std::cout << "Passed: Open existing file" << std::endl;

    Command initCommand("init testfile/tempnewdir/workspacetempfile");
    testWorkspace->Handle(initCommand);
    assert(testWorkspace->GetCurrentEditorIndex() == 1);
    std::cout << "Passed: Init new file" << std::endl;

    Command saveCommand("save");
    testWorkspace->Handle(saveCommand);
    Command exitCommand("exit");
    testWorkspace->Handle(exitCommand);
    assert(std::filesystem::exists("data/.editor_workspace"));
    std::cout << "Passed: saving status of workspace" << std::endl;

    testWorkspace.reset();

    std::string inData = R"({
        "current_editor": 0,
        "editors": [
            {
                "log_mode": 1,
                "modified": false,
                "path": "testfile/tempnewdir/workspacetempfile"
            }
        ],
        "log_mode": 1
    })";

    Ref<Workspace> workspaceWithData = CreateRef<Workspace>(inData);
    assert(workspaceWithData->GetCurrentEditorIndex() == 0);
    assert(workspaceWithData->GetLogMode() == LogMode::WithLog);
    std::cout << "Passed: workspace with init data" << std::endl;

    workspaceWithData.reset();

    std::filesystem::remove("testfile/tempnewdir/workspacetempfile");
    std::filesystem::remove("testfile/tempnewdir/.workspacetempfile.log");
    std::filesystem::remove("testfile/.emptyfile.log");
    std::filesystem::remove("testfile/.logstatedfile.log");
    std::filesystem::remove("testfile/.tempeditorfile.log");
    std::filesystem::remove("testfile/tempnewdir");

    std::cout << "======== End of Workspace Testing ========" << std::endl << std::endl;
}