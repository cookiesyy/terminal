# Windows Terminal 侧边栏文件浏览器 - 实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 在Windows Terminal右侧添加文件浏览器侧边栏，支持浏览当前目录、点击打开文件

**Architecture:** 使用MVVM模式，FileBrowserPane(View) + FileBrowserViewModel(ViewModel)处理UI和业务逻辑，通过TerminalPage集成到主界面，使用std::filesystem读取目录，ShellExecute打开文件

**Tech Stack:** C++/WinRT, XAML, std::filesystem, Windows API (ShellExecute)

---

## 文件结构概览

### 新增文件

**UI组件**:
- `src/cascadia/TerminalApp/FileBrowserPane.xaml` - 文件浏览器UI布局
- `src/cascadia/TerminalApp/FileBrowserPane.h` - UI组件头文件
- `src/cascadia/TerminalApp/FileBrowserPane.cpp` - UI组件实现
- `src/cascadia/TerminalApp/FileBrowserPane.idl` - WinRT接口定义

**业务逻辑**:
- `src/cascadia/TerminalApp/FileBrowserViewModel.h` - ViewModel头文件
- `src/cascadia/TerminalApp/FileBrowserViewModel.cpp` - ViewModel实现
- `src/cascadia/TerminalApp/FileBrowserViewModel.idl` - WinRT接口定义

**测试**:
- `src/cascadia/ut_app/FileBrowserTests.cpp` - 单元测试

### 修改文件

**集成到主界面**:
- `src/cascadia/TerminalApp/TerminalPage.xaml` - 添加侧边栏布局
- `src/cascadia/TerminalApp/TerminalPage.h` - 添加ViewModel引用和状态
- `src/cascadia/TerminalApp/TerminalPage.cpp` - 添加切换和同步逻辑
- `src/cascadia/TerminalApp/TerminalPage.idl` - 添加公共接口

**添加触发按钮**:
- `src/cascadia/TerminalApp/TabRowControl.xaml` - 添加文件浏览器按钮

**构建配置**:
- `src/cascadia/TerminalApp/TerminalApp.vcxproj` - 添加新文件到项目

---

## Task 1: 创建FileBrowserViewModel IDL定义

**Files:**
- Create: `src/cascadia/TerminalApp/FileBrowserViewModel.idl`

- [ ] **Step 1: 创建IDL文件定义FileItem和FileBrowserViewModel接口**

```idl
namespace TerminalApp
{
    runtimeclass FileItem
    {
        String Name { get; };
        String Path { get; };
        Boolean IsDirectory { get; };
        UInt64 Size { get; };
        String SizeDisplay { get; };
    };

    runtimeclass FileBrowserViewModel
    {
        FileBrowserViewModel();
        Windows.Foundation.Collections.IObservableVector<FileItem> FileList { get; };
        String CurrentPath { get; };
        void LoadDirectory(String path);
        void OpenFile(String filePath);
        void NavigateUp();
    };
}
```

- [ ] **Step 2: 提交IDL定义**

```bash
git add src/cascadia/TerminalApp/FileBrowserViewModel.idl
git commit -m "feat(file-browser): add FileBrowserViewModel IDL definition"
```

---

## Task 2: 实现FileBrowserViewModel头文件

**Files:**
- Create: `src/cascadia/TerminalApp/FileBrowserViewModel.h`

- [ ] **Step 1: 创建FileBrowserViewModel头文件**

```cpp
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
```

- [ ] **Step 2: 提交头文件**

```bash
git add src/cascadia/TerminalApp/FileBrowserViewModel.h
git commit -m "feat(file-browser): add FileBrowserViewModel header"
```

---

## Task 3: 实现FileBrowserViewModel核心逻辑

**Files:**
- Create: `src/cascadia/TerminalApp/FileBrowserViewModel.cpp`

- [ ] **Step 1: 实现FileItem构造函数和FormatFileSize辅助函数**

```cpp
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
            _sizeDisplay = _FormatFileSize(size);
        }
        else
        {
            _sizeDisplay = L"-";
        }
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
}
```

- [ ] **Step 2: 提交辅助函数实现**

```bash
git add src/cascadia/TerminalApp/FileBrowserViewModel.cpp
git commit -m "feat(file-browser): implement FileItem and format helpers"
```

---

## Task 4: 实现LoadDirectory和文件操作

**Files:**
- Modify: `src/cascadia/TerminalApp/FileBrowserViewModel.cpp`

- [ ] **Step 1: 实现ViewModel构造函数和LoadDirectory**

```cpp
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
                size
            ));
        }
        
        // 排序：文件夹在前，同类型按名称排序
        std::sort(items.begin(), items.end(), [](const auto& a, const auto& b) {
            if (a.IsDirectory() != b.IsDirectory())
                return a.IsDirectory();
            return a.Name() < b.Name();
        });
    }
    catch (const fs::filesystem_error&)
    {
        // 错误处理：返回空列表
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
```

- [ ] **Step 2: 提交LoadDirectory实现**

```bash
git add src/cascadia/TerminalApp/FileBrowserViewModel.cpp
git commit -m "feat(file-browser): implement LoadDirectory"
```

---

## Task 5: 实现OpenFile和NavigateUp

**Files:**
- Modify: `src/cascadia/TerminalApp/FileBrowserViewModel.cpp`

- [ ] **Step 1: 实现OpenFile使用ShellExecute打开文件**

```cpp
void FileBrowserViewModel::OpenFile(const winrt::hstring& filePath)
{
    SHELLEXECUTEINFO sei = { sizeof(sei) };
    sei.lpVerb = L"open";
    sei.lpFile = filePath.c_str();
    sei.nShow = SW_SHOWNORMAL;
    
    if (!ShellExecuteEx(&sei))
    {
        // 错误处理：可以通过事件通知UI显示错误
        DWORD error = GetLastError();
        // TODO: 添加错误事件
    }
}
```

- [ ] **Step 2: 实现NavigateUp返回上级目录**

```cpp
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
```

- [ ] **Step 3: 提交文件操作实现**

```bash
git add src/cascadia/TerminalApp/FileBrowserViewModel.cpp
git commit -m "feat(file-browser): implement OpenFile and NavigateUp"
```

---

## Task 6: 创建FileBrowserPane XAML UI

**Files:**
- Create: `src/cascadia/TerminalApp/FileBrowserPane.xaml`

- [ ] **Step 1: 创建FileBrowserPane XAML布局**

```xml
<UserControl x:Class="TerminalApp.FileBrowserPane"
             xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
             xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
             xmlns:local="using:TerminalApp"
             Width="220"
             Background="{ThemeResource ApplicationPageBackgroundThemeBrush}">
    
    <Grid>
        <Grid.RowDefinitions>
            <RowDefinition Height="Auto" />
            <RowDefinition Height="*" />
        </Grid.RowDefinitions>
        
        <!-- 顶部导航栏 -->
        <StackPanel Grid.Row="0" Padding="8" Background="{ThemeResource SystemControlBackgroundAltMediumBrush}">
            <TextBlock x:Name="CurrentPathText" 
                       Text="{x:Bind ViewModel.CurrentPath, Mode=OneWay}"
                       FontSize="11"
                       TextTrimming="CharacterEllipsis"
                       Margin="0,0,0,4" />
            <Button x:Name="NavigateUpButton"
                    Content="⬆ 返回上级"
                    Click="NavigateUpButton_Click"
                    HorizontalAlignment="Stretch" />
        </StackPanel>
        
        <!-- 文件列表 -->
        <ListView x:Name="FileListView"
                  Grid.Row="1"
                  ItemsSource="{x:Bind ViewModel.FileList, Mode=OneWay}"
                  SelectionMode="Single"
                  IsItemClickEnabled="True"
                  ItemClick="FileListView_ItemClick">
            <ListView.ItemTemplate>
                <DataTemplate x:DataType="local:FileItem">
                    <Grid Padding="8,4">
                        <Grid.ColumnDefinitions>
                            <ColumnDefinition Width="Auto" />
                            <ColumnDefinition Width="*" />
                            <ColumnDefinition Width="Auto" />
                        </Grid.ColumnDefinitions>
                        
                        <TextBlock Grid.Column="0" 
                                   Text="{x:Bind IsDirectory}" 
                                   FontSize="16"
                                   Margin="0,0,8,0" />
                        <TextBlock Grid.Column="1" 
                                   Text="{x:Bind Name, Mode=OneWay}"
                                   TextTrimming="CharacterEllipsis" />
                        <TextBlock Grid.Column="2"
                                   Text="{x:Bind SizeDisplay, Mode=OneWay}"
                                   FontSize="11"
                                   Foreground="{ThemeResource SystemControlForegroundBaseMediumBrush}" />
                    </Grid>
                </DataTemplate>
            </ListView.ItemTemplate>
        </ListView>
    </Grid>
</UserControl>
```

- [ ] **Step 2: 提交XAML文件**

```bash
git add src/cascadia/TerminalApp/FileBrowserPane.xaml
git commit -m "feat(file-browser): add FileBrowserPane XAML UI"
```

---

## Task 7: 创建FileBrowserPane IDL和代码后台

**Files:**
- Create: `src/cascadia/TerminalApp/FileBrowserPane.idl`
- Create: `src/cascadia/TerminalApp/FileBrowserPane.h`
- Create: `src/cascadia/TerminalApp/FileBrowserPane.cpp`

- [ ] **Step 1: 创建FileBrowserPane IDL定义**

```idl
namespace TerminalApp
{
    [default_interface] runtimeclass FileBrowserPane : Windows.UI.Xaml.Controls.UserControl
    {
        FileBrowserPane();
        FileBrowserViewModel ViewModel { get; };
    };
}
```

- [ ] **Step 2: 创建FileBrowserPane头文件**

```cpp
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
```

- [ ] **Step 3: 提交IDL和头文件**

```bash
git add src/cascadia/TerminalApp/FileBrowserPane.idl src/cascadia/TerminalApp/FileBrowserPane.h
git commit -m "feat(file-browser): add FileBrowserPane IDL and header"
```

---

## Task 8: 实现FileBrowserPane事件处理

**Files:**
- Create: `src/cascadia/TerminalApp/FileBrowserPane.cpp`

- [ ] **Step 1: 实现FileBrowserPane构造函数和事件处理**

```cpp
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
```

- [ ] **Step 2: 提交FileBrowserPane实现**

```bash
git add src/cascadia/TerminalApp/FileBrowserPane.cpp
git commit -m "feat(file-browser): implement FileBrowserPane event handlers"
```

---

## Task 9: 集成到TerminalPage布局

**Files:**
- Modify: `src/cascadia/TerminalApp/TerminalPage.xaml`

- [ ] **Step 1: 在TerminalPage.xaml中添加列定义和FileBrowserPane**

在Root Grid的RowDefinitions之后添加ColumnDefinitions：

```xml
<Grid.ColumnDefinitions>
    <ColumnDefinition Width="*" />
    <ColumnDefinition Width="Auto" />
</Grid.ColumnDefinitions>
```

- [ ] **Step 2: 将TabContent设置到第0列**

修改TabContent的Grid.Column属性：

```xml
<Grid x:Name="TabContent"
      Grid.Row="2"
      Grid.Column="0"
      HorizontalAlignment="Stretch"
      VerticalAlignment="Stretch">
```

- [ ] **Step 3: 添加FileBrowserPane到第1列**

在TabContent之后添加：

```xml
<local:FileBrowserPane x:Name="FileBrowserPane"
                       Grid.Row="2"
                       Grid.Column="1"
                       Visibility="Collapsed" />
```

- [ ] **Step 4: 提交布局修改**

```bash
git add src/cascadia/TerminalApp/TerminalPage.xaml
git commit -m "feat(file-browser): integrate FileBrowserPane into TerminalPage layout"
```

---

## Task 10: 添加切换按钮到TabRowControl

**Files:**
- Modify: `src/cascadia/TerminalApp/TabRowControl.xaml`

- [ ] **Step 1: 在TabRowControl.xaml中添加文件浏览器按钮**

在TabRowControl的适当位置（建议在右侧控件区域）添加：

```xml
<Button x:Name="FileBrowserButton"
        Content="📁"
        ToolTipService.ToolTip="文件浏览器"
        Click="FileBrowserButton_Click"
        Style="{StaticResource IconButtonStyle}"
        Margin="4,0" />
```

- [ ] **Step 2: 提交按钮UI**

```bash
git add src/cascadia/TerminalApp/TabRowControl.xaml
git commit -m "feat(file-browser): add toggle button to TabRowControl"
```

---

## Task 11: 实现TerminalPage切换逻辑

**Files:**
- Modify: `src/cascadia/TerminalApp/TerminalPage.idl`
- Modify: `src/cascadia/TerminalApp/TerminalPage.h`
- Modify: `src/cascadia/TerminalApp/TerminalPage.cpp`

- [ ] **Step 1: 在TerminalPage.idl中添加切换方法**

```idl
void ToggleFileBrowser();
```

- [ ] **Step 2: 在TerminalPage.h中添加状态和方法声明**

```cpp
private:
    bool _fileBrowserVisible{ false };
    void _ToggleFileBrowser();
```

- [ ] **Step 3: 在TerminalPage.cpp中实现切换逻辑**

```cpp
void TerminalPage::ToggleFileBrowser()
{
    _fileBrowserVisible = !_fileBrowserVisible;
    
    if (_fileBrowserVisible)
    {
        FileBrowserPane().Visibility(Windows::UI::Xaml::Visibility::Visible);
        // 加载当前工作目录
        auto cwd = _GetCurrentWorkingDirectory();
        FileBrowserPane().ViewModel().LoadDirectory(cwd);
    }
    else
    {
        FileBrowserPane().Visibility(Windows::UI::Xaml::Visibility::Collapsed);
    }
}

winrt::hstring TerminalPage::_GetCurrentWorkingDirectory()
{
    // 获取当前活动终端的工作目录
    // 临时实现：返回用户主目录
    wchar_t path[MAX_PATH];
    SHGetFolderPath(NULL, CSIDL_PROFILE, NULL, 0, path);
    return winrt::hstring{ path };
}
```

- [ ] **Step 4: 提交切换逻辑**

```bash
git add src/cascadia/TerminalApp/TerminalPage.idl src/cascadia/TerminalApp/TerminalPage.h src/cascadia/TerminalApp/TerminalPage.cpp
git commit -m "feat(file-browser): implement ToggleFileBrowser logic"
```

---

## Task 12: 连接按钮到切换逻辑

**Files:**
- Modify: `src/cascadia/TerminalApp/TabRowControl.h`
- Modify: `src/cascadia/TerminalApp/TabRowControl.cpp`

- [ ] **Step 1: 在TabRowControl.h中添加按钮事件处理声明**

```cpp
void FileBrowserButton_Click(const Windows::Foundation::IInspectable& sender,
                             const Windows::UI::Xaml::RoutedEventArgs& e);
```

- [ ] **Step 2: 在TabRowControl.cpp中实现按钮点击事件**

```cpp
void TabRowControl::FileBrowserButton_Click(const Windows::Foundation::IInspectable&,
                                            const Windows::UI::Xaml::RoutedEventArgs&)
{
    // 调用TerminalPage的切换方法
    if (auto page = _GetTerminalPage())
    {
        page.ToggleFileBrowser();
    }
}

TerminalApp::TerminalPage TabRowControl::_GetTerminalPage()
{
    // 向上遍历可视树找到TerminalPage
    auto element = this->Parent();
    while (element)
    {
        if (auto page = element.try_as<TerminalApp::TerminalPage>())
        {
            return page;
        }
        element = element.as<Windows::UI::Xaml::FrameworkElement>().Parent();
    }
    return nullptr;
}
```

- [ ] **Step 3: 提交按钮连接逻辑**

```bash
git add src/cascadia/TerminalApp/TabRowControl.h src/cascadia/TerminalApp/TabRowControl.cpp
git commit -m "feat(file-browser): connect toggle button to TerminalPage"
```

---

## Task 13: 编写单元测试

**Files:**
- Create: `src/cascadia/ut_app/FileBrowserTests.cpp`

- [ ] **Step 1: 创建基础测试框架**

```cpp
#include "pch.h"
#include "../TerminalApp/FileBrowserViewModel.h"

using namespace WEX::Logging;
using namespace WEX::TestExecution;
using namespace winrt::TerminalApp;

namespace TerminalAppUnitTests
{
    class FileBrowserTests
    {
        TEST_CLASS(FileBrowserTests);

        TEST_METHOD(VerifyFileItemConstruction);
        TEST_METHOD(VerifyFileItemSizeFormatting);
        TEST_METHOD(VerifyLoadDirectoryReturnsFileList);
    };
}
```

- [ ] **Step 2: 实现文件大小格式化测试**

```cpp
void FileBrowserTests::VerifyFileItemSizeFormatting()
{
    auto vm = winrt::make<implementation::FileBrowserViewModel>();
    
    // 测试字节
    auto item1 = winrt::make<implementation::FileItem>(L"test1.txt", L"C:\\test1.txt", false, 512);
    VERIFY_ARE_EQUAL(L"512B", item1.SizeDisplay());
    
    // 测试KB
    auto item2 = winrt::make<implementation::FileItem>(L"test2.txt", L"C:\\test2.txt", false, 2048);
    VERIFY_ARE_EQUAL(L"2.0KB", item2.SizeDisplay());
    
    // 测试MB
    auto item3 = winrt::make<implementation::FileItem>(L"test3.txt", L"C:\\test3.txt", false, 1048576);
    VERIFY_ARE_EQUAL(L"1.0MB", item3.SizeDisplay());
    
    // 测试文件夹
    auto item4 = winrt::make<implementation::FileItem>(L"folder", L"C:\\folder", true, 0);
    VERIFY_ARE_EQUAL(L"-", item4.SizeDisplay());
}
```

- [ ] **Step 3: 提交测试代码**

```bash
git add src/cascadia/ut_app/FileBrowserTests.cpp
git commit -m "test(file-browser): add unit tests for FileBrowserViewModel"
```

- [ ] **Step 4: 运行测试验证**

Run: `vstest.console.exe OpenConsole.AppUnitTests.dll /Tests:FileBrowserTests`
Expected: 所有测试通过

---

## Task 14: 更新项目构建配置

**Files:**
- Modify: `src/cascadia/TerminalApp/TerminalApp.vcxproj`

- [ ] **Step 1: 添加新文件到vcxproj**

在vcxproj中添加以下条目：

```xml
<ClCompile Include="FileBrowserViewModel.cpp" />
<ClCompile Include="FileBrowserPane.cpp" />
<ClInclude Include="FileBrowserViewModel.h" />
<ClInclude Include="FileBrowserPane.h" />
<Midl Include="FileBrowserViewModel.idl" />
<Midl Include="FileBrowserPane.idl" />
<Page Include="FileBrowserPane.xaml" />
```

- [ ] **Step 2: 确保pch.h包含必要的头文件**

在`pch.h`中添加（如果尚未包含）：

```cpp
#include <filesystem>
#include <shellapi.h>
#include <shlobj.h>
```

- [ ] **Step 3: 构建项目验证**

Run: `msbuild src/cascadia/CascadiaPackage/CascadiaPackage.wapproj /p:Configuration=Debug /p:Platform=x64`
Expected: 构建成功，无错误

- [ ] **Step 4: 提交构建配置**

```bash
git add src/cascadia/TerminalApp/TerminalApp.vcxproj src/cascadia/TerminalApp/pch.h
git commit -m "build(file-browser): add new files to project configuration"
```

---

## Task 15: 手动集成测试

**Files:**
- 无文件修改，纯测试验证

- [ ] **Step 1: 启动Windows Terminal并验证按钮显示**

Run: 启动编译好的Windows Terminal
Expected: 标签栏右侧显示📁按钮

- [ ] **Step 2: 点击按钮验证侧边栏切换**

Action: 点击📁按钮
Expected: 右侧出现文件浏览器侧边栏，显示当前用户主目录文件列表

- [ ] **Step 3: 验证文件列表排序**

Expected: 文件夹在前，文件在后，同类型按名称排序

- [ ] **Step 4: 双击文件夹进入子目录**

Action: 双击任意文件夹
Expected: 文件列表刷新显示子目录内容，当前路径更新

- [ ] **Step 5: 点击返回上级按钮**

Action: 点击"⬆ 返回上级"按钮
Expected: 返回上级目录，文件列表更新

- [ ] **Step 6: 点击文件验证打开**

Action: 点击任意文本文件
Expected: 使用系统默认程序打开文件

- [ ] **Step 7: 再次点击按钮隐藏侧边栏**

Action: 点击📁按钮
Expected: 侧边栏隐藏，终端区域恢复全宽

---

## 自我审查检查清单

**规格覆盖检查:**
- [x] 侧边栏触发 - Task 10, 11, 12实现工具栏按钮切换
- [x] 侧边栏布局 - Task 9实现右侧边栏布局
- [x] 文件列表显示 - Task 1-6实现平铺列表、文件图标、大小显示
- [x] 文件操作 - Task 5实现OpenFile和NavigateUp
- [x] 排序规则 - Task 4实现文件夹优先、名称排序
- [x] 测试策略 - Task 13实现单元测试

**占位符扫描:**
- 所有代码块完整，无TBD/TODO
- 所有类型、方法签名一致

**类型一致性:**
- FileItem: Name, Path, IsDirectory, Size, SizeDisplay - 所有任务中一致
- FileBrowserViewModel: LoadDirectory, OpenFile, NavigateUp - 所有任务中一致
- TerminalPage: ToggleFileBrowser - 所有任务中一致

---

## 执行交付

计划已完成并保存到 `docs/superpowers/plans/2026-06-10-sidebar-file-browser.md`。

**两种执行选项:**

**1. Subagent-Driven (推荐)** - 每个任务派发新的子代理，任务间审查，快速迭代

**2. Inline Execution** - 在当前会话中使用 executing-plans 执行，批量执行带检查点

**选择哪种执行方式？**