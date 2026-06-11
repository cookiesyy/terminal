#pragma once
#include "FileItem.g.h"
#include "FileBrowserViewModel.g.h"
#include <filesystem>

namespace winrt::TerminalApp::implementation
{
    // Helper function for formatting file sizes
    inline winrt::hstring FormatFileSize(uint64_t bytes)
    {
        const wchar_t* units[] = { L"B", L"KB", L"MB", L"GB", L"TB" };
        int unitIndex = 0;
        double size = static_cast<double>(bytes);

        while (size >= 1024.0 && unitIndex < 4)
        {
            size /= 1024.0;
            unitIndex++;
        }

        wchar_t buffer[64];
        swprintf_s(buffer, L"%.2f %s", size, units[unitIndex]);
        return winrt::hstring{ buffer };
    }

    struct FileItem : FileItemT<FileItem>
    {
        FileItem(const winrt::hstring& name, const winrt::hstring& path, bool isDir, uint64_t size);

        winrt::hstring Name() { return _name; }
        winrt::hstring Path() { return _path; }
        bool IsDirectory() { return _isDirectory; }
        uint64_t Size() { return _size; }
        winrt::hstring SizeDisplay() { return _sizeDisplay; }

    private:
        winrt::hstring _name;
        winrt::hstring _path;
        bool _isDirectory;
        uint64_t _size;
        winrt::hstring _sizeDisplay;
    };

    struct FileBrowserViewModel : FileBrowserViewModelT<FileBrowserViewModel>
    {
        FileBrowserViewModel();

        Windows::Foundation::Collections::IObservableVector<TerminalApp::FileItem> FileList() { return _fileList; }
        winrt::hstring CurrentPath() { return _currentPath; }

        void LoadDirectory(const winrt::hstring& path);
        void OpenFile(const winrt::hstring& filePath);
        void NavigateUp();

    private:
        Windows::Foundation::Collections::IObservableVector<TerminalApp::FileItem> _fileList;
        winrt::hstring _currentPath;

        std::vector<TerminalApp::FileItem> _LoadDirectoryImpl(const std::wstring& path);
    };
}

namespace winrt::TerminalApp::factory_implementation
{
    BASIC_FACTORY(FileBrowserViewModel);
}
