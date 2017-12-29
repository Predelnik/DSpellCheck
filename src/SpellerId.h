#pragma once

enum class SpellerId {
    aspell,
    hunspell,
    native,

    // ReSharper disable once CppInconsistentNaming
    COUNT,
};

std::wstring gui_string(SpellerId value);