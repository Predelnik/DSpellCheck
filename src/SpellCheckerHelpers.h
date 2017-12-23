#pragma once

class Settings;
class EditorInterface;
class MappedWstring;
enum class EditorViewType;

namespace SpellCheckerHelpers {
bool is_spell_checking_needed(const EditorInterface &editor,
                              const Settings &settings);
void apply_word_conversions(const Settings &settings, std::wstring &word);
void cut_apostrophes(const Settings &settings, std::wstring_view &word);
MappedWstring to_mapped_wstring(const EditorInterface &editor,
                                EditorViewType view, std::string_view str);

} // namespace SpellCheckerHelpers