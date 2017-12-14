#pragma once

enum class SpellerId {
    aspell,
    hunspell,
    native,

    // ReSharper disable once CppInconsistentNaming
    COUNT,
};

const wchar_t *gui_string (SpellerId value);