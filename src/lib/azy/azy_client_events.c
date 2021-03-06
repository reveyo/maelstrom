/*
 * Copyright 2010, 2011, 2012, 2013 Mike Blumenkrantz <michael.blumenkrantz@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "azy_private.h"

static Eina_Bool _azy_client_handler_get(Azy_Client_Handler_Data *hd);
static Eina_Bool _azy_client_handler_call(Azy_Client_Handler_Data *hd);
static void      _azy_client_handler_data_free(Azy_Client_Handler_Data *data);
static Eina_Bool _azy_client_recv_timer(Azy_Client_Handler_Data *hd);
static void      _azy_client_handler_redirect(Azy_Client_Handler_Data *hd);


static void
_azy_client_handler_data_free(Azy_Client_Handler_Data *hd)
{
   Eina_List *f;
   DBG("(hd=%p, client=%p, net=%p)", hd, hd->client, hd->client->net);
/*
   if (!AZY_MAGIC_CHECK(hd, AZY_MAGIC_CLIENT_DATA_HANDLER))
     return;
 */
   AZY_MAGIC_SET(hd, AZY_MAGIC_NONE);

   f = eina_list_data_find_list(hd->client->conns, hd);
   if (f)
     {
        hd->client->conns = eina_list_remove_list(hd->client->conns, f);

        if (hd->client->conns)
          {
             if (hd->client->recv)
               ecore_event_handler_data_set(hd->client->recv, hd->client->conns->data);
             if (hd->client->net)
               ecore_con_server_data_set(hd->client->net->conn, hd->client);
          }
        else /* if (client->net && client->recv) */
          {
             if (hd->client->recv)
               ecore_event_handler_data_set(hd->client->recv, NULL);
          }
     }

   if (hd->recv)
     azy_net_free(hd->recv);
   eina_stringshare_del(hd->uri);
   if (hd->send) eina_strbuf_free(hd->send);
   free(hd);
}

Eina_Bool
_azy_client_handler_upgrade(Azy_Client_Handler_Data *hd,
                            int type EINA_UNUSED,
                            Ecore_Con_Event_Server_Upgrade *ev)
{
   if (!AZY_MAGIC_CHECK(hd, AZY_MAGIC_CLIENT_DATA_HANDLER))
     return ECORE_CALLBACK_RENEW;

   EINA_SAFETY_ON_NULL_RETURN_VAL(hd->client, ECORE_CALLBACK_RENEW);

   if (!hd->client->net)
     {
        INFO("Removing probably dead client %p", hd->client);
        _azy_client_handler_data_free(hd);
        return ECORE_CALLBACK_RENEW;
     }
   if (hd->client != (Azy_Client *)ecore_con_server_data_get(ev->server))
     {
        DBG("Ignoring callback due to pointer mismatch");
        return ECORE_CALLBACK_PASS_ON;
     }

   hd->client->refcount++;
   hd->client->secure = hd->client->upgraded = EINA_TRUE;
   ecore_event_add(AZY_EVENT_CLIENT_UPGRADE, hd->client, (Ecore_End_Cb)_azy_event_handler_fake_free, azy_client_unref);
   return ECORE_CALLBACK_RENEW;
}

static void
_azy_client_transfer_complete(Azy_Client_Handler_Data *hd, Azy_Content *content)
{
   Azy_Client_Transfer_Complete_Cb cb;
   Azy_Client *client;

   content->id = hd->id;
   content->recv_net = hd->recv;
   client = hd->client;
   hd->recv = NULL;

   cb = eina_hash_find(client->callbacks, &content->id);
   if (cb)
     {
        Eina_Error r;

        client->refcount++;
        r = cb(client, content, content->ret);
        /* FIXME: do something here? */
        eina_hash_del_by_key(client->callbacks, &content->id);
        azy_events_client_transfer_complete_cleanup(client, content);
     }
   else
     azy_events_client_transfer_complete_event(hd, content);
   _azy_client_handler_data_free(hd);
   if (cb) azy_client_unref(client);
}

static Eina_Bool
_azy_client_handler_get(Azy_Client_Handler_Data *hd)
{
   void *ret = NULL;
   Azy_Content *content;

   DBG("(hd=%p, client=%p, net=%p)", hd, hd->client, hd->recv);

   hd->recv->transport = azy_util_transport_get(azy_net_header_get(hd->recv, "content-type"));
   content = azy_content_new(NULL);

   EINA_SAFETY_ON_NULL_RETURN_VAL(content, ECORE_CALLBACK_RENEW);

   content->data = hd->content_data;
   if (hd->recv->buffer_stolen) goto out;

   switch (hd->recv->transport)
     {
      case AZY_NET_TRANSPORT_JSON: /* assume block of json */
      case AZY_NET_TRANSPORT_XML:
      case AZY_NET_TRANSPORT_ATOM:
        if (!azy_content_deserialize(content, hd->recv))
          azy_content_error_faultmsg_set(content, AZY_CLIENT_ERROR_MARSHALIZER, "Call return parsing failed.");
        else if (hd->callback && content->retval && (!hd->callback(content->retval, &ret)))
          azy_content_error_faultmsg_set(content, AZY_CLIENT_ERROR_MARSHALIZER, "Call return value demarshalization failed.");
        if (azy_content_error_is_set(content))
          {
             char buf[64];
             snprintf(buf, sizeof(buf), "%" PRIi64 " bytes:\n<<<<<<<<<<<<<\n%%.%" PRIi64 "s\n<<<<<<<<<<<<<", EBUFLEN(hd->recv->buffer), EBUFLEN(hd->recv->buffer));
             ERR(buf, EBUF(hd->recv->buffer));
          }
        if (ret) content->ret = ret;
        break;
      case AZY_NET_TRANSPORT_HTML: //some asshole sites don't correctly report rss content-type
        if (azy_content_deserialize_xml(content, (char*)EBUF(hd->recv->buffer), EBUFLEN(hd->recv->buffer))) break;
      default:
        content->ret = hd->recv->buffer;
        hd->recv->buffer = NULL;
     }

out:
   _azy_client_transfer_complete(hd, content);

   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_azy_client_handler_call(Azy_Client_Handler_Data *hd)
{
   void *ret = NULL;
   Azy_Content *content;

   DBG("(hd=%p, client=%p, net=%p)", hd, hd->client, hd->recv);

   if (azy_rpc_log_dom >= 0)
     {
        char buf[64];
        snprintf(buf, sizeof(buf), "RECEIVED:\n<<<<<<<<<<<<<\n%%.%" PRIi64 "s\n<<<<<<<<<<<<<", EBUFLEN(hd->recv->buffer));
        RPC_INFO(buf, EBUF(hd->recv->buffer));
     }
   /* handle HTTP GET request */
   if (!hd->method) return _azy_client_handler_get(hd);
   INFO("Running RPC for %s", hd->method);
   hd->recv->transport = azy_util_transport_get(azy_net_header_get(hd->recv, "content-type"));
   content = azy_content_new(hd->method);

   EINA_SAFETY_ON_NULL_RETURN_VAL(content, ECORE_CALLBACK_RENEW);

   content->data = hd->content_data;
   if (hd->recv->buffer_stolen) goto out;

   if (!azy_content_deserialize_response(content, hd->recv->transport, (char *)EBUF(hd->recv->buffer), EBUFLEN(hd->recv->buffer)))
     azy_content_error_faultmsg_set(content, AZY_CLIENT_ERROR_MARSHALIZER, "Call return parsing failed.");
   else if ((hd->recv->transport == AZY_NET_TRANSPORT_JSON) && (content->id != hd->id))
     {
        ERR("Content id: %u  |  Call id: %u", content->id, hd->id);
        azy_content_error_faultmsg_set(content, AZY_CLIENT_ERROR_MARSHALIZER, "Call return id does not match.");
     }
   else if (hd->callback && content->retval && (!hd->callback(content->retval, &ret)))
     azy_content_error_faultmsg_set(content, AZY_CLIENT_ERROR_MARSHALIZER, "Call return value demarshalization failed.");

   if (azy_content_error_is_set(content))
     {
        char buf[64];
        snprintf(buf, sizeof(buf), "%s:\n<<<<<<<<<<<<<\n%%.%" PRIi64 "s\n<<<<<<<<<<<<<", hd->method, EBUFLEN(hd->recv->buffer));
        ERR(buf, EBUF(hd->recv->buffer));
     }
   content->ret = ret;

out:
   _azy_client_transfer_complete(hd, content);

   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_azy_client_recv_timer(Azy_Client_Handler_Data *hd)
{
   if (!AZY_MAGIC_CHECK(hd, AZY_MAGIC_CLIENT_DATA_HANDLER))
     return ECORE_CALLBACK_CANCEL;
   if (!AZY_MAGIC_CHECK(hd->recv, AZY_MAGIC_NET))
     return ECORE_CALLBACK_CANCEL;

   DBG("(hd=%p, client=%p, net=%p)", hd, hd->client, hd->client->net);

   ecore_con_server_flush(hd->client->net->conn);
   if ((hd->recv->http.content_length > 0) && (!azy_events_length_overflows(hd->recv->progress, hd->recv->http.content_length)))
     {
        if (!hd->recv->nodata)
          {
             hd->recv->nodata = EINA_TRUE;
             return ECORE_CALLBACK_RENEW;
          }
        INFO("Server at '%s' timed out!", azy_net_ip_get(hd->recv));
        ecore_con_server_del(hd->recv->conn);
        _azy_client_handler_data_free(hd);
        hd->recv->timer = NULL;
        return ECORE_CALLBACK_CANCEL;
     }

   hd->recv->timer = NULL;
   return ECORE_CALLBACK_CANCEL;
}

static void
_azy_client_handler_redirect(Azy_Client_Handler_Data *hd)
{
   const char *location, *next;
   Eina_Strbuf *msg;
   Eina_Bool reconnect = EINA_FALSE;

   location = azy_net_header_get(hd->recv, "location");
   if (!location)
     {
        /* FIXME */
        azy_net_free(hd->recv);
        hd->recv = NULL;
        azy_events_connection_kill(hd->client->net->conn, EINA_FALSE, NULL);
        return;
     }
   INFO("Handling HTTP %i: redirect to %s", hd->recv->http.res.http_code, location);
   next = strchr(location, '/');
   if (next && (next - location < 8))
     {
        const char *p, *slash;

        p = next + 1;
        if (p && *p && (*p == '/'))
          p += 1;
        slash = p;
        next = strchr(p, '/'); /* try normal uri */
        if (next)
          {
             if (strncmp(slash, hd->client->addr, next - slash))
               {
                  azy_client_addr_set(hd->client, strndupa(slash, next - slash));
                  reconnect = EINA_TRUE;
               }
          }
        else
          next = strchr(p, '?');  /* try php uri */
     }
   if (!next)
     next = "/";  /* flail around wildly hoping someone notices */

   azy_net_uri_set(hd->client->net, next);
   azy_net_type_set(hd->client->net, hd->type);
   msg = azy_net_header_create(hd->client->net);
   EINA_SAFETY_ON_NULL_RETURN(msg);
   if (hd->send)
     {
        azy_net_content_length_set(hd->client->net, eina_strbuf_length_get(hd->send));
        eina_strbuf_prepend_length(hd->send, eina_strbuf_string_get(msg), eina_strbuf_length_get(msg));
        eina_strbuf_free(msg);
     }
   else
     hd->send = msg;

   hd->redirect = EINA_TRUE;

   if (reconnect || (!hd->client->net->proto) || (!hd->recv->proto)) /* handle http 1.0 */
     {
        azy_net_free(hd->recv);
        hd->recv = NULL;
        azy_events_connection_kill(hd->client->net->conn, EINA_FALSE, NULL);
        azy_client_connect(hd->client);
        return;
     }

   azy_net_free(hd->recv);
   hd->recv = NULL;
   EINA_SAFETY_ON_NULL_RETURN(msg);
   /* need to handle redirects, so we get a bit crazy here by sending data without helpers */
   if (ecore_con_server_send(hd->client->net->conn, eina_strbuf_string_get(hd->send), eina_strbuf_length_get(hd->send)))
     {
        INFO("Send [1/1] complete! %zu bytes queued for sending.", eina_strbuf_length_get(hd->send));
        eina_strbuf_free(hd->send);
        hd->send = NULL;
        return;
     }

   ERR("Could not queue data for sending to redirect URI!");
   eina_strbuf_free(msg);
   azy_events_connection_kill(hd->client->net->conn, EINA_FALSE, NULL);
}

Eina_Bool
_azy_client_handler_data(Azy_Client_Handler_Data *hd,
                         int type,
                         Ecore_Con_Event_Server_Data *ev)
{
   int offset = 0;
   void *data = (ev) ? ev->data : NULL;
   int len = (ev) ? ev->size : 0;
   static unsigned int recursive;
   Azy_Client *client;

   if (!AZY_MAGIC_CHECK(hd, AZY_MAGIC_CLIENT_DATA_HANDLER))
     return ECORE_CALLBACK_RENEW;

   EINA_SAFETY_ON_NULL_RETURN_VAL(hd->client, ECORE_CALLBACK_RENEW);

   if (!hd->client->net)
     {
        INFO("Removing probably dead client %p", hd->client);
        _azy_client_handler_data_free(hd);
        return ECORE_CALLBACK_RENEW;
     }
   if (ev && (hd->client != ecore_con_server_data_get(ev->server)))
     {
        DBG("Ignoring callback due to pointer mismatch");
        return ECORE_CALLBACK_PASS_ON;
     }
   DBG("(hd=%p, method='%s', ev=%p, data=%p)", hd, (hd) ? hd->method : NULL, ev, (ev) ? ev->data : NULL);
   DBG("(client=%p, server->client=%p)", hd->client, (ev) ? ecore_con_server_data_get(ev->server) : NULL);

   client = hd->client;
   if (!hd->recv)
     {
        hd->recv = azy_net_new(client->net->conn);
        hd->recv->http.req.host = eina_stringshare_ref(client->net->http.req.host);
        hd->recv->http.req.http_path = eina_stringshare_ref(hd->uri);
     }

   EINA_SAFETY_ON_NULL_RETURN_VAL(hd->recv, ECORE_CALLBACK_RENEW);
   hd->recv->nodata = EINA_FALSE;

   if (!hd->recv->progress)
     {
        hd->recv->buffer = client->overflow;
        hd->recv->progress = EBUFLEN(client->overflow);
        client->overflow = NULL;
        if (hd->recv->buffer)
          INFO("%s: Set recv size to %" PRIi64 " from overflow", hd->method, hd->recv->progress);

        /* returns offset where http header line ends */
        if (!(offset = azy_events_type_parse(hd->recv, type, data, len)) && ev && (!hd->recv->http.res.http_msg))
          return azy_events_connection_kill(client->net->conn, EINA_FALSE, NULL);
        else if ((!offset) && client->overflow)
          {
             hd->recv->buffer = NULL;
             hd->recv->progress = 0;
             INFO("%s: Overflow could not be parsed, set recv size to 0, storing overflow of %" PRIi64 "",
                  hd->method, EBUFLEN(client->overflow));
             return ECORE_CALLBACK_RENEW;
          }
        else if (client->overflow)
          {
             client->overflow = NULL;
             INFO("%s: Overflow was parsed! Removing...", hd->method);
          }
     }
   if (!hd->recv->headers_read) /* if headers aren't done being read, keep reading them */
     {
        if (!azy_events_header_parse(hd->recv, data, len, offset) && ev)
          return azy_events_connection_kill(client->net->conn, EINA_FALSE, NULL);
     }
   else if (data) /* otherwise keep appending to buffer */
     {
        if (azy_events_length_overflows(hd->recv->progress + len, hd->recv->http.content_length))
          client->overflow = azy_events_overflow_add(hd->recv, data, len);
        else if (hd->recv->http.transfer_encoding)
          azy_events_transfer_decode(hd->recv, data, len);
        else
          {
             azy_events_recv_progress(hd->recv, data, len);
             INFO("%s: Incremented recv size to %" PRIi64 " (+%i)", hd->method, hd->recv->progress, len);
          }
     }
   else if (azy_events_length_overflows(hd->recv->progress, hd->recv->http.content_length))
     client->overflow = azy_events_overflow_add(hd->recv, NULL, 0);

   if ((hd->recv->http.res.http_code >= 301) && (hd->recv->http.res.http_code <= 303)) /* ughhhh redirect */
     {
        _azy_client_handler_redirect(hd);
        return ECORE_CALLBACK_RENEW;
     }

   if (hd->recv->overflow)
     {
        client->overflow = hd->recv->overflow;
        hd->recv->overflow = NULL;
     }

   if (!hd->recv->headers_read)
     return ECORE_CALLBACK_RENEW;

   azy_events_client_transfer_progress_event(hd, ev ? (size_t)ev->size : EBUFLEN(hd->recv->buffer));
   if ((hd->recv->http.content_length > 0) && (hd->recv->progress < (size_t)hd->recv->http.content_length))
     {
        if (!hd->recv->timer)
          /* no timer and full content length not received, start timer */
          hd->recv->timer = ecore_timer_add(30, (Ecore_Task_Cb)_azy_client_recv_timer, hd->recv);
        else
          /* timer and full content length not received, reset timer */
          ecore_timer_reset(hd->recv->timer);
     }
   else if (hd->recv->progress && (hd->recv->http.content_length > 0))
     {
        /* else create a "done" event */
        if (hd->recv->timer)
          {
             ecore_timer_del(hd->recv->timer);
             hd->recv->timer = NULL;
          }
        _azy_client_handler_call(hd); // hd is freed here, ensure use of client ptr after
     }

   if (client && client->overflow && (hd != eina_list_data_get(client->conns)))
     {
        Azy_Client_Handler_Data *dh;
        const char *method;
        int64_t prev_len;
        unsigned int id;

        dh = client->conns->data;
        if (dh->recv)
          return ECORE_CALLBACK_RENEW;

        id = dh->id;
        /* ref here in case recursive calls free dh to avoid segv */
        method = eina_stringshare_ref(dh->method);
        WARN("%s:%u (%u): Calling %s recursively to try using %" PRIi64 " bytes of overflow data...", method, id, recursive, __PRETTY_FUNCTION__, EBUFLEN(client->overflow));
        recursive++;
        prev_len = EBUFLEN(client->overflow);
        _azy_client_handler_data(dh, type, NULL);
        recursive--;
        if (!client->overflow)
          WARN("%s:%u (%u): Overflow has been successfully used (%" PRIi64 " bytes)!", method, id, recursive, prev_len - EBUFLEN(client->overflow));
        else
          WARN("%s:%u (%u): Overflow could not be entirely used (%" PRIi64 " bytes gone), storing %" PRIi64 " bytes for next event.", method, id, recursive, prev_len - EBUFLEN(client->overflow), EBUFLEN(client->overflow));
        eina_stringshare_del(method);
     }

   return ECORE_CALLBACK_RENEW;
}

Eina_Bool
_azy_client_handler_del(Azy_Client *client,
                        int type                    EINA_UNUSED,
                        Ecore_Con_Event_Server_Del *ev)
{
   Azy_Client_Handler_Data *hd;

   if (ev && ((client != ecore_con_server_data_get(ev->server)) || (!ecore_con_server_data_get(ev->server))))
     return ECORE_CALLBACK_PASS_ON;

   DBG("(client=%p, net=%p)", client, client->net);
   client->connected = EINA_FALSE;

   if (client->conns)
     {
        hd = client->conns->data;

        if (hd->recv && hd->recv->buffer && ((EBUFLEN(hd->recv->buffer) == (size_t)hd->recv->http.content_length) || (hd->recv->http.content_length == -1)))
          _azy_client_handler_call(client->conns->data);
        else
          {
             Eina_List *l, *l2;

             EINA_LIST_FOREACH_SAFE(client->conns, l, l2, hd)
               {
                  if (!hd->redirect)
                    _azy_client_handler_data_free(hd);
               }
          }
     }
   if (client->net && (!client->conns))
     {
        azy_net_free(client->net);
        client->net = NULL;
     }
   else if (client->conns)
     {
        hd = client->conns->data;

        if (hd->recv)
          {
             azy_net_code_set(client->net, hd->recv->http.res.http_code);
             azy_net_free(hd->recv);
          }
        hd->recv = NULL;
     }

   //client->refcount++; don't increase refcount here, canceled out by also refcount-- from disconnecting
   ecore_event_add(AZY_EVENT_CLIENT_DISCONNECTED, client, (Ecore_End_Cb)_azy_event_handler_fake_free, azy_client_unref);
   return ECORE_CALLBACK_CANCEL;
}

Eina_Bool
_azy_client_handler_add(Azy_Client *client,
                        int type                       EINA_UNUSED,
                        Ecore_Con_Event_Server_Add *ev EINA_UNUSED)
{
   Eina_List *l;
   Azy_Client_Handler_Data *hd;

   if (client != ecore_con_server_data_get(ev->server))
     return ECORE_CALLBACK_PASS_ON;
   DBG("(client=%p, net=%p)", client, client->net);

   client->connected = EINA_TRUE;

   client->refcount += 2; //one for event, one for being connected
   ecore_event_add(AZY_EVENT_CLIENT_CONNECTED, client, (Ecore_End_Cb)_azy_event_handler_fake_free, azy_client_unref);
   EINA_LIST_FOREACH(client->conns, l, hd)
     {
        if (hd->redirect) /* saved call from redirect, send again */
          {
             EINA_SAFETY_ON_TRUE_RETURN_VAL(
               !ecore_con_server_send(client->net->conn, eina_strbuf_string_get(hd->send), eina_strbuf_length_get(hd->send)),
               ECORE_CALLBACK_CANCEL);
             //CRI("Sent>>>>>>>>>\n%*s\n>>>>>>>>", eina_strbuf_length_get(hd->send), eina_strbuf_string_get(hd->send));
             eina_strbuf_free(hd->send);
             hd->send = NULL;
          }
     }
   return ECORE_CALLBACK_CANCEL;
}

