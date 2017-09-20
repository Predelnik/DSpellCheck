#pragma once

struct toolbarIcons;

struct ToolbarIconsWrapper {
    ToolbarIconsWrapper(HINSTANCE hInst, LPCWSTR name, UINT type, int cx, int cy, UINT fuLoad);
    const toolbarIcons* get();
    ~ToolbarIconsWrapper();

private:
    ToolbarIconsWrapper();
    std::unique_ptr<toolbarIcons> m_icons;
};
