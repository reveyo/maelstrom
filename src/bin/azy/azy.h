/*
 * Copyright 2006-2008 Ondrej Jirman <ondrej.jirman@zonio.net>
 * Copyright 2010, 2011, 2012 Mike Blumenkrantz <michael.blumenkrantz@gmail.com>
 */

#ifndef AZY_H
#define AZY_H

#include <Eina.h>
#include <stdlib.h>

#ifndef strdupa
# define strdupa(str)       strcpy(alloca(strlen(str) + 1), str)
#endif

#ifndef strndupa
# define strndupa(str, len) strncpy(alloca(len + 1), str, len)
#endif

typedef enum
{
   TD_BASE,
   TD_STRUCT,
   TD_ARRAY,
} Azy_Base_Type;

typedef struct _Azy_Typedef       Azy_Typedef;
typedef struct _Azy_Struct_Member Azy_Struct_Member;
typedef struct _Azy_Method_Param  Azy_Method_Param;
typedef struct _Azy_Method        Azy_Method;
typedef struct _Azy_Server_Module Azy_Server_Module;
typedef struct _Azy_Model         Azy_Model;
typedef struct _Azy_Error_Code    Azy_Error_Code;

struct _Azy_Typedef
{
   Azy_Base_Type       type; /* typedef node type */
   Eina_Stringshare   *name; /* name of the type (for use in ZER) */
   Eina_Stringshare   *cname; /* name of the type (for use in ZER) */
   Eina_Stringshare   *ctype; /* C type name */
   Eina_Stringshare   *cnull; /* null value in C for this type */
   size_t             csize; /* size of type */
   Azy_Server_Module *module; /* module owning this type */
   Eina_Stringshare   *etype; /* eina value type for parsing */

   const char         *march_name;
   const char         *demarch_name;
   const char         *free_func;
   const char         *copy_func;
   const char         *eq_func;
   const char         *print_func;
   const char         *fmt_str;
   const char         *isnull_func;
   const char         *hash_func;

   Eina_List          *struct_members; /* struct members list */
   Azy_Typedef       *item_type; /* array item type */
   const char         *doc;
};

struct _Azy_Struct_Member
{
   const char   *name;
   const char   *strname;
   Azy_Typedef *type;
};

struct _Azy_Method_Param
{
   const char   *name;
   int           pass_ownership;
   Azy_Typedef *type;
};

struct _Azy_Method
{
   const char   *name;
   Eina_List    *params;
   Azy_Typedef *return_type;
   const char   *stub_impl;
   int           stub_impl_line;
   const char   *doc;
};

struct _Azy_Server_Module
{
   const char *name;

   Eina_List  *methods;      /* methods */
   Eina_List  *errors;
   double      version;

   const char *stub_header;
   const char *stub_init;
   const char *stub_shutdown;
   const char *stub_attrs;
   const char *stub_pre;
   const char *stub_post;
   const char *stub_fallback;
   const char *stub_download;
   const char *stub_upload;
   int         stub_header_line;
   int         stub_init_line;
   int         stub_shutdown_line;
   int         stub_attrs_line;
   int         stub_pre_line;
   int         stub_post_line;
   int         stub_fallback_line;
   int         stub_download_line;
   int         stub_upload_line;

   const char *doc;
};

struct _Azy_Error_Code
{
   const char *name;
   const char *cname;      /* C enum value */
   const char *msg;
   int         code;
   const char *doc;
};

struct _Azy_Model
{
   const char         *name;
   Eina_List          *errors;
   Eina_List          *modules;
   Eina_List          *types; /* global types */

   // parser helpers
   Azy_Server_Module *cur_module;
};

Azy_Model *azy_new(void);
Azy_Model *azy_parse_string_azy(const char *str,
                              Eina_Bool  *err);
Azy_Model *azy_parse_file_azy(const char *str,
                            Eina_Bool  *err);

Azy_Error_Code *azy_error_new(Azy_Model         *azy,
                                const char         *name,
                                int                 code,
                                const char         *msg);

Azy_Typedef *azy_typedef_new_array(Azy_Model         *azy,
                                     Azy_Typedef       *item);
Azy_Typedef *azy_typedef_new_struct(Azy_Model         *azy,
                                      const char         *name);

int azy_method_compare(Azy_Method *m1,
                        Azy_Method *m2);

Azy_Typedef *azy_typedef_find(Azy_Model         *azy,
                                const char         *name);
const char *azy_typedef_azy_name(Azy_Typedef *t);

const char *azy_stringshare_toupper(const char *str);

#endif
