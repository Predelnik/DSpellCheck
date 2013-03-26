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

#ifndef ABSTRACTSPELLERINTERFACE_H
#define ABSTRACTSPELLERINTERFACE_H
class AbstractSpellerInterface
{
private:
protected:
public:
  virtual std::vector<TCHAR*> *GetLanguageList () = 0;
  virtual void SetLanguage (TCHAR *Lang) = 0;
  virtual void SetMultipleLanguages (std::vector<TCHAR *> *List) = 0; // Languages are from LangList
  virtual void SetMode (int Multi) { MultiMode = Multi; }                         // Multi - 1, Single - 0
  virtual BOOL CheckWord (char *Word) = 0;                         // Word in Utf-8 or ANSI (For now only Utf-8)
  virtual std::vector<char *> *GetSuggestions (char *Word) = 0;
  virtual void AddToDictionary (char *Word) = 0;
  virtual void IgnoreAll (char *Word) = 0;
  virtual BOOL IsWorking () = 0;

private:
protected:
  int MultiMode;
public:
};
#endif // ABSTRACTSPELLERINTERFACE_H