// This file is part of DSpellCheck Plug-in for Notepad++
// Copyright (C)2018 Sergey Semushin <Predelnik@gmail.com>
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#ifdef near
#undef near
#endif                  // near
#define HUNSPELL_STATIC // We're using static build so ...
#include "HunspellInterface.h"
#include "CommonFunctions.h"
#include "LanguageInfo.h"
#include "Plugin.h"
#include "hunspell/hunspell.hxx"

#include "utils/winapi.h"
#include <fcntl.h>
#include <fstream>
#include <io.h>

namespace {
void append_word_to_user_dictionary(const wchar_t *dictionary_path, const char *word) {
  if (!dictionary_path)
    return;

  auto fp = _wfopen(dictionary_path, L"a");
  fprintf(fp, "%s\n", word);
  fclose(fp);
}

std::vector<char> read_file_data(const wchar_t *file_path) {
  auto fp = _wfopen(file_path, L"rb");
  if (!fp)
    return {};
  _fseeki64(fp, 0, SEEK_END);
  auto size = static_cast<size_t> (_ftelli64(fp));
  std::vector<char> res(size + 1);
  _fseeki64(fp, 0, SEEK_SET);
  if (fread(res.data(), 1, size, fp) != size)
    return {};
  fclose(fp);
  return res;
}

void update_word_count(const wchar_t *dictionary_path) {
  if (!dictionary_path || !PathFileExists(dictionary_path))
    return;

  auto data = read_file_data(dictionary_path);
  if (data.empty())
    return;
  auto line_cnt = std::count(data.begin(), data.end(), '\n') + 1 /*because number of lines = number of separators + 1 */
                  - 1 /* because e should ignore line with current count */ - 1 /* final line end*/;
  char *end_ptr;
  auto cur_cnt = strtol(data.data(), &end_ptr, 10);
  if (line_cnt != cur_cnt) {
    auto fp = _wfopen(dictionary_path, L"wb");
    fprintf(fp, "%Id\r\n", line_cnt);
    auto it = std::find(data.begin(), data.end(), '\n');
    if (it != data.end()) {
      auto cnt_len = (it - data.begin()) + 1;
      fwrite(data.data() + cnt_len, 1, data.size() - cnt_len - 1, fp);
    }
    fclose(fp);
  }
}
} // namespace

static std::vector<std::wstring> list_files(const wchar_t *path, const wchar_t *mask, const wchar_t *filter) {
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
static std::basic_string<OutputCharType> convert_impl(const IconvWrapperT &conv, std::basic_string_view<InputCharType> input) {
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
  size_t res = iconv(conv.get(), reinterpret_cast<const char **>(&in_buf), &in_size, &out_buf, &out_size);
  if (res == static_cast<size_t>(-1)) {
    return {};
  }
  return {reinterpret_cast<OutputCharType *>(buf.data())};
}

std::string DicInfo::to_dictionary_encoding(std::wstring_view input) const { return convert_impl<char>(converter, input); }

std::wstring DicInfo::from_dictionary_encoding(std::string_view input) const { return convert_impl<wchar_t>(back_converter, input); }

HunspellInterface::HunspellInterface(HWND npp_window_arg) : m_use_one_dic(false) {
  m_npp_window = npp_window_arg;
  m_singular_speller = {};
  m_last_selected_speller = {};
  m_is_hunspell_working = false;
}

void HunspellInterface::update_on_dic_removal(wchar_t *path, bool &need_single_lang_reset, bool &need_multi_lang_reset) {
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

  HANDLE h1 = CreateFile(path1, access, share, nullptr, OPEN_EXISTING,
                         (GetFileAttributes(path1) & FILE_ATTRIBUTE_DIRECTORY) != 0 ? FILE_FLAG_BACKUP_SEMANTICS : 0, nullptr);
  if (INVALID_HANDLE_VALUE != h1) {
    if (GetFileInformationByHandle(h1, &bhfi1) == FALSE)
      bhfi1.dwVolumeSerialNumber = 0;
    h2 = CreateFile(path2, access, share, nullptr, OPEN_EXISTING, (GetFileAttributes(path2) & FILE_ATTRIBUTE_DIRECTORY) != 0 ? FILE_FLAG_BACKUP_SEMANTICS : 0,
                    nullptr);
    if (GetFileInformationByHandle(h2, &bhfi2) == FALSE)
      bhfi2.dwVolumeSerialNumber = bhfi1.dwVolumeSerialNumber + 1;
  } else
    return false;

  CloseHandle(h1);
  CloseHandle(h2);
  return INVALID_HANDLE_VALUE != h1 && INVALID_HANDLE_VALUE != h2 && bhfi1.dwVolumeSerialNumber == bhfi2.dwVolumeSerialNumber &&
         bhfi1.nFileIndexHigh == bhfi2.nFileIndexHigh && bhfi1.nFileIndexLow == bhfi2.nFileIndexLow;
}

std::vector<LanguageInfo> HunspellInterface::get_language_list() const {
  std::vector<LanguageInfo> list;
  for (auto &dic : m_dic_list) {
    list.emplace_back(dic.name, true, get_lang_only_system(dic.name.c_str()));
  }
  return list;
}

DicInfo *HunspellInterface::create_hunspell(const AvailableLangInfo &lang_info) {
  {
    auto it = m_all_hunspells.find(lang_info.full_path);
    if (it != m_all_hunspells.end()) {
      return &it->second;
    }
  }

  DicInfo new_empty_dic;

  new_empty_dic.loading_task = TaskWrapper(m_npp_window);
  new_empty_dic.loading_task->do_deferred(
      [lang_info, dic_dir = m_dic_dir, user_dict_path = m_user_dic_path](concurrency::cancellation_token)
  {
    auto aff_path = lang_info.full_path + L".aff";
    auto dic_path = lang_info.full_path + L".dic";
    auto aff_buf_ansi = to_string(aff_path.c_str());
    auto dic_buf_ansi = to_string(dic_path.c_str());
    auto new_name = aff_path.substr(0, aff_path.length() - 4);
    // shared_ptr is used only as a workaround due to the fact that TaskWrapper uses std::function
    // TODO: use some unique_function implementation in TaskWrapper and remove shared_ptr usage here.
    auto new_dic = std::make_shared<DicInfo>();
    new_dic->local_dic_path = dic_dir + L"\\"s + lang_info.name + L".usr";
    auto new_hunspell = std::make_unique<Hunspell>(aff_buf_ansi.c_str(), dic_buf_ansi.c_str());
    const char *dic_encoding = new_hunspell->get_dic_encoding();
    if (stricmp(dic_encoding, "Microsoft-cp1251") == 0)
      dic_encoding = "cp1251"; // Queer fix for encoding which isn't being guessed
    // correctly by libiconv TODO: Find other possible
    // such failures
    new_dic->converter = {dic_encoding, "UCS-2LE"};
    new_dic->back_converter = {"UCS-2LE", dic_encoding};
    update_word_count(new_dic->local_dic_path.c_str());
    new_hunspell->add_dic(to_string(new_dic->local_dic_path).c_str());
    if ("UTF-8"sv != dic_encoding) {
      auto encoded_path = create_encoded_dict_version(user_dict_path.c_str(), dic_encoding);
      if (!encoded_path.empty()) {
        new_hunspell->add_dic(to_string(encoded_path).c_str());
        WinApi::delete_file(encoded_path.c_str());
      }
    } else
      new_hunspell->add_dic(to_string(user_dict_path).c_str());
    new_dic->hunspell = std::move(new_hunspell);
    return new_dic;
  },
      [path = lang_info.full_path, this](std::shared_ptr<DicInfo> dic_info)
  {
    m_all_hunspells[path] = std::move(*dic_info);
    speller_loaded();
  });

  auto &target = m_all_hunspells[lang_info.full_path];
  target = std::move(new_empty_dic);
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

void HunspellInterface::set_multiple_languages(const std::vector<std::wstring> &list) {
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

bool HunspellInterface::speller_check_word(const DicInfo &dic, WordForSpeller word) {
  if (!dic.is_loaded())
    return true;
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
  switch (m_speller_mode) {
  case SpellerMode::SingleLanguage: {
    if (m_singular_speller != nullptr)
      res = speller_check_word(*m_singular_speller, word);
    else
      res = true;
  } break;
  case SpellerMode::MultipleLanguages: {
    if (m_spellers.empty())
      return true;

    for (auto &speller : m_spellers) {
      res = res || speller_check_word(*speller, word);
      if (res)
        break;
    }
  } break;
  }
  return res;
}

void HunspellInterface::message_box_word_cannot_be_added() {
  MessageBox(m_npp_window, rc_str(IDC_ERROR_BAD_ENCODING).c_str(), rc_str(IDS_WORD_CANT_BE_ADDED).c_str(), MB_OK | MB_ICONWARNING);
}

HunspellInterface::~HunspellInterface() {
  m_is_hunspell_working = false;

  if (!m_system_wrong_dic_path.empty() && !m_user_dic_path.empty() && !are_paths_equal(m_system_wrong_dic_path.c_str(), m_user_dic_path.c_str())) {
    WinApi::delete_file(m_system_wrong_dic_path.c_str());
  }

  update_word_count(m_user_dic_path.c_str());
  for (auto &p : m_all_hunspells) {
    auto &hs = p.second;
    update_word_count(hs.local_dic_path.c_str());
  }
}

void HunspellInterface::reset_spellers() {
  // these triggers reload of all hunspells and user dictionaries
  m_all_hunspells.clear();
}

// drop cache if dictionary was removed
void HunspellInterface::dictionary_removed(const std::wstring &path) { m_all_hunspells.erase(path); }

std::wstring HunspellInterface::create_encoded_dict_version(const wchar_t *dict_path, const char *target_encoding) {
  std::ifstream is(dict_path);
  std::string line;
  auto tmp_filename = _wtmpnam(nullptr);
  std::ofstream os(tmp_filename);
  while (std::getline(is, line)) {
    auto result = convert_impl<char>(IconvWrapperT{target_encoding, "UTF-8"}, std::string_view(line));
    if (!result.empty())
      os << result << '\n';
  }
  return tmp_filename;
}

void HunspellInterface::add_to_dictionary(const wchar_t *word) {
  if (m_last_selected_speller == nullptr || !m_last_selected_speller->is_loaded())
    return;

  std::wstring dic_path;
  if (m_use_one_dic)
    dic_path = m_user_dic_path;
  else
    dic_path = m_last_selected_speller->local_dic_path;

  auto check_open = [&](const wchar_t *flags) {
    SetFileAttributes(dic_path.c_str(), FILE_ATTRIBUTE_NORMAL);
    auto fp = _wfopen(dic_path.c_str(), flags);
    if (flags == L"w"sv) {
      fprintf(fp, "0\n");
    }
    if (!fp) {
      MessageBox(m_npp_window, rc_str(IDS_USER_DICT_CANT_SAVE_BODY).c_str(), rc_str(IDS_USER_DICT_CANT_SAVE_TITLE).c_str(), MB_OK | MB_ICONWARNING);
    } else {
      fclose(fp);
    }
  };

  if (!PathFileExists(dic_path.c_str())) {
    auto last_slash_pos = dic_path.rfind(L'\\');
    if (last_slash_pos == std::wstring::npos)
      return;
    auto dir = dic_path.substr(0, last_slash_pos);
    check_for_directory_existence(dir);
    check_open(L"w");
  } else {
    check_open(L"a");
  }

  if (m_use_one_dic) {
    append_word_to_user_dictionary(m_user_dic_path.c_str(), to_utf8_string(word).c_str());
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
    append_word_to_user_dictionary(m_last_selected_speller->local_dic_path.c_str(), conv_word.c_str());
    if (!conv_word.empty())
      m_last_selected_speller->hunspell->add(conv_word);
    else
      message_box_word_cannot_be_added();
  }
}

void HunspellInterface::ignore_all(const wchar_t *word) {
  if (m_last_selected_speller == nullptr || !m_last_selected_speller->is_loaded())
    return;

  m_ignored.insert(word);
}

std::vector<std::wstring> HunspellInterface::get_suggestions(const wchar_t *word) const {
  std::vector<std::string> list;
  m_last_selected_speller = nullptr;

  switch (m_speller_mode) {
  case SpellerMode::SingleLanguage: {
    m_last_selected_speller = m_singular_speller;
    if (!m_singular_speller->is_loaded())
      return {};
    list = m_singular_speller->hunspell->suggest(m_singular_speller->to_dictionary_encoding(word));
  } break;
  case SpellerMode::MultipleLanguages: {
    for (auto speller : m_spellers) {
      if (!speller->is_loaded())
        continue;
      auto cur_list = speller->hunspell->suggest(speller->to_dictionary_encoding(word));
      if (cur_list.size() > list.size()) {
        list = std::move(cur_list);
        m_last_selected_speller = speller;
      }
    }
  } break;
  }

  if (!m_last_selected_speller)
    return {};

  std::vector<std::wstring> sugg_list(list.size());
  std::transform(list.begin(), list.end(), sugg_list.begin(), [this](const std::string &s) { return m_last_selected_speller->from_dictionary_encoding(s); });
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
      new_x.name = file_name_without_ext.substr(file_name_without_ext.rfind(L'\\') + 1);
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
