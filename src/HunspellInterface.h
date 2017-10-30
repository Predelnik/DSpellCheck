/*
This file is part of DSpellCheck Plug-in for Notepad++
Copyright (C)2013 Sergey Semushin <Predelnik@gmail.com>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#pragma once

#include "SpellerInterface.h"

#include "iconv.h"
#include "CommonFunctions.h"
#include "MainDef.h"

class LanguageInfo;

class Hunspell;

using WordSet = std::unordered_set<std::string>;
using SortedWordSet = std::set<std::string>;
class IconvWrapperT {
    static void close_iconv (iconv_t conv)
    {
      if (conv != reinterpret_cast<iconv_t>(-1))
        iconv_close (conv);
    }

public:
    template <typename... ArgTypes>
    IconvWrapperT (ArgTypes &&... args) :
      m_conv (iconv_open (std::forward<ArgTypes> (args)...), &close_iconv)
    {
    }
    iconv_t get () const { return m_conv.get (); }
    IconvWrapperT () : m_conv (nullptr, &close_iconv) {}
    IconvWrapperT (IconvWrapperT &&) = default;
    IconvWrapperT &operator= (IconvWrapperT &&) = default;

    ~IconvWrapperT () {
    }

private:
    std::unique_ptr<void, void(*)(iconv_t)> m_conv;
};

struct DicInfo {
  std::unique_ptr<Hunspell> hunspell;
  IconvWrapperT converter;
  IconvWrapperT back_converter;
  IconvWrapperT converter_ansi;
  IconvWrapperT back_converter_ansi;
  std::wstring local_dic_path;
  WordSet local_dic; // Stored in Dictionary encoding
};

struct AvailableLangInfo {
  std::wstring name;
  int type; // Type = 1 - System Dir Dictionary, 0 - Nomal Dictionary

  bool operator<(const AvailableLangInfo &rhs) const {
    return name < rhs.name;
  }
};

class HunspellInterface : public SpellerInterface {
public:
  HunspellInterface(HWND npp_window_arg);
  ~HunspellInterface();
  std::vector<LanguageInfo> get_language_list() const override;
  void set_language(const wchar_t* lang) override;
  void set_multiple_languages(
      const std::vector<std::wstring>& list) override;             // Languages are from LangList
  bool check_word(const char* word) override; // Word in Utf-8 or ANSI
  bool is_working() const override;
  std::vector<std::string> get_suggestions(const char* word) override;
  void add_to_dictionary(const char* word) override;
  void ignore_all(const char* word) override;

  void set_directory(const wchar_t* dir);
  void set_additional_directory(const wchar_t* dir);
  static void read_user_dic(WordSet& target, const wchar_t* path);
  void set_use_one_dic(bool value);
  void update_on_dic_removal(wchar_t *path, bool &need_single_lang_reset,
                          bool &need_multi_lang_reset);
  bool get_lang_only_system(const wchar_t* lang) const;

private:
  DicInfo* create_hunspell(const wchar_t* name, int type);
    static bool speller_check_word(const DicInfo& dic, const char* word, EncodingType encoding);
  void message_box_word_cannot_be_added();

private:
  bool m_is_hunspell_working;
  bool m_use_one_dic;
  std::wstring m_dic_dir;
  std::wstring m_sys_dic_dir;
  std::set<AvailableLangInfo> m_dic_list;
  std::map<std::wstring, DicInfo> m_all_hunspells;
    static std::string get_converted_word(const char* source, iconv_t converter);
  DicInfo *m_singular_speller;
  DicInfo *m_last_selected_speller;
  std::vector<DicInfo *> m_spellers;
  WordSet m_memorized;
  WordSet m_ignored;
  std::wstring m_user_dic_path;        // For now only default one.
  std::wstring m_system_wrong_dic_path; // Only for reading and then removing
  HWND m_npp_window;
};
