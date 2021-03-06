/*
 * Copyright 2010, 2011, 2012 Mike Blumenkrantz <michael.blumenkrantz@gmail.com>
 */

#include "T_Test1.azy_server.h"
#include "T_Test2.azy_server.h"
#ifdef HAVE_MYSQL
#include "T_SQL.azy_server.h"
#endif

Eina_Bool
server_suspend(Azy_Server_Module *m)
{
   Eina_Value *v;
   Azy_Content *content;

   if (!azy_server_module_active_get(m)) return EINA_FALSE;
   v = azy_value_util_string_new("that was crazy!");
   content = azy_server_module_content_get(m);
   azy_content_retval_set(content, v);

   azy_server_module_events_resume(m, EINA_TRUE);
   return EINA_FALSE;
}

int
main(void)
{
   azy_init();
   Azy_Server_Module_Def *modules[] = {
                                          T_Test1_module_def(),
                                          T_Test2_module_def(),
#ifdef HAVE_MYSQL
                                          T_SQL_module_def(),
#endif
                                           NULL
                                        };

   eina_log_domain_level_set("azy", EINA_LOG_LEVEL_DBG);

//   azy_server_basic_run(4444, AZY_SERVER_TLS | AZY_SERVER_BROADCAST, "server.pem", modules);
   azy_server_basic_run(4444, AZY_SERVER_BROADCAST, NULL, modules);

   azy_shutdown();
   return 0;
}

