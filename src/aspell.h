/* Automatically generated file.  Do not edit directly. */

/* This file is part of The New Aspell
* Copyright (C) 2001-2002 by Kevin Atkinson under the GNU LGPL
* license version 2.0 or 2.1.  You should have received a copy of the
* LGPL license along with this library if you did not you can find it
* at http://www.gnu.org/.                                              */

#ifndef ASPELL_ASPELL__H
#define ASPELL_ASPELL__H

BOOL LoadAspell(TCHAR *PathArg);
void GetActualAspellPath (TCHAR *&Path, TCHAR *&PathArg);
void GetDefaultAspellPath (TCHAR *&Path);
void UnloadAspell(void);
void AspellErrorMsgBox(HWND hWnd, LPCSTR error);

#ifdef __cplusplus
extern "C" {
#endif

  /******************************* type id *******************************/

  union AspellTypeId {
    unsigned int num;

    char str[4];
  };

  typedef union AspellTypeId AspellTypeId;

  /************************** mutable container **************************/

  typedef struct AspellMutableContainer AspellMutableContainer;

  typedef int (__cdecl * PFUNC_aspell_mutable_container_add)(struct AspellMutableContainer * ths, const char * to_add);

  typedef int (__cdecl * PFUNC_aspell_mutable_container_remove)(struct AspellMutableContainer * ths, const char * to_rem);

  typedef void (__cdecl * PFUNC_aspell_mutable_container_clear)(struct AspellMutableContainer * ths);

  typedef struct AspellMutableContainer * (__cdecl * PFUNC_aspell_mutable_container_to_mutable_container)(struct AspellMutableContainer * ths);

  /******************************* key info *******************************/

  enum AspellKeyInfoType {AspellKeyInfoString, AspellKeyInfoInt, AspellKeyInfoBool, AspellKeyInfoList};
  typedef enum AspellKeyInfoType AspellKeyInfoType;

  struct AspellKeyInfo {
    /* the name of the key */
    const char * name;

    /* the key type */
    enum AspellKeyInfoType type;

    /* the default value of the key */
    const char * def;

    /* a brief description of the key or null if internal value */
    const char * desc;

    /* other data used by config implementations
    * should be set to 0 if not used */
    char otherdata[16];
  };

  typedef struct AspellKeyInfo AspellKeyInfo;

  /******************************** config ********************************/

  typedef struct AspellKeyInfoEnumeration AspellKeyInfoEnumeration;

  typedef int (__cdecl * PFUNC_aspell_key_info_enumeration_at_end)(const struct AspellKeyInfoEnumeration * ths);

  typedef const struct AspellKeyInfo * (__cdecl * PFUNC_aspell_key_info_enumeration_next)(struct AspellKeyInfoEnumeration * ths);

  typedef void (__cdecl * PFUNC_delete_aspell_key_info_enumeration)(struct AspellKeyInfoEnumeration * ths);

  typedef struct AspellKeyInfoEnumeration * (__cdecl * PFUNC_aspell_key_info_enumeration_clone)(const struct AspellKeyInfoEnumeration * ths);

  typedef void (__cdecl * PFUNC_aspell_key_info_enumeration_assign)(struct AspellKeyInfoEnumeration * ths, const struct AspellKeyInfoEnumeration * other);

  typedef struct AspellConfig AspellConfig;

  typedef struct AspellConfig * (__cdecl * PFUNC_new_aspell_config)();

  typedef void (__cdecl * PFUNC_delete_aspell_config)(struct AspellConfig * ths);

  typedef struct AspellConfig * (__cdecl * PFUNC_aspell_config_clone)(const struct AspellConfig * ths);

  typedef void (__cdecl * PFUNC_aspell_config_assign)(struct AspellConfig * ths, const struct AspellConfig * other);

  typedef unsigned int (__cdecl * PFUNC_aspell_config_error_number)(const struct AspellConfig * ths);

  typedef const char * (__cdecl * PFUNC_aspell_config_error_message)(const struct AspellConfig * ths);

  typedef const struct AspellError * (__cdecl * PFUNC_aspell_config_error)(const struct AspellConfig * ths);

  /* sets extra keys which this config class should accept
  * begin and end are expected to point to the begging
  * and end of an array of Aspell Key Info */
  typedef void (__cdecl * PFUNC_aspell_config_set_extra)(struct AspellConfig * ths, const struct AspellKeyInfo * begin, const struct AspellKeyInfo * end);

  /* returns the KeyInfo object for the
  * corresponding key or returns null and sets
  * error_num to PERROR_UNKNOWN_KEY if the key is
  * not valid. The pointer returned is valid for
  * the lifetime of the object. */
  typedef const struct AspellKeyInfo * (__cdecl * PFUNC_aspell_config_keyinfo)(struct AspellConfig * ths, const char * key);

  /* returns a newly allocated enumeration of all the
  * possible objects this config class uses */
  typedef struct AspellKeyInfoEnumeration * (__cdecl * PFUNC_aspell_config_possible_elements)(struct AspellConfig * ths, int include_extra);

  /* returns the default value for given key which
  * way involve substating variables, thus it is
  * not the same as keyinfo(key)->def returns null
  * and sets error_num to PERROR_UNKNOWN_KEY if
  * the key is not valid. Uses the temporary
  * string. */
  typedef const char * (__cdecl * PFUNC_aspell_config_get_default)(struct AspellConfig * ths, const char * key);

  /* returns a newly alloacted enumeration of all the
  * key/value pairs. This DOES not include ones
  * which are set to their default values */
  typedef struct AspellStringPairEnumeration * (__cdecl * PFUNC_aspell_config_elements)(struct AspellConfig * ths);

  /* inserts an item, if the item already exists it
  * will be replaced. returns true if it succesed
  * or false on error. If the key in not valid it
  * sets error_num to PERROR_UNKNOWN_KEY, if the
  * value is not valid it will sets error_num to
  * PERROR_BAD_VALUE, if the value can not be
  * changed it sets error_num to
  * PERROR_CANT_CHANGE_VALUE, and if the value is
  * a list and you are trying to set it directory
  * it sets error_num to PERROR_LIST_SET */
  typedef int (__cdecl * PFUNC_aspell_config_replace)(struct AspellConfig * ths, const char * key, const char * value);

  /* remove a key and returns true if it exists
  * otherise return false. This effictly sets the
  * key to its default value. Calling replace with
  * a value of "<default>" will also call
  * remove. If the key does not exists sets
  * error_num to 0 or PERROR_NOT, if the key in
  * not valid sets error_num to
  * PERROR_UNKNOWN_KEY, if the value can not be
  * changed sets error_num to
  * PERROR_CANT_CHANGE_VALUE */
  typedef int (__cdecl * PFUNC_aspell_config_remove)(struct AspellConfig * ths, const char * key);

  typedef int (__cdecl * PFUNC_aspell_config_have)(const struct AspellConfig * ths, const char * key);

  /* returns null on error */
  typedef const char * (__cdecl * PFUNC_aspell_config_retrieve)(struct AspellConfig * ths, const char * key);

  typedef int (__cdecl * PFUNC_aspell_config_retrieve_list)(struct AspellConfig * ths, const char * key, struct AspellMutableContainer * lst);

  /* return -1 on error, 0 if false, 1 if true */
  typedef int (__cdecl * PFUNC_aspell_config_retrieve_bool)(struct AspellConfig * ths, const char * key);

  /* return -1 on error */
  typedef int (__cdecl * PFUNC_aspell_config_retrieve_int)(struct AspellConfig * ths, const char * key);

  /******************************** error ********************************/

  struct AspellError {
    const char * mesg;

    const struct AspellErrorInfo * err;
  };

  typedef struct AspellError AspellError;

  int aspell_error_is_a(const struct AspellError * ths, const struct AspellErrorInfo * e);

  struct AspellErrorInfo {
    const struct AspellErrorInfo * isa;

    const char * mesg;

    unsigned int num_parms;

    const char * parms[3];
  };

  typedef struct AspellErrorInfo AspellErrorInfo;

  /**************************** can have error ****************************/

  typedef struct AspellCanHaveError AspellCanHaveError;

  typedef unsigned int (__cdecl * PFUNC_aspell_error_number)(const struct AspellCanHaveError * ths);

  typedef const char * (__cdecl * PFUNC_aspell_error_message)(const struct AspellCanHaveError * ths);

  typedef const struct AspellError * (__cdecl * PFUNC_aspell_error)(const struct AspellCanHaveError * ths);

  typedef void (__cdecl * PFUNC_delete_aspell_can_have_error)(struct AspellCanHaveError * ths);

  /******************************** errors ********************************/

  extern const struct AspellErrorInfo * const aerror_other;
  extern const struct AspellErrorInfo * const aerror_operation_not_supported;
  extern const struct AspellErrorInfo * const   aerror_cant_copy;
  extern const struct AspellErrorInfo * const aerror_file;
  extern const struct AspellErrorInfo * const   aerror_cant_open_file;
  extern const struct AspellErrorInfo * const     aerror_cant_read_file;
  extern const struct AspellErrorInfo * const     aerror_cant_write_file;
  extern const struct AspellErrorInfo * const   aerror_invalid_name;
  extern const struct AspellErrorInfo * const   aerror_bad_file_format;
  extern const struct AspellErrorInfo * const aerror_dir;
  extern const struct AspellErrorInfo * const   aerror_cant_read_dir;
  extern const struct AspellErrorInfo * const aerror_config;
  extern const struct AspellErrorInfo * const   aerror_unknown_key;
  extern const struct AspellErrorInfo * const   aerror_cant_change_value;
  extern const struct AspellErrorInfo * const   aerror_bad_key;
  extern const struct AspellErrorInfo * const   aerror_bad_value;
  extern const struct AspellErrorInfo * const   aerror_duplicate;
  extern const struct AspellErrorInfo * const aerror_language_related;
  extern const struct AspellErrorInfo * const   aerror_unknown_language;
  extern const struct AspellErrorInfo * const   aerror_unknown_soundslike;
  extern const struct AspellErrorInfo * const   aerror_language_not_supported;
  extern const struct AspellErrorInfo * const   aerror_no_wordlist_for_lang;
  extern const struct AspellErrorInfo * const   aerror_mismatched_language;
  extern const struct AspellErrorInfo * const aerror_encoding;
  extern const struct AspellErrorInfo * const   aerror_unknown_encoding;
  extern const struct AspellErrorInfo * const   aerror_encoding_not_supported;
  extern const struct AspellErrorInfo * const   aerror_conversion_not_supported;
  extern const struct AspellErrorInfo * const aerror_pipe;
  extern const struct AspellErrorInfo * const   aerror_cant_create_pipe;
  extern const struct AspellErrorInfo * const   aerror_process_died;
  extern const struct AspellErrorInfo * const aerror_bad_input;
  extern const struct AspellErrorInfo * const   aerror_invalid_word;
  extern const struct AspellErrorInfo * const   aerror_word_list_flags;
  extern const struct AspellErrorInfo * const     aerror_invalid_flag;
  extern const struct AspellErrorInfo * const     aerror_conflicting_flags;

  /******************************* speller *******************************/

  typedef struct AspellSpeller AspellSpeller;

  typedef struct AspellCanHaveError * (__cdecl * PFUNC_new_aspell_speller)(struct AspellConfig * config);

  typedef struct AspellSpeller * (__cdecl * PFUNC_to_aspell_speller)(struct AspellCanHaveError * obj);

  typedef void (__cdecl * PFUNC_delete_aspell_speller)(struct AspellSpeller * ths);

  typedef unsigned int (__cdecl * PFUNC_aspell_speller_error_number)(const struct AspellSpeller * ths);

  typedef const char * (__cdecl * PFUNC_aspell_speller_error_message)(const struct AspellSpeller * ths);

  typedef const struct AspellError * (__cdecl * PFUNC_aspell_speller_error)(const struct AspellSpeller * ths);

  typedef struct AspellConfig * (__cdecl * PFUNC_aspell_speller_config)(struct AspellSpeller * ths);

  /* returns  0 if it is not in the dictionary,
  * 1 if it is, or -1 on error. */
  typedef int (__cdecl * PFUNC_aspell_speller_check)(struct AspellSpeller * ths, const char * word, int word_size);

  typedef int (__cdecl * PFUNC_aspell_speller_add_to_personal)(struct AspellSpeller * ths, const char * word, int word_size);

  typedef int (__cdecl * PFUNC_aspell_speller_add_to_session)(struct AspellSpeller * ths, const char * word, int word_size);

  typedef const struct AspellWordList * (__cdecl * PFUNC_aspell_speller_personal_word_list)(struct AspellSpeller * ths);

  typedef const struct AspellWordList * (__cdecl * PFUNC_aspell_speller_session_word_list)(struct AspellSpeller * ths);

  typedef const struct AspellWordList * (__cdecl * PFUNC_aspell_speller_main_word_list)(struct AspellSpeller * ths);

  typedef int (__cdecl * PFUNC_aspell_speller_save_all_word_lists)(struct AspellSpeller * ths);

  typedef int (__cdecl * PFUNC_aspell_speller_clear_session)(struct AspellSpeller * ths);

  /* Return null on error.
  * the word list returned by suggest is only valid until the next
  * call to suggest */
  typedef const struct AspellWordList * (__cdecl * PFUNC_aspell_speller_suggest)(struct AspellSpeller * ths, const char * word, int word_size);

  typedef int (__cdecl * PFUNC_aspell_speller_store_replacement)(struct AspellSpeller * ths, const char * mis, int mis_size, const char * cor, int cor_size);

  /******************************** filter ********************************/

  typedef struct AspellFilter AspellFilter;

  typedef void (__cdecl * PFUNC_delete_aspell_filter)(struct AspellFilter * ths);

  typedef unsigned int (__cdecl * PFUNC_aspell_filter_error_number)(const struct AspellFilter * ths);

  typedef const char * (__cdecl * PFUNC_aspell_filter_error_message)(const struct AspellFilter * ths);

  typedef const struct AspellError * (__cdecl * PFUNC_aspell_filter_error)(const struct AspellFilter * ths);

  typedef struct AspellFilter * (__cdecl * PFUNC_to_aspell_filter)(struct AspellCanHaveError * obj);

  /*************************** document checker ***************************/

  struct AspellToken {
    unsigned int offset;

    unsigned int len;
  };

  typedef struct AspellToken AspellToken;

  typedef struct AspellDocumentChecker AspellDocumentChecker;

  typedef void (__cdecl * PFUNC_delete_aspell_document_checker)(struct AspellDocumentChecker * ths);

  typedef unsigned int (__cdecl * PFUNC_aspell_document_checker_error_number)(const struct AspellDocumentChecker * ths);

  typedef const char * (__cdecl * PFUNC_aspell_document_checker_error_message)(const struct AspellDocumentChecker * ths);

  typedef const struct AspellError * (__cdecl * PFUNC_aspell_document_checker_error)(const struct AspellDocumentChecker * ths);

  /* Creates a new document checker.
  * The speller class is expect to last until this
  * class is destroyed.
  * If config is given it will be used to overwide
  * any relevent options set by this speller class.
  * The config class is not once this function is done.
  * If filter is given then it will take ownership of
  * the filter class and use it to do the filtering.
  * You are expected to free the checker when done. */
  typedef struct AspellCanHaveError * (__cdecl * PFUNC_new_aspell_document_checker)(struct AspellSpeller * speller);

  typedef struct AspellDocumentChecker * (__cdecl * PFUNC_to_aspell_document_checker)(struct AspellCanHaveError * obj);

  /* reset the internal state of the filter.
  * should be called whenever a new document is being filtered */
  typedef void (__cdecl * PFUNC_aspell_document_checker_reset)(struct AspellDocumentChecker * ths);

  /* process a string
  * The string passed in should only be split on white space
  * characters.  Furthermore, between calles to reset, each string
  * should be passed in exactly once and in the order they appeared
  * in the document.  Passing in stings out of order, skipping
  * strings or passing them in more than once may lead to undefined
  * results. */
  typedef void (__cdecl * PFUNC_aspell_document_checker_process)(struct AspellDocumentChecker * ths, const char * str, int size);

  /* returns the next misspelled word in the processed string
  * if there are no more misspelled word than token.word
  * will be null and token.size will be 0 */
  typedef struct AspellToken (__cdecl * PFUNC_aspell_document_checker_next_misspelling)(struct AspellDocumentChecker * ths);

  /* returns the underlying filter class */
  typedef struct AspellFilter * (__cdecl * PFUNC_aspell_document_checker_filter)(struct AspellDocumentChecker * ths);

  /****************************** word list ******************************/

  typedef struct AspellWordList AspellWordList;

  typedef int (__cdecl * PFUNC_aspell_word_list_empty)(const struct AspellWordList * ths);

  typedef unsigned int (__cdecl * PFUNC_aspell_word_list_size)(const struct AspellWordList * ths);

  typedef struct AspellStringEnumeration * (__cdecl * PFUNC_aspell_word_list_elements)(const struct AspellWordList * ths);

  /************************** string enumeration **************************/

  typedef struct AspellStringEnumeration AspellStringEnumeration;

  typedef void (__cdecl * PFUNC_delete_aspell_string_enumeration)(struct AspellStringEnumeration * ths);

  typedef struct AspellStringEnumeration * (__cdecl * PFUNC_aspell_string_enumeration_clone)(const struct AspellStringEnumeration * ths);

  typedef void (__cdecl * PFUNC_aspell_string_enumeration_assign)(struct AspellStringEnumeration * ths, const struct AspellStringEnumeration * other);

  typedef int (__cdecl * PFUNC_aspell_string_enumeration_at_end)(const struct AspellStringEnumeration * ths);

  typedef const char * (__cdecl * PFUNC_aspell_string_enumeration_next)(struct AspellStringEnumeration * ths);

  /********************************* info *********************************/

  struct AspellModuleInfo {
    const char * name;

    double order_num;

    const char * lib_dir;

    struct AspellStringList * dict_dirs;

    struct AspellStringList * dict_exts;
  };

  typedef struct AspellModuleInfo AspellModuleInfo;

  struct AspellDictInfo {
    /* name to identify the dictionary by */
    const char * name;

    const char * code;

    const char * jargon;

    int size;

    const char * size_str;

    struct AspellModuleInfo * module;
  };

  typedef struct AspellDictInfo AspellDictInfo;

  typedef struct AspellModuleInfoList AspellModuleInfoList;

  typedef struct AspellModuleInfoList * (__cdecl * PFUNC_get_aspell_module_info_list)(struct AspellConfig * config);

  typedef int (__cdecl * PFUNC_aspell_module_info_list_empty)(const struct AspellModuleInfoList * ths);

  typedef unsigned int (__cdecl * PFUNC_aspell_module_info_list_size)(const struct AspellModuleInfoList * ths);

  typedef struct AspellModuleInfoEnumeration * (__cdecl * PFUNC_aspell_module_info_list_elements)(const struct AspellModuleInfoList * ths);

  typedef struct AspellDictInfoList AspellDictInfoList;

  typedef struct AspellDictInfoList * (__cdecl * PFUNC_get_aspell_dict_info_list)(struct AspellConfig * config);

  typedef int (__cdecl * PFUNC_aspell_dict_info_list_empty)(const struct AspellDictInfoList * ths);

  typedef unsigned int (__cdecl * PFUNC_aspell_dict_info_list_size)(const struct AspellDictInfoList * ths);

  typedef struct AspellDictInfoEnumeration * (__cdecl * PFUNC_aspell_dict_info_list_elements)(const struct AspellDictInfoList * ths);

  typedef struct AspellModuleInfoEnumeration AspellModuleInfoEnumeration;

  typedef int (__cdecl * PFUNC_aspell_module_info_enumeration_at_end)(const struct AspellModuleInfoEnumeration * ths);

  typedef const struct AspellModuleInfo * (__cdecl * PFUNC_aspell_module_info_enumeration_next)(struct AspellModuleInfoEnumeration * ths);

  typedef void (__cdecl * PFUNC_delete_aspell_module_info_enumeration)(struct AspellModuleInfoEnumeration * ths);

  typedef struct AspellModuleInfoEnumeration * (__cdecl * PFUNC_aspell_module_info_enumeration_clone)(const struct AspellModuleInfoEnumeration * ths);

  typedef void (__cdecl * PFUNC_aspell_module_info_enumeration_assign)(struct AspellModuleInfoEnumeration * ths, const struct AspellModuleInfoEnumeration * other);

  typedef struct AspellDictInfoEnumeration AspellDictInfoEnumeration;

  typedef int (__cdecl * PFUNC_aspell_dict_info_enumeration_at_end)(const struct AspellDictInfoEnumeration * ths);

  typedef const struct AspellDictInfo * (__cdecl * PFUNC_aspell_dict_info_enumeration_next)(struct AspellDictInfoEnumeration * ths);

  typedef void (__cdecl * PFUNC_delete_aspell_dict_info_enumeration)(struct AspellDictInfoEnumeration * ths);

  typedef struct AspellDictInfoEnumeration * (__cdecl * PFUNC_aspell_dict_info_enumeration_clone)(const struct AspellDictInfoEnumeration * ths);

  typedef void (__cdecl * PFUNC_aspell_dict_info_enumeration_assign)(struct AspellDictInfoEnumeration * ths, const struct AspellDictInfoEnumeration * other);

  /***************************** string list *****************************/

  typedef struct AspellStringList AspellStringList;

  typedef struct AspellStringList * (__cdecl * PFUNC_new_aspell_string_list)();

  typedef int (__cdecl * PFUNC_aspell_string_list_empty)(const struct AspellStringList * ths);

  typedef unsigned int (__cdecl * PFUNC_aspell_string_list_size)(const struct AspellStringList * ths);

  typedef struct AspellStringEnumeration * (__cdecl * PFUNC_aspell_string_list_elements)(const struct AspellStringList * ths);

  typedef int (__cdecl * PFUNC_aspell_string_list_add)(struct AspellStringList * ths, const char * to_add);

  typedef int (__cdecl * PFUNC_aspell_string_list_remove)(struct AspellStringList * ths, const char * to_rem);

  typedef void (__cdecl * PFUNC_aspell_string_list_clear)(struct AspellStringList * ths);

  typedef struct AspellMutableContainer * (__cdecl * PFUNC_aspell_string_list_to_mutable_container)(struct AspellStringList * ths);

  typedef void (__cdecl * PFUNC_delete_aspell_string_list)(struct AspellStringList * ths);

  typedef struct AspellStringList * (__cdecl * PFUNC_aspell_string_list_clone)(const struct AspellStringList * ths);

  typedef void (__cdecl * PFUNC_aspell_string_list_assign)(struct AspellStringList * ths, const struct AspellStringList * other);

  /****************************** string map ******************************/

  typedef struct AspellStringMap AspellStringMap;

  typedef struct AspellStringMap * (__cdecl * PFUNC_new_aspell_string_map)();

  typedef int (__cdecl * PFUNC_aspell_string_map_add)(struct AspellStringMap * ths, const char * to_add);

  typedef int (__cdecl * PFUNC_aspell_string_map_remove)(struct AspellStringMap * ths, const char * to_rem);

  typedef void (__cdecl * PFUNC_aspell_string_map_clear)(struct AspellStringMap * ths);

  typedef struct AspellMutableContainer * (__cdecl * PFUNC_aspell_string_map_to_mutable_container)(struct AspellStringMap * ths);

  typedef void (__cdecl * PFUNC_delete_aspell_string_map)(struct AspellStringMap * ths);

  typedef struct AspellStringMap * (__cdecl * PFUNC_aspell_string_map_clone)(const struct AspellStringMap * ths);

  typedef void (__cdecl * PFUNC_aspell_string_map_assign)(struct AspellStringMap * ths, const struct AspellStringMap * other);

  typedef int (__cdecl * PFUNC_aspell_string_map_empty)(const struct AspellStringMap * ths);

  typedef unsigned int (__cdecl * PFUNC_aspell_string_map_size)(const struct AspellStringMap * ths);

  typedef struct AspellStringPairEnumeration * (__cdecl * PFUNC_aspell_string_map_elements)(const struct AspellStringMap * ths);

  /* Insert a new element.
  * Will NOT overright an existing entry.
  * Returns false if the element already exists. */
  typedef int (__cdecl * PFUNC_aspell_string_map_insert)(struct AspellStringMap * ths, const char * key, const char * value);

  /* Insert a new element.
  * Will overright an existing entry.
  * Always returns true. */
  typedef int (__cdecl * PFUNC_aspell_string_map_replace)(struct AspellStringMap * ths, const char * key, const char * value);

  /* Looks up an element.
  * Returns null if the element did not exist.
  * Returns an empty string if the element exists but has a null value.
  * Otherwises returns the value */
  typedef const char * (__cdecl * PFUNC_aspell_string_map_lookup)(const struct AspellStringMap * ths, const char * key);

  /***************************** string pair *****************************/

  struct AspellStringPair {
    const char * first;

    const char * second;
  };

  typedef struct AspellStringPair AspellStringPair;

  /*********************** string pair enumeration ***********************/

  typedef struct AspellStringPairEnumeration AspellStringPairEnumeration;

  typedef int (__cdecl * PFUNC_aspell_string_pair_enumeration_at_end)(const struct AspellStringPairEnumeration * ths);

  typedef struct AspellStringPair (__cdecl * PFUNC_aspell_string_pair_enumeration_next)(struct AspellStringPairEnumeration * ths);

  typedef void (__cdecl * PFUNC_delete_aspell_string_pair_enumeration)(struct AspellStringPairEnumeration * ths);

  typedef struct AspellStringPairEnumeration * (__cdecl * PFUNC_aspell_string_pair_enumeration_clone)(const struct AspellStringPairEnumeration * ths);

  typedef void (__cdecl * PFUNC_aspell_string_pair_enumeration_assign)(struct AspellStringPairEnumeration * ths, const struct AspellStringPairEnumeration * other);

#ifdef __cplusplus
}
#endif

extern PFUNC_aspell_mutable_container_add                  aspell_mutable_container_add;
extern PFUNC_aspell_mutable_container_remove               aspell_mutable_container_remove;
extern PFUNC_aspell_mutable_container_clear                aspell_mutable_container_clear;
extern PFUNC_aspell_mutable_container_to_mutable_container aspell_mutable_container_to_mutable_container;
extern PFUNC_aspell_key_info_enumeration_at_end            aspell_key_info_enumeration_at_end;
extern PFUNC_aspell_key_info_enumeration_next              aspell_key_info_enumeration_next;
extern PFUNC_delete_aspell_key_info_enumeration            delete_aspell_key_info_enumeration;
extern PFUNC_aspell_key_info_enumeration_clone             aspell_key_info_enumeration_clone;
extern PFUNC_aspell_key_info_enumeration_assign            aspell_key_info_enumeration_assign;
extern PFUNC_new_aspell_config                             new_aspell_config;
extern PFUNC_delete_aspell_config                          delete_aspell_config;
extern PFUNC_aspell_config_clone                           aspell_config_clone;
extern PFUNC_aspell_config_assign                          aspell_config_assign;
extern PFUNC_aspell_config_error_number                    aspell_config_error_number;
extern PFUNC_aspell_config_error_message                   aspell_config_error_message;
extern PFUNC_aspell_config_error                           aspell_config_error;
extern PFUNC_aspell_config_set_extra                       aspell_config_set_extra;
extern PFUNC_aspell_config_keyinfo                         aspell_config_keyinfo;
extern PFUNC_aspell_config_possible_elements               aspell_config_possible_elements;
extern PFUNC_aspell_config_get_default                     aspell_config_get_default;
extern PFUNC_aspell_config_elements                        aspell_config_elements;
extern PFUNC_aspell_config_replace                         aspell_config_replace;
extern PFUNC_aspell_config_remove                          aspell_config_remove;
extern PFUNC_aspell_config_have                            aspell_config_have;
extern PFUNC_aspell_config_retrieve                        aspell_config_retrieve;
extern PFUNC_aspell_config_retrieve_list                   aspell_config_retrieve_list;
extern PFUNC_aspell_config_retrieve_bool                   aspell_config_retrieve_bool;
extern PFUNC_aspell_config_retrieve_int                    aspell_config_retrieve_int;
extern PFUNC_aspell_error_number                           aspell_error_number;
extern PFUNC_aspell_error_message                          aspell_error_message;
extern PFUNC_aspell_error                                  aspell_error;
extern PFUNC_delete_aspell_can_have_error                  delete_aspell_can_have_error;
extern PFUNC_new_aspell_speller                            new_aspell_speller;
extern PFUNC_to_aspell_speller                             to_aspell_speller;
extern PFUNC_delete_aspell_speller                         delete_aspell_speller;
extern PFUNC_aspell_speller_error_number                   aspell_speller_error_number;
extern PFUNC_aspell_speller_error_message                  aspell_speller_error_message;
extern PFUNC_aspell_speller_error                          aspell_speller_error;
extern PFUNC_aspell_speller_config                         aspell_speller_config;
extern PFUNC_aspell_speller_check                          aspell_speller_check;
extern PFUNC_aspell_speller_add_to_personal                aspell_speller_add_to_personal;
extern PFUNC_aspell_speller_add_to_session                 aspell_speller_add_to_session;
extern PFUNC_aspell_speller_personal_word_list             aspell_speller_personal_word_list;
extern PFUNC_aspell_speller_session_word_list              aspell_speller_session_word_list;
extern PFUNC_aspell_speller_main_word_list                 aspell_speller_main_word_list;
extern PFUNC_aspell_speller_save_all_word_lists            aspell_speller_save_all_word_lists;
extern PFUNC_aspell_speller_clear_session                  aspell_speller_clear_session;
extern PFUNC_aspell_speller_suggest                        aspell_speller_suggest;
extern PFUNC_aspell_speller_store_replacement              aspell_speller_store_replacement;
extern PFUNC_delete_aspell_filter                          delete_aspell_filter;
extern PFUNC_aspell_filter_error_number                    aspell_filter_error_number;
extern PFUNC_aspell_filter_error_message                   aspell_filter_error_message;
extern PFUNC_aspell_filter_error                           aspell_filter_error;
extern PFUNC_to_aspell_filter                              to_aspell_filter;
extern PFUNC_delete_aspell_document_checker                delete_aspell_document_checker;
extern PFUNC_aspell_document_checker_error_number          aspell_document_checker_error_number;
extern PFUNC_aspell_document_checker_error_message         aspell_document_checker_error_message;
extern PFUNC_aspell_document_checker_error                 aspell_document_checker_error;
extern PFUNC_new_aspell_document_checker                   new_aspell_document_checker;
extern PFUNC_to_aspell_document_checker                    to_aspell_document_checker;
extern PFUNC_aspell_document_checker_reset                 aspell_document_checker_reset;
extern PFUNC_aspell_document_checker_process               aspell_document_checker_process;
extern PFUNC_aspell_document_checker_next_misspelling      aspell_document_checker_next_misspelling;
extern PFUNC_aspell_document_checker_filter                aspell_document_checker_filter;
extern PFUNC_aspell_word_list_empty                        aspell_word_list_empty;
extern PFUNC_aspell_word_list_size                         aspell_word_list_size;
extern PFUNC_aspell_word_list_elements                     aspell_word_list_elements;
extern PFUNC_delete_aspell_string_enumeration              delete_aspell_string_enumeration;
extern PFUNC_aspell_string_enumeration_clone               aspell_string_enumeration_clone;
extern PFUNC_aspell_string_enumeration_assign              aspell_string_enumeration_assign;
extern PFUNC_aspell_string_enumeration_at_end              aspell_string_enumeration_at_end;
extern PFUNC_aspell_string_enumeration_next                aspell_string_enumeration_next;
extern PFUNC_get_aspell_module_info_list                   get_aspell_module_info_list;
extern PFUNC_aspell_module_info_list_empty                 aspell_module_info_list_empty;
extern PFUNC_aspell_module_info_list_size                  aspell_module_info_list_size;
extern PFUNC_aspell_module_info_list_elements              aspell_module_info_list_elements;
extern PFUNC_get_aspell_dict_info_list                     get_aspell_dict_info_list;
extern PFUNC_aspell_dict_info_list_empty                   aspell_dict_info_list_empty;
extern PFUNC_aspell_dict_info_list_size                    aspell_dict_info_list_size;
extern PFUNC_aspell_dict_info_list_elements                aspell_dict_info_list_elements;
extern PFUNC_aspell_module_info_enumeration_at_end         aspell_module_info_enumeration_at_end;
extern PFUNC_aspell_module_info_enumeration_next           aspell_module_info_enumeration_next;
extern PFUNC_delete_aspell_module_info_enumeration         delete_aspell_module_info_enumeration;
extern PFUNC_aspell_module_info_enumeration_clone          aspell_module_info_enumeration_clone;
extern PFUNC_aspell_module_info_enumeration_assign         aspell_module_info_enumeration_assign;
extern PFUNC_aspell_dict_info_enumeration_at_end           aspell_dict_info_enumeration_at_end;
extern PFUNC_aspell_dict_info_enumeration_next             aspell_dict_info_enumeration_next;
extern PFUNC_delete_aspell_dict_info_enumeration           delete_aspell_dict_info_enumeration;
extern PFUNC_aspell_dict_info_enumeration_clone            aspell_dict_info_enumeration_clone;
extern PFUNC_aspell_dict_info_enumeration_assign           aspell_dict_info_enumeration_assign;
extern PFUNC_new_aspell_string_list                        new_aspell_string_list;
extern PFUNC_aspell_string_list_empty                      aspell_string_list_empty;
extern PFUNC_aspell_string_list_size                       aspell_string_list_size;
extern PFUNC_aspell_string_list_elements                   aspell_string_list_elements;
extern PFUNC_aspell_string_list_add                        aspell_string_list_add;
extern PFUNC_aspell_string_list_remove                     aspell_string_list_remove;
extern PFUNC_aspell_string_list_clear                      aspell_string_list_clear;
extern PFUNC_aspell_string_list_to_mutable_container       aspell_string_list_to_mutable_container;
extern PFUNC_delete_aspell_string_list                     delete_aspell_string_list;
extern PFUNC_aspell_string_list_clone                      aspell_string_list_clone;
extern PFUNC_aspell_string_list_assign                     aspell_string_list_assign;
extern PFUNC_new_aspell_string_map                         new_aspell_string_map;
extern PFUNC_aspell_string_map_add                         aspell_string_map_add;
extern PFUNC_aspell_string_map_remove                      aspell_string_map_remove;
extern PFUNC_aspell_string_map_clear                       aspell_string_map_clear;
extern PFUNC_aspell_string_map_to_mutable_container        aspell_string_map_to_mutable_container;
extern PFUNC_delete_aspell_string_map                      delete_aspell_string_map;
extern PFUNC_aspell_string_map_clone                       aspell_string_map_clone;
extern PFUNC_aspell_string_map_assign                      aspell_string_map_assign;
extern PFUNC_aspell_string_map_empty                       aspell_string_map_empty;
extern PFUNC_aspell_string_map_size                        aspell_string_map_size;
extern PFUNC_aspell_string_map_elements                    aspell_string_map_elements;
extern PFUNC_aspell_string_map_insert                      aspell_string_map_insert;
extern PFUNC_aspell_string_map_replace                     aspell_string_map_replace;
extern PFUNC_aspell_string_map_lookup                      aspell_string_map_lookup;
extern PFUNC_aspell_string_pair_enumeration_at_end         aspell_string_pair_enumeration_at_end;
extern PFUNC_aspell_string_pair_enumeration_next           aspell_string_pair_enumeration_next;
extern PFUNC_delete_aspell_string_pair_enumeration         delete_aspell_string_pair_enumeration;
extern PFUNC_aspell_string_pair_enumeration_clone          aspell_string_pair_enumeration_clone;
extern PFUNC_aspell_string_pair_enumeration_assign         aspell_string_pair_enumeration_assign;

#endif /* ASPELL_ASPELL__H */

