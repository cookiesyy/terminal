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
            _sizeDisplay = FormatFileSize(size);
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
            // Failed to open file
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
