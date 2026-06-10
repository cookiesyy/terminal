#pragma once
#include "FileBrowserPane.g.h"
#include "FileBrowserViewModel.h"

namespace winrt::TerminalApp::implementation
{
    struct FileBrowserPane : FileBrowserPaneT<FileBrowserPane>
    {
        FileBrowserPane();

        TerminalApp::FileBrowserViewModel ViewModel() { return _viewModel; }

        void NavigateUpButton_Click(const Windows::Foundation::IInspectable& sender,
                                    const Windows::UI::Xaml::RoutedEventArgs& e);
        void FileListView_ItemClick(const Windows::Foundation::IInspectable& sender,
                                    const Windows::UI::Xaml::Controls::ItemClickEventArgs& e);

    private:
        TerminalApp::FileBrowserViewModel _viewModel{ nullptr };
    };
}

namespace winrt::TerminalApp::factory_implementation
{
    struct FileBrowserPane : FileBrowserPaneT<FileBrowserPane, implementation::FileBrowserPane>
    {
    };
}
