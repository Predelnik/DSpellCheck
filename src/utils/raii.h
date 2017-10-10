#pragma once

struct ToolbarIcons;

struct ToolbarIconsWrapper {
    ToolbarIconsWrapper(HINSTANCE h_inst, LPCWSTR name, UINT type, int cx, int cy, UINT fu_load);
    const ToolbarIcons* get();
    ~ToolbarIconsWrapper();

private:
    ToolbarIconsWrapper();
    std::unique_ptr<ToolbarIcons> m_icons;
};
