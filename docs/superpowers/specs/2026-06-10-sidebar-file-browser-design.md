# Windows Terminal 侧边栏文件浏览器 - 设计文档

**日期**: 2026-06-10  
**作者**: 老王  
**状态**: 待审查

---

## 1. 概述

在Windows Terminal右侧添加文件浏览器侧边栏，使用户能够快速浏览当前目录下的文件和文件夹，并通过点击直接打开文件，提升命令行与文件管理的工作效率。

### 1.1 核心目标

- 提供便捷的文件浏览功能，无需离开终端界面
- 与终端工作流无缝集成
- 保持简洁的用户界面，不干扰终端使用

### 1.2 非目标

- ~~不实现文件预览功能（显示文件内容）~~
- ~~不提供文件编辑功能~~
- ~~不实现复杂的文件操作（复制、移动、删除等）~~

---

## 2. 功能需求

### 2.1 侧边栏触发

**触发方式**: 工具栏按钮

- 在Windows Terminal标题栏或标签栏区域添加"文件浏览器"按钮
- 按钮图标建议使用文件夹图标（📁）
- 点击按钮切换侧边栏显示/隐藏状态
- 侧边栏状态在会话期间持久化

**实现位置**: 
- 优先考虑标签栏右侧，与最小化/最大化/关闭按钮组对齐
- 备选方案：标题栏左侧，与新建标签页按钮相邻

### 2.2 侧边栏布局

**位置**: 右侧边栏

- 侧边栏固定显示在终端窗口右侧
- 终端内容区域在左，文件浏览器在右
- 布局结构：`[终端区域] | [文件浏览器]`

**宽度控制**: 自适应终端区域

- 终端区域优先，占据主要显示空间
- 侧边栏宽度根据窗口剩余空间自适应
- 建议最小宽度：180px
- 建议最大宽度：不超过窗口宽度的30%
- 默认宽度：220px或窗口宽度的25%（取较小值）

### 2.3 文件列表显示

**显示方式**: 平铺列表

- 仅显示当前工作目录（pwd）下的文件和文件夹
- 不展示树形结构，保持界面简洁
- 文件列表垂直排列，支持滚动

**目录导航**:
- 双击文件夹：进入该文件夹，刷新列表显示子目录内容
- 提供"返回上级"按钮（.. 或 ↑ 图标）
- 显示当前路径面包屑导航

**显示信息**:
- 文件/文件夹图标（区分类型）
- 文件/文件夹名称
- 文件大小（仅文件显示，文件夹显示"-"）

**排序规则**:
1. 文件夹优先显示在文件前
2. 同类型按名称字母顺序排序
3. 隐藏文件（以.开头）默认不显示，可选配置项显示

### 2.4 文件操作

**点击行为**:
- 点击文件：使用系统默认程序打开文件
- 双击文件夹：进入该文件夹

**打开方式实现**:
- Windows: 调用 `ShellExecute` 或 `start` 命令
- 传递完整文件路径给系统
- 错误处理：文件不存在或无权限时显示提示信息

**支持的文件类型**:
- 所有文件类型均可点击
- 系统根据文件扩展名关联默认程序
- 无关联程序的文件显示"选择打开方式"对话框

---

## 3. 技术架构

### 3.1 组件设计

基于Windows Terminal的模块化架构，需要新增和修改以下组件：

#### 3.1.1 UI组件（TerminalApp层）

**新增组件**: `FileBrowserPane`

文件路径: `/src/cascadia/TerminalApp/FileBrowserPane.xaml`

职责：
- 渲染文件浏览器UI
- 处理用户交互（点击、滚动）
- 显示文件列表和目录导航

XAML结构：
```
Grid (根容器)
├── StackPanel (顶部导航栏)
│   ├── TextBlock (当前路径)
│   └── Button (返回上级)
└── ListView (文件列表)
    └── ItemTemplate
        ├── Image (文件图标)
        ├── TextBlock (文件名)
        └── TextBlock (文件大小)
```

**修改组件**: `TerminalPage.xaml`

修改点：
- 在主Grid中添加新列定义用于侧边栏
- 当侧边栏显示时，将主内容区域（TabContent）和FileBrowserPane分别放入不同列
- 使用ColumnDefinition控制宽度分配

布局结构：
```
Grid (Root)
├── ColumnDefinition Width="*" (终端区域，自适应)
├── ColumnDefinition Width="Auto" (侧边栏，根据内容宽度)
├── TabContent (Grid.Column="0")
└── FileBrowserPane (Grid.Column="1", Visibility绑定到侧边栏状态)
```

**修改组件**: `TitlebarControl.xaml` 或 `TabRowControl.xaml`

修改点：
- 添加文件浏览器切换按钮
- 按钮Command绑定到ToggleFileBrowser命令

#### 3.1.2 业务逻辑组件

**新增类**: `FileBrowserViewModel` (C++/WinRT)

文件路径: `/src/cascadia/TerminalApp/FileBrowserViewModel.h/cpp`

职责：
- 管理文件列表数据
- 监听当前工作目录变化
- 处理文件系统操作（读取目录、打开文件）

核心方法：
- `LoadDirectory(winrt::hstring path)` - 加载指定目录文件列表
- `OpenFile(winrt::hstring filePath)` - 打开文件
- `NavigateUp()` - 返回上级目录
- `RefreshFileList()` - 刷新当前目录

数据模型：
```cpp
struct FileItem {
    winrt::hstring Name;
    winrt::hstring Path;
    bool IsDirectory;
    uint64_t Size;  // 文件大小（字节）
    winrt::hstring SizeDisplay;  // 格式化后的大小显示（如"2.5KB"）
};
```

**修改类**: `TerminalPage.h/cpp`

修改点：
- 添加 `_fileBrowserVisible` 状态变量
- 添加 `ToggleFileBrowser()` 方法
- 在构造函数中初始化FileBrowserViewModel
- 监听终端工作目录变化事件，同步更新文件浏览器

### 3.2 数据流

```
用户点击按钮
    ↓
TerminalPage.ToggleFileBrowser()
    ↓
更新 _fileBrowserVisible 状态
    ↓
FileBrowserPane Visibility 绑定更新
    ↓
FileBrowserViewModel.LoadDirectory(pwd)
    ↓
文件系统API读取目录
    ↓
更新 ObservableCollection<FileItem>
    ↓
ListView UI刷新显示
```

### 3.3 目录同步机制

**监听终端工作目录变化**:

方案1: 监听Shell命令输出（推荐）
- 监听 `cd` 命令执行
- 解析命令行输入，检测目录切换
- 更新FileBrowserViewModel的当前路径

方案2: 定时轮询
- 每秒查询当前Shell进程的工作目录
- 比较路径是否变化，变化则刷新文件列表
- 备选方案，实时性较差但实现简单

**实现位置**: 
- 在 `TerminalControl` 或 `ConptyConnection` 层添加目录变化事件
- TerminalPage订阅该事件，转发给FileBrowserViewModel

---

## 4. UI/UX设计

### 4.1 视觉设计

**配色方案**: 跟随Windows Terminal主题

- 背景色：使用Terminal主题的背景色或稍浅的变体
- 文字色：使用Terminal主题的前景色
- 选中项高亮：使用accentColor或主题高亮色
- 分割线：使用主题边框颜色

**字体**: 
- 使用Terminal配置的字体系列
- 字号略小于终端字号（便于显示更多内容）

**图标**:
- 文件夹：📁 或使用Windows系统图标
- 文件：根据扩展名显示不同图标（可选用Segoe MDL2 Assets字体图标）
- 返回上级：⬆️ 或 📂 带箭头

### 4.2 交互设计

**鼠标交互**:
- 悬停：文件项背景色高亮
- 单击文件：选中高亮
- 双击文件：打开文件
- 双击文件夹：进入文件夹
- 单击返回按钮：返回上级目录

**键盘导航**（可选增强）:
- ↑/↓：在文件列表中移动选择
- Enter：打开选中项
- Backspace：返回上级目录

### 4.3 响应式设计

**窗口宽度自适应**:
- 窗口宽度 < 800px：侧边栏最小宽度180px
- 窗口宽度 800-1200px：侧边栏宽度220px
- 窗口宽度 > 1200px：侧边栏宽度为窗口宽度的25%，最大300px

**垂直滚动**:
- 文件列表超出可视区域时显示滚动条
- 使用虚拟化列表提升大目录性能

---

## 5. 实现细节

### 5.1 文件系统操作

**读取目录**: 使用Windows API

```cpp
// 使用 std::filesystem (C++17)
#include <filesystem>
namespace fs = std::filesystem;

std::vector<FileItem> LoadDirectory(const std::wstring& path) {
    std::vector<FileItem> items;
    try {
        for (const auto& entry : fs::directory_iterator(path)) {
            FileItem item;
            item.Name = entry.path().filename().wstring();
            item.Path = entry.path().wstring();
            item.IsDirectory = entry.is_directory();
            if (!item.IsDirectory) {
                item.Size = fs::file_size(entry.path());
                item.SizeDisplay = FormatFileSize(item.Size);
            }
            items.push_back(item);
        }
    } catch (const fs::filesystem_error& e) {
        // 处理权限错误或路径不存在
    }
    return items;
}
```

**打开文件**: 调用Shell执行

```cpp
void OpenFile(const std::wstring& filePath) {
    SHELLEXECUTEINFO sei = { sizeof(sei) };
    sei.lpVerb = L"open";
    sei.lpFile = filePath.c_str();
    sei.nShow = SW_SHOWNORMAL;
    
    if (!ShellExecuteEx(&sei)) {
        // 错误处理：显示错误提示
        DWORD error = GetLastError();
        // 记录日志或显示消息框
    }
}
```

### 5.2 文件大小格式化

```cpp
std::wstring FormatFileSize(uint64_t bytes) {
    const wchar_t* units[] = { L"B", L"KB", L"MB", L"GB", L"TB" };
    int unitIndex = 0;
    double size = static_cast<double>(bytes);
    
    while (size >= 1024.0 && unitIndex < 4) {
        size /= 1024.0;
        unitIndex++;
    }
    
    wchar_t buffer[32];
    if (unitIndex == 0) {
        swprintf(buffer, 32, L"%d%s", static_cast<int>(size), units[unitIndex]);
    } else {
        swprintf(buffer, 32, L"%.1f%s", size, units[unitIndex]);
    }
    return std::wstring(buffer);
}
```

### 5.3 目录变化监听

**方案1实现**: 监听Shell命令

在 `TerminalControl.cpp` 中：
```cpp
void TerminalControl::_HandleCommand(const std::wstring& command) {
    // 检测 cd 命令
    if (command.starts_with(L"cd ")) {
        std::wstring newPath = ParseCdCommand(command);
        _WorkingDirectoryChangedHandlers(newPath);
    }
}
```

在 `TerminalPage.cpp` 中：
```cpp
void TerminalPage::_OnWorkingDirectoryChanged(const winrt::hstring& newPath) {
    if (_fileBrowserViewModel) {
        _fileBrowserViewModel.LoadDirectory(newPath);
    }
}
```

### 5.4 错误处理

**文件系统错误**:
- 权限不足：显示"无权限访问该目录"提示
- 路径不存在：显示"目录不存在"提示
- 文件被占用：显示"无法打开文件"提示

**实现方式**:
- 使用 `try-catch` 捕获 `std::filesystem` 异常
- 在FileBrowserPane中显示错误信息条（InfoBar）
- 3秒后自动隐藏或用户手动关闭

---

## 6. 性能考虑

### 6.1 大目录优化

**虚拟化列表**: 使用XAML的 `ItemsRepeater` 或 `ListView` 虚拟化

- 只渲染可见区域的文件项
- 支持成千上万个文件的目录

**分页加载**（可选增强）:
- 首次加载仅显示前500个文件
- 滚动到底部时加载更多

### 6.2 文件系统I/O优化

**异步加载**:
- 使用 `winrt::fire_and_forget` 异步加载目录
- 显示加载指示器（Loading...）
- 避免阻塞UI线程

```cpp
winrt::fire_and_forget FileBrowserViewModel::LoadDirectoryAsync(winrt::hstring path) {
    co_await winrt::resume_background();  // 切换到后台线程
    auto items = LoadDirectory(path.c_str());
    
    co_await winrt::resume_foreground(Dispatcher());  // 切换回UI线程
    FileList().Clear();
    for (const auto& item : items) {
        FileList().Append(item);
    }
}
```

**缓存机制**（可选增强）:
- 缓存最近访问的目录内容
- 设置缓存过期时间（如30秒）
- 减少重复的文件系统调用

---

## 7. 配置选项

建议在 `settings.json` 中添加以下配置项：

```json
{
    "fileBrowser": {
        "enabled": true,              // 是否启用文件浏览器功能
        "defaultVisible": false,      // 启动时是否默认显示
        "showHiddenFiles": false,     // 是否显示隐藏文件
        "minWidth": 180,              // 最小宽度（像素）
        "maxWidth": 300,              // 最大宽度（像素）
        "defaultWidth": 220,          // 默认宽度（像素）
        "position": "right"           // 位置：left 或 right
    }
}
```

**配置实现**:
- 在 `TerminalSettingsModel` 中添加 `FileBrowserSettings` 类
- TerminalPage读取配置初始化侧边栏状态

---

## 8. 测试策略

### 8.1 单元测试

测试文件: `/src/cascadia/ut_app/FileBrowserTests.cpp`

测试用例：
- `VerifyLoadDirectoryReturnsCorrectFileList()` - 验证读取目录返回正确文件列表
- `VerifyFileItemSizeFormatting()` - 验证文件大小格式化正确
- `VerifyNavigateUpChangesDirectory()` - 验证返回上级目录功能
- `VerifyOpenFileCallsShellExecute()` - 验证打开文件调用系统API
- `VerifyErrorHandlingForInvalidPath()` - 验证无效路径错误处理

### 8.2 集成测试

测试场景：
- 切换侧边栏显示/隐藏状态
- 在终端执行 `cd` 命令后文件浏览器同步更新
- 点击文件后系统默认程序正确打开
- 双击文件夹进入子目录
- 大目录（1000+文件）加载性能

### 8.3 手动测试

测试检查清单：
- [ ] 按钮点击切换侧边栏正常
- [ ] 文件列表正确显示当前目录内容
- [ ] 文件图标和大小显示正确
- [ ] 点击文件使用默认程序打开
- [ ] 双击文件夹进入子目录
- [ ] 返回上级按钮正常工作
- [ ] 权限不足目录显示错误提示
- [ ] 窗口缩放时侧边栏宽度自适应
- [ ] 主题切换后侧边栏配色跟随更新
- [ ] 配置项生效（显示隐藏文件、默认可见性等）

---

## 9. 交付物

### 9.1 代码文件

新增文件：
- `/src/cascadia/TerminalApp/FileBrowserPane.xaml`
- `/src/cascadia/TerminalApp/FileBrowserPane.h`
- `/src/cascadia/TerminalApp/FileBrowserPane.cpp`
- `/src/cascadia/TerminalApp/FileBrowserPane.idl`
- `/src/cascadia/TerminalApp/FileBrowserViewModel.h`
- `/src/cascadia/TerminalApp/FileBrowserViewModel.cpp`
- `/src/cascadia/TerminalApp/FileBrowserViewModel.idl`
- `/src/cascadia/ut_app/FileBrowserTests.cpp`

修改文件：
- `/src/cascadia/TerminalApp/TerminalPage.xaml`
- `/src/cascadia/TerminalApp/TerminalPage.h`
- `/src/cascadia/TerminalApp/TerminalPage.cpp`
- `/src/cascadia/TerminalApp/TerminalPage.idl`
- `/src/cascadia/TerminalApp/TitlebarControl.xaml` 或 `TabRowControl.xaml`
- `/src/cascadia/TerminalSettingsModel/TerminalSettings.h`
- `/src/cascadia/TerminalSettingsModel/TerminalSettings.cpp`

### 9.2 文档

- 用户文档：如何使用文件浏览器功能
- 配置文档：settings.json配置项说明
- 开发文档：本设计文档

---

## 10. 实施计划

### 第一阶段：基础UI实现（2-3天）
- 创建FileBrowserPane UI组件
- 添加切换按钮到标题栏
- 实现侧边栏显示/隐藏逻辑

### 第二阶段：文件列表功能（3-4天）
- 实现FileBrowserViewModel
- 实现目录读取和文件列表显示
- 实现目录导航（双击文件夹、返回上级）

### 第三阶段：文件操作和同步（2-3天）
- 实现打开文件功能
- 实现终端工作目录同步机制
- 错误处理和提示信息

### 第四阶段：优化和测试（2-3天）
- 性能优化（异步加载、虚拟化列表）
- 编写单元测试和集成测试
- 配置项实现
- UI/UX细节打磨

**总计预估**: 9-13天开发时间

---

## 11. 风险和限制

### 11.1 技术风险

1. **目录同步实时性**
   - 风险：监听Shell命令可能无法捕获所有目录变化（如子进程中的cd）
   - 缓解：提供手动刷新按钮，或使用轮询机制作为备选

2. **大目录性能**
   - 风险：包含数千文件的目录可能导致UI卡顿
   - 缓解：使用虚拟化列表和异步加载

3. **跨平台兼容性**
   - 风险：设计主要针对Windows，Linux/Mac支持需要额外适配
   - 缓解：当前仅实现Windows版本，后续再扩展其他平台

### 11.2 用户体验限制

1. **仅支持当前目录**
   - 限制：不支持树形目录，无法同时查看多级目录结构
   - 原因：保持界面简洁，避免复杂性
   - 替代：用户可双击进入子目录导航

2. **不支持文件操作**
   - 限制：不提供文件复制、移动、删除、重命名等操作
   - 原因：终端用户更习惯用命令行操作文件
   - 替代：用户可在终端中使用命令完成文件操作

---

## 12. 未来增强

第一版实现后，可考虑以下增强功能：

1. **可拖拽调整宽度**：鼠标拖拽侧边栏边缘调整宽度
2. **文件搜索**：在当前目录中搜索文件名
3. **文件预览**：鼠标悬停显示文件前几行内容（文本文件）
4. **右键菜单**：提供复制路径、在资源管理器中打开等快捷操作
5. **书签功能**：收藏常用目录快速跳转
6. **Git状态显示**：显示文件的Git状态（已修改、未追踪等）
7. **树形目录模式**：提供选项切换到树形显示模式
8. **跨平台支持**：适配Linux和macOS

---

## 附录

### A. 参考资料

- [Windows Terminal 代码结构文档](../doc/ORGANIZATION.md)
- [Windows Terminal 贡献指南](../CONTRIBUTING.md)
- [XAML ListView 文档](https://docs.microsoft.com/windows/uwp/design/controls-and-patterns/listview-and-gridview)
- [C++ std::filesystem 文档](https://en.cppreference.com/w/cpp/filesystem)
- [ShellExecute API 文档](https://docs.microsoft.com/windows/win32/api/shellapi/nf-shellapi-shellexecutew)

### B. 术语表

- **pwd**: Present Working Directory，当前工作目录
- **ConPTY**: Windows Console Pseudoconsole，Windows控制台伪终端
- **WinRT**: Windows Runtime，Windows运行时API
- **XAML**: Extensible Application Markup Language，可扩展应用程序标记语言
- **ViewModel**: Model-View-ViewModel模式中的视图模型层

---

**设计审查**: 待用户确认

**Why:** 
- 文件浏览器功能可以显著提升终端与文件系统交互的效率
- 用户无需频繁使用ls命令查看目录内容
- 点击打开文件比手动输入命令路径更快捷
- 保持简洁设计，不干扰终端核心功能

**How to apply:**
- 按照实施计划逐阶段开发
- 优先实现核心功能（UI、文件列表、打开文件）
- 性能优化和配置项作为第二优先级
- 编写充分的测试确保功能稳定性
- 遵循Windows Terminal现有代码风格和架构模式