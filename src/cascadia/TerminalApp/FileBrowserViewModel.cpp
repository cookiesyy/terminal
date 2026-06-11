#include "pch.h"
#include "FileBrowserViewModel.h"
#include "FileBrowserViewModel.g.cpp"

namespace winrt::TerminalApp::implementation
{
    FileItem::FileItem(const winrt::hstring& name, const winrt::hstring& path, bool isDir, uint64_t size) :
        _name{ name },
        _path{ path },
        _isDirectory{ isDir },
        _size{ size }
    {
        if (!isDir)
        {
            _sizeDisplay = FileBrowserViewModel::_FormatFileSize(size);
        }
        else
        {
            _sizeDisplay = L"-";
        }
    }

    FileBrowserViewModel::FileBrowserViewModel()
    {
        _fileList = winrt::single_threaded_observable_vector<TerminalApp::FileItem>();
        _currentPath = L"";
    }

    winrt::hstring FileBrowserViewModel::_FormatFileSize(uint64_t bytes)
    {
        const wchar_t* units[] = { L"B", L"KB", L"MB", L"GB", L"TB" };
        int unitIndex = 0;
        double size = static_cast<double>(bytes);

        while (size >= 1024.0 && unitIndex < 4)
        {
            size /= 1024.0;
            unitIndex++;
        }

        wchar_t buffer[32];
        if (unitIndex == 0)
        {
            swprintf_s(buffer, 32, L"%d%s", static_cast<int>(size), units[unitIndex]);
        }
        else
        {
            swprintf_s(buffer, 32, L"%.1f%s", size, units[unitIndex]);
        }
        return winrt::hstring{ buffer };
    }

    std::vector<TerminalApp::FileItem> FileBrowserViewModel::_LoadDirectoryImpl(const std::wstring& path)
    {
        std::vector<TerminalApp::FileItem> items;
        namespace fs = std::filesystem;

        try
        {
            for (const auto& entry : fs::directory_iterator(path))
            {
                auto filename = entry.path().filename().wstring();
                auto fullPath = entry.path().wstring();
                bool isDir = entry.is_directory();
                uint64_t size = 0;

                if (!isDir)
                {
                    size = fs::file_size(entry.path());
                }

                items.push_back(winrt::make<FileItem>(
                    winrt::hstring{ filename },
                    winrt::hstring{ fullPath },
                    isDir,
                    size));
            }

            std::sort(items.begin(), items.end(), [](const auto& a, const auto& b) {
                if (a.IsDirectory() != b.IsDirectory())
                    return a.IsDirectory();
                return a.Name() < b.Name();
            });
        }
        catch (const fs::filesystem_error&)
        {
        }

        return items;
    }

    void FileBrowserViewModel::LoadDirectory(const winrt::hstring& path)
    {
        _currentPath = path;
        auto items = _LoadDirectoryImpl(path.c_str());

        _fileList.Clear();
        for (const auto& item : items)
        {
            _fileList.Append(item);
        }
    }

    void FileBrowserViewModel::OpenFile(const winrt::hstring& filePath)
    {
        SHELLEXECUTEINFO sei = { sizeof(sei) };
        sei.lpVerb = L"open";
        sei.lpFile = filePath.c_str();
        sei.nShow = SW_SHOWNORMAL;

        if (!ShellExecuteEx(&sei))
        {
            DWORD error = GetLastError();
        }
    }

    void FileBrowserViewModel::NavigateUp()
    {
        namespace fs = std::filesystem;

        if (_currentPath.empty())
            return;

        fs::path current{ _currentPath.c_str() };
        if (current.has_parent_path())
        {
            auto parent = current.parent_path().wstring();
            LoadDirectory(winrt::hstring{ parent });
        }
    }
}
