#pragma once

namespace MCP {

    void Register();

    // For debug purposes
    void __stdcall RenderFrameworkTools();
    void __stdcall RenderLoadedConfigs();
    void __stdcall RenderLog();

    inline std::vector<std::string> logLines;

};

namespace MCPLog {
    std::filesystem::path GetLogPath();
    std::vector<std::string> ReadLogFile();

    inline bool log_trace = true;
    inline bool log_info = true;
    inline bool log_warning = true;
    inline bool log_error = true;
    inline char custom[255];
};