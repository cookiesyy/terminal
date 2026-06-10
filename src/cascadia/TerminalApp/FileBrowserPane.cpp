#include "pch.h"
#include "FileBrowserPane.h"
#include "FileBrowserPane.g.cpp"

namespace winrt::TerminalApp::implementation
{
    FileBrowserPane::FileBrowserPane()
    {
        InitializeComponent();
        _viewModel = winrt::make<FileBrowserViewModel>();
    }

    void FileBrowserPane::NavigateUpButton_Click(const Windows::Foundation::IInspectable&,
                                                  const Windows::UI::Xaml::RoutedEventArgs&)
    {
        _viewModel.NavigateUp();
    }

    void FileBrowserPane::FileListView_ItemClick(const Windows::Foundation::IInspectable&,
                                                  const Windows::UI::Xaml::Controls::ItemClickEventArgs& e)
    {
        auto item = e.ClickedItem().as<TerminalApp::FileItem>();

        if (item.IsDirectory())
        {
            _viewModel.LoadDirectory(item.Path());
        }
        else
        {
            _viewModel.OpenFile(item.Path());
        }
    }
}
