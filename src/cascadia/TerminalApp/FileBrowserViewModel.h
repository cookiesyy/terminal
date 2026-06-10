#pragma once
#include "FileBrowserViewModel.g.h"
#include <filesystem>

namespace winrt::TerminalApp::implementation
{
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
        winrt::hstring _FormatFileSize(uint64_t bytes);
    };
}
