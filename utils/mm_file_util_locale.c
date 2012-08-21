/*
 * libmm-fileinfo
 *
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: Haejeong Kim <backto.kim@samsung.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
 
#include <stdlib.h> 
#include <string.h>

#ifdef GCONF_SUPPORT
#include <gconf/gconf-client.h>
#endif

#include "mm_debug.h"
#include "mm_file_utils.h"

/* This macro is the same with global-gconf.h */
#define MMFILE_LANGUAGETYPE_REPOSITORY      "/Apps/Settings/language_type"
typedef enum
{
    MMFILE_LANGUAGE_ENGLISH = 0x00,     /**<Language - English*/
    MMFILE_LANGUAGE_GERMAN,             /**<Language - German*/
    MMFILE_LANGUAGE_FRENCH,             /**<Language - French*/
    MMFILE_LANGUAGE_ITALIAN,            /**<Language - Italian*/
    MMFILE_LANGUAGE_DUTCH,              /**<Language - Dutch*/
    MMFILE_LANGUAGE_SPANISH,            /**<Language - Spanish*/
    MMFILE_LANGUAGE_GREEK,              /**<Language - Greek*/
    MMFILE_LANGUAGE_PORTUGUESE,         /**<Language - Portuguese*/
    MMFILE_LANGUAGE_TURKISH,            /**<Language - Turkish*/
#if 0        
    MMFILE_LANGUAGE_BULGARIAN,          /**<Language - Bulgarian*/
    MMFILE_LANGUAGE_ARABIC,             /**<Language - Arabic*/
#endif
    MMFILE_LANGUAGE_MAX
} eMMFileSettingPhoneLanguage;

const char *MMFILE_LOCALE_TABLE [MMFILE_LANGUAGE_MAX] =
{
    "EUC-KR",       /* Temporally - Language - English */
    "ISO8859-1",       /* Language - German */
    "ISO8859-1",       /* Language - French */
    "ISO8859-1",       /* Language - Italian */
    "ISO8859-1",       /* Temporally -  Language - Dutch */
    "ISO8859-1",       /* Language - Spanish */
    "ISO8859-7",    /* Language - Greek */
    "ISO8859-1",       /* Language - Portuguese */
    "ISO8859-3",       /* Language - Turkish */
#if 0
                    /* Language - Bulgarian */
                    /* Language - Arabic */
#endif
};


EXPORT_API
char *MMFileUtilGetLocale (int *error)
{
#ifdef GCONF_SUPPORT
    GConfClient *client = NULL;
#endif
    int index = 0;
    int err = 0;
   
    /* get enum of language through gconf*/
#ifdef GCONF_SUPPORT
    g_type_init();
#endif
    
#ifdef GCONF_SUPPORT
    client = gconf_client_get_default ();
    if (client)
    {
        index = gconf_client_get_int (client, MMFILE_LANGUAGETYPE_REPOSITORY, NULL);
        g_object_unref (client);
    }
    else
#endif
    {
        index = 0;
        //debug_warning ("fail to get gconf-client\n");
    }
    
    if (index < 0 || index >= MMFILE_LANGUAGE_MAX)
    {
        debug_error ("invalid index\n");
        err = MMFILE_UTIL_FAIL;
        return NULL;
    }

    err = MMFILE_UTIL_SUCCESS;
    if (error)
    {
        *error = err;
    }
    
    return (char *)MMFILE_LOCALE_TABLE[index];
}

