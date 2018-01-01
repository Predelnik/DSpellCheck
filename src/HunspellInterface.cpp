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
// To avoid problems with windows.h and hunspell
#ifdef near
#undef near
#endif                  // near
#define HUNSPELL_STATIC // We're using static build so ...
#include "HunspellInterface.h"
#include "CommonFunctions.h"
#include "LanguageInfo.h"
#include "Plugin.h"
#include "hunspell/hunspell.hxx"

#include <fcntl.h>
#include <fstream>
#include <io.h>

static std::vector<std::wstring>
list_files(const wchar_t *path, const wchar_t *mask, const wchar_t *filter) {
  WIN32_FIND_DATA ffd;
  std::stack<std::wstring> directories;

  directories.push(path);
  std::vector<std::wstring> out;

  while (!directories.empty()) {
    auto top = std::move(directories.top());
    auto spec = top + L"\\"s + mask;
    directories.pop();

    HANDLE h_find = FindFirstFile(spec.c_str(), &ffd);
    if (h_find == INVALID_HANDLE_VALUE) {
      return {};
    }

    do {
      if (ffd.cFileName != L"."sv && ffd.cFileName != L".."sv) {
        auto buf = top + L"\\" + ffd.cFileName;
        if ((ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
          directories.push(buf);
        } else {
          if (PathMatchSpec(buf.c_str(), filter))
            out.push_back(buf);
        }
      }
    } while (FindNextFile(h_find, &ffd) != 0);

    if (GetLastError() != ERROR_NO_MORE_FILES) {
      FindClose(h_find);
      return {};
    }

    FindClose(h_find);
  }
  return out;
}

template <typename OutputCharType, typename InputCharType>
static std::basic_string<OutputCharType>
convert_impl(const IconvWrapperT &conv,
             std::basic_string_view<InputCharType> input) {
  static std::vector<char> buf;
  if constexpr (std::is_same_v<OutputCharType, char>)
    buf.resize((input.length() + 1) * 6);
  else
    buf.resize((input.length() + 1) * sizeof(OutputCharType));
  if (conv.get() == iconv_t(-1)) {
    return {};
  }
  size_t in_size = (input.length() + 1) * sizeof(InputCharType);
  size_t out_size = buf.size();
  char *out_buf = buf.data();
  auto in_buf = input.data();
  size_t res = iconv(conv.get(), reinterpret_cast<const char **>(&in_buf),
                     &in_size, &out_buf, &out_size);
  if (res == static_cast<size_t>(-1)) {
    return {};
  }
  return {reinterpret_cast<OutputCharType *>(buf.data())};
}

std::string DicInfo::to_dictionary_encoding(std::wstring_view input) const {
  return convert_impl<char>(converter, input);
}

std::wstring DicInfo::from_dictionary_encoding(std::string_view input) const {
  return convert_impl<wchar_t>(back_converter, input);
}

HunspellInterface::HunspellInterface(HWND npp_window_arg)
    : m_use_one_dic(false) {
  m_npp_window = npp_window_arg;
  m_singular_speller = {};
  m_last_selected_speller = {};
  m_is_hunspell_working = false;
}

void HunspellInterface::update_on_dic_removal(wchar_t *path,
                                              bool &need_single_lang_reset,
                                              bool &need_multi_lang_reset) {
  auto it = m_all_hunspells.find(path);
  need_single_lang_reset = false;
  need_multi_lang_reset = false;
  if (it != m_all_hunspells.end()) {
    if (m_singular_speller != nullptr && m_singular_speller == &it->second)
      need_single_lang_reset = true;

    for (auto speller : m_spellers)
      if (speller == &it->second) {
        need_multi_lang_reset = true;
      }
    m_all_hunspells.erase(it);
  }
}

void HunspellInterface::set_use_one_dic(bool value) { m_use_one_dic = value; }

bool are_paths_equal(const wchar_t *path1, const wchar_t *path2) {
  BY_HANDLE_FILE_INFORMATION bhfi1, bhfi2;
  HANDLE h2;
  DWORD access = 0;
  DWORD share = FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE;

  HANDLE h1 =
      CreateFile(path1, access, share, nullptr, OPEN_EXISTING,
                 (GetFileAttributes(path1) & FILE_ATTRIBUTE_DIRECTORY) != 0
                     ? FILE_FLAG_BACKUP_SEMANTICS
                     : 0,
                 nullptr);
  if (INVALID_HANDLE_VALUE != h1) {
    if (GetFileInformationByHandle(h1, &bhfi1) == FALSE)
      bhfi1.dwVolumeSerialNumber = 0;
    h2 = CreateFile(path2, access, share, nullptr, OPEN_EXISTING,
                    (GetFileAttributes(path2) & FILE_ATTRIBUTE_DIRECTORY) != 0
                        ? FILE_FLAG_BACKUP_SEMANTICS
                        : 0,
                    nullptr);
    if (GetFileInformationByHandle(h2, &bhfi2) == FALSE)
      bhfi2.dwVolumeSerialNumber = bhfi1.dwVolumeSerialNumber + 1;
  } else
    return false;

  CloseHandle(h1);
  CloseHandle(h2);
  return INVALID_HANDLE_VALUE != h1 && INVALID_HANDLE_VALUE != h2 &&
         bhfi1.dwVolumeSerialNumber == bhfi2.dwVolumeSerialNumber &&
         bhfi1.nFileIndexHigh == bhfi2.nFileIndexHigh &&
         bhfi1.nFileIndexLow == bhfi2.nFileIndexLow;
}

template <typename CharType>
static void
write_user_dic(const std::unordered_set<std::basic_string<CharType>> &new_words,
               const wchar_t *path) {
  // Wordset itself
  // being removed here as local variable, all strings are not copied

  auto path_view = std::wstring_view(path);
  auto slash_pos = path_view.rfind(L'\\');
  if (slash_pos == std::string::npos)
    return;
  check_for_directory_existence(std::wstring{path_view.substr(0, slash_pos)});

  SetFileAttributes(path, FILE_ATTRIBUTE_NORMAL);

  std::ifstream input(path);
  std::vector<std::string> lines;
  {
    std::string line;
    std::getline(input, line);
    for (; std::getline(input, line);)
      lines.push_back(line);
  }

  if constexpr (sizeof(CharType) == 2) {
    std::transform(new_words.begin(), new_words.end(),
                   std::back_inserter(lines),
                   [](const std::wstring &s) { return to_utf8_string(s); });
  } else {
    lines.insert(lines.end(), new_words.begin(), new_words.end());
  }
  auto fp = _wfopen(path, L"w");
  if (!fp)
    return;

  fprintf(fp, "%zu\n", lines.size());
  for (const auto &line : lines)
    fprintf(fp, "%s\n", line.c_str());
  fclose(fp);
}

HunspellInterface::~HunspellInterface() {
  m_is_hunspell_working = false;

  if (!m_system_wrong_dic_path.empty() && !m_user_dic_path.empty() &&
      !are_paths_equal(m_system_wrong_dic_path.c_str(),
                       m_user_dic_path.c_str())) {
    SetFileAttributes(m_system_wrong_dic_path.c_str(), FILE_ATTRIBUTE_NORMAL);
    DeleteFile(m_system_wrong_dic_path.c_str());
  }

  if (!m_user_dic_path.empty())
    write_user_dic(m_new_common_words, m_user_dic_path.c_str());
  for (auto &p : m_all_hunspells) {
    auto &hs = p.second;
    if (!hs.local_dic_path.empty())
      write_user_dic(hs.new_words, hs.local_dic_path.c_str());
  }
}

std::vector<LanguageInfo> HunspellInterface::get_language_list() const {
  std::vector<LanguageInfo> list;
  for (auto &dic : m_dic_list) {
    list.emplace_back(dic.name, true, get_lang_only_system(dic.name.c_str()));
  }
  return list;
}

DicInfo *HunspellInterface::create_hunspell(const AvailableLangInfo &info) {
  {
    auto it = m_all_hunspells.find(info.full_path);
    if (it != m_all_hunspells.end()) {
      return &it->second;
    }
  }
  auto aff_buf = info.full_path + L".aff";
  auto dic_buf = info.full_path + L".dic";
  auto aff_buf_ansi = to_string(aff_buf.c_str());
  auto dic_buf_ansi = to_string(dic_buf.c_str());
  auto new_hunspell =
      std::make_unique<Hunspell>(aff_buf_ansi.c_str(), dic_buf_ansi.c_str());
  auto new_name = aff_buf.substr(0, aff_buf.length() - 4);
  DicInfo new_dic;
  const char *dic_encoding = new_hunspell->get_dic_encoding();
  if (stricmp(dic_encoding, "Microsoft-cp1251") == 0)
    dic_encoding = "cp1251"; // Queer fix for encoding which isn't being guessed
  // correctly by libiconv TODO: Find other possible
  // such failures
  new_dic.converter = {dic_encoding, "UCS-2LE"};
  new_dic.back_converter = {"UCS-2LE", dic_encoding};
  new_dic.local_dic_path += m_dic_dir + L"\\"s + info.name + L".usr";
  new_hunspell->add_dic(to_string(new_dic.local_dic_path).c_str());
  if ("UTF-8"sv == dic_encoding) {
    auto encoded_path =
        create_encoded_dict_version(m_user_dic_path.c_str(), dic_encoding);
    if (!encoded_path.empty()) {
      new_hunspell->add_dic(to_string(encoded_path).c_str());
      DeleteFile(encoded_path.c_str());
    }
  } else
      new_hunspell->add_dic(to_string(m_user_dic_path).c_str());

  {
    for (const auto &word : m_new_common_words) {
      auto conv_word = new_dic.to_dictionary_encoding(word);
      if (!conv_word.empty())
        new_hunspell->add(conv_word); // Adding all already memorized words to
                                      // newly loaded Hunspell instance
    }
  }
  {
    for (const auto &word : new_dic.new_words) {
      new_hunspell->add(word); // Adding all already memorized words from local
                               // dictionary to Hunspell instance, local
                               // dictionaries are in dictionary encoding
    }
  }
  new_dic.hunspell = std::move(new_hunspell);
  auto &target = m_all_hunspells[new_name];
  target = std::move(new_dic);
  return &target;
}

void HunspellInterface::set_language(const wchar_t *lang) {
  if (m_dic_list.empty()) {
    m_singular_speller = nullptr;
    return;
  }
  AvailableLangInfo temp;
  temp.name = lang;
  temp.type = 0;
  auto it = m_dic_list.find(temp);
  if (it == m_dic_list.end())
    it = m_dic_list.begin();
  m_singular_speller = create_hunspell(*it);
}

void HunspellInterface::set_multiple_languages(
    const std::vector<std::wstring> &list) {
  m_spellers.clear();

  if (m_dic_list.empty())
    return;

  for (auto &lang : list) {
    AvailableLangInfo temp;
    temp.name = lang;
    temp.type = 0;
    auto it = m_dic_list.find(temp);
    if (it == m_dic_list.end())
      continue;
    auto ptr = create_hunspell(*it);
    m_spellers.push_back(ptr);
  }
}

bool HunspellInterface::speller_check_word(const DicInfo &dic,
                                           WordForSpeller word) {
  if (word.data.ends_with_dot)
    word.str += L".";
  auto word_to_check = dic.to_dictionary_encoding(word.str.c_str());
  if (word_to_check.empty())
    return false;
  // No additional check for memorized is needed since all words are already in
  // dictionary

  return dic.hunspell->spell(word_to_check);
}

bool HunspellInterface::check_word(WordForSpeller word) const {
  if (m_ignored.find(word.str) != m_ignored.end())
    return true;

  bool res = false;
  if (m_multi_mode == 0) {
    if (m_singular_speller != nullptr)
      res = speller_check_word(*m_singular_speller, word);
    else
      res = true;
  } else {
    if (m_spellers.empty())
      return true;

    for (auto &speller : m_spellers) {
      res = res || speller_check_word(*speller, word);
      if (res)
        break;
    }
  }
  return res;
}

void HunspellInterface::message_box_word_cannot_be_added() {
  MessageBox(m_npp_window, rc_str(IDC_ERROR_BAD_ENCODING).c_str(),
             rc_str(IDS_WORD_CANT_BE_ADDED).c_str(), MB_OK | MB_ICONWARNING);
}

void HunspellInterface::reset_spellers() {
  // these triggers reload of all hunspells and user dictionaries
  m_all_hunspells.clear();
}

std::wstring
HunspellInterface::create_encoded_dict_version(const wchar_t *dict_path,
                                               const char *target_encoding) {
  std::ifstream is(dict_path);
  std::string line;
  auto tmp_filename = _wtmpnam(nullptr);
  std::ofstream os(tmp_filename);
  while (std::getline(is, line)) {
    auto result = convert_impl<char>(IconvWrapperT{target_encoding, "UTF-8"},
                                     std::string_view(line));
    if (!result.empty())
      os << result << '\n';
  }
  return tmp_filename;
}

void HunspellInterface::add_to_dictionary(const wchar_t *word) {
  if (m_last_selected_speller == nullptr)
    return;

  std::wstring dic_path;
  if (m_use_one_dic)
    dic_path = m_user_dic_path;
  else
    dic_path = m_last_selected_speller->local_dic_path;

  if (!PathFileExists(dic_path.c_str())) {
    auto last_slash_pos = dic_path.rfind(L"\\");
    if (last_slash_pos == std::wstring::npos)
      return;
    auto dir = dic_path.substr(0, last_slash_pos);
    check_for_directory_existence(dir);
    // If there's no file then we're checking if we can create it, there's no
    // harm in it
    int local_dic_file_handle =
        _wopen(dic_path.c_str(), _O_CREAT | _O_BINARY | _O_WRONLY);
    if (local_dic_file_handle == -1) {
      MessageBox(m_npp_window, rc_str(IDS_USER_DICT_CANT_SAVE_BODY).c_str(),
                 rc_str(IDS_USER_DICT_CANT_SAVE_TITLE).c_str(),
                 MB_OK | MB_ICONWARNING);
    } else {
      _close(local_dic_file_handle);
    }
    SetFileAttributes(dic_path.c_str(), FILE_ATTRIBUTE_NORMAL);
  } else {
    SetFileAttributes(dic_path.c_str(), FILE_ATTRIBUTE_NORMAL);
    int local_dic_file_handle =
        _wopen(dic_path.c_str(), _O_APPEND | _O_BINARY | _O_WRONLY);
    if (local_dic_file_handle == -1) {
      MessageBox(m_npp_window, rc_str(IDS_USER_DICT_CANT_SAVE_BODY).c_str(),
                 rc_str(IDS_USER_DICT_CANT_SAVE_TITLE).c_str(),
                 MB_OK | MB_ICONWARNING);
    } else
      _close(local_dic_file_handle);
  }

  if (m_use_one_dic) {
    m_new_common_words.insert(word);
    for (auto &p : m_all_hunspells) {
      auto conv_word = p.second.to_dictionary_encoding(word);
      if (!conv_word.empty())
        p.second.hunspell->add(conv_word);
      else if (p.second.hunspell == m_last_selected_speller->hunspell)
        message_box_word_cannot_be_added();
      // Adding word to all currently loaded dictionaries and in memorized list
      // to save it.
    }
  } else {
    auto conv_word = m_last_selected_speller->to_dictionary_encoding(word);
    m_last_selected_speller->new_words.insert(conv_word);
    if (!conv_word.empty())
      m_last_selected_speller->hunspell->add(conv_word);
    else
      message_box_word_cannot_be_added();
  }
}

void HunspellInterface::ignore_all(const wchar_t *word) {
  if (m_last_selected_speller == nullptr)
    return;

  m_ignored.insert(word);
}

std::vector<std::wstring>
HunspellInterface::get_suggestions(const wchar_t *word) const {
  std::vector<std::string> list;
  m_last_selected_speller = m_singular_speller;

  if (m_multi_mode == 0) {
    list = m_singular_speller->hunspell->suggest(
        m_singular_speller->to_dictionary_encoding(word));
  } else {
    for (auto speller : m_spellers) {
      auto cur_list =
          speller->hunspell->suggest(speller->to_dictionary_encoding(word));
      if (cur_list.size() > list.size()) {
        list = std::move(cur_list);
        m_last_selected_speller = speller;
      }
    }
  }

  std::vector<std::wstring> sugg_list(list.size());
  std::transform(list.begin(), list.end(), sugg_list.begin(),
                 [this](const std::string &s) {
                   return m_last_selected_speller->from_dictionary_encoding(s);
                 });
  return sugg_list;
}

void HunspellInterface::set_directory(const wchar_t *dir) {
  if (dir == nullptr || *dir == L'\0')
    return;

  m_user_dic_path = dir;
  if (m_user_dic_path.back() != L'\\')
    m_user_dic_path += L"\\";
  m_user_dic_path += L"UserDic.dic"; // Should be tunable really
  m_dic_dir = dir;

  m_dic_list.clear();
  m_is_hunspell_working = true;

  auto files = list_files(dir, L"*.*", L"*.aff");
  if (files.empty()) {
    return;
  }

  for (auto &file_path : files) {
    auto dic_file_path = file_path.substr(0, file_path.length() - 4) + L".dic";
    if (PathFileExists(dic_file_path.c_str())) {
      auto dic_name = file_path.substr(dic_file_path.rfind(L'\\') + 1);
      dic_name = dic_name.substr(0, dic_name.length() - 4);
      AvailableLangInfo new_x;
      new_x.type = 0;
      new_x.name = dic_name;
      new_x.full_path = file_path.substr(0, file_path.length() - 4);
      m_dic_list.insert(new_x);
    }
  }
}

void HunspellInterface::set_additional_directory(const wchar_t *dir) {
  m_sys_dic_dir = dir;
  m_is_hunspell_working = true;

  auto files = list_files(dir, L"*.*", L"*.aff");
  if (files.empty())
    return;

  for (auto &file : files) {
    auto file_name_without_ext = file.substr(0, file.rfind(L'.'));
    if (PathFileExists((file_name_without_ext + L".dic").c_str())) {
      AvailableLangInfo new_x;
      new_x.type = 1;
      new_x.name =
          file_name_without_ext.substr(file_name_without_ext.rfind(L'\\') + 1);
      new_x.full_path = file.substr(0, file.length() - 4);
      if (m_dic_list.count(new_x) == 0)
        m_dic_list.insert(new_x);
    }
  }
  // Now we have 2 dictionaries on our hands

  // Reading system path unified dic too
  m_system_wrong_dic_path = dir;
  if (m_system_wrong_dic_path.back() != L'\\')
    m_system_wrong_dic_path += L"\\";
  m_system_wrong_dic_path += L"UserDic.dic"; // Should be tunable really
}

bool HunspellInterface::get_lang_only_system(const wchar_t *lang) const {
  AvailableLangInfo needle;
  needle.name = lang;
  needle.type = 1;
  auto it = m_dic_list.find(needle);
  return it != m_dic_list.end() && it->type == 1;
}

bool HunspellInterface::is_working() const { return m_is_hunspell_working; }
