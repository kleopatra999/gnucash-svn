#include "config.h"

#include <glib.h>

#include "gnc-xml-helper.h"

#include "sixtp-dom-generators.h"
#include "sixtp-utils.h"

#include "GNCId.h"
#include "kvp_frame.h"

xmlNodePtr
guid_to_dom_tree(const char *tag, const GUID* gid)
{
    char *guid_str;
    xmlNodePtr ret;
    
    ret = xmlNewNode(NULL, tag);

    xmlSetProp(ret, "type", "guid");

    guid_str = guid_to_string(gid);
    if (!guid_str)
    {
        printf("FAILURE: guid_to_string failed\n");
        return NULL;
    }
        
    xmlNodeAddContent(ret, guid_str);

    g_free(guid_str);
    
    return ret;
}

xmlNodePtr
commodity_ref_to_dom_tree(const char *tag, const gnc_commodity *c)
{
    xmlNodePtr ret;

    g_return_val_if_fail(c, NULL);
    
    ret = xmlNewNode(NULL, tag);

    if(!gnc_commodity_get_namespace(c) || !gnc_commodity_get_mnemonic(c))
    {
        return NULL;
    }
    
    xmlNewTextChild(ret, NULL, "cmdty:space", gnc_commodity_get_namespace(c));
    xmlNewTextChild(ret, NULL, "cmdty:id", gnc_commodity_get_mnemonic(c));

    return ret;
}

gchar *
timespec_sec_to_string(const Timespec *ts)
{
    gchar *ret;
    struct tm parsed_time;
    time_t tmp_time;
    
    ret = g_new(gchar, 512);

    tmp_time = ts->tv_sec;
    
    if(!localtime_r(&tmp_time, &parsed_time))
    {
        g_free(ret);
        return NULL;
    }
    
    if(strftime(ret, 512, TIMESPEC_TIME_FORMAT, &parsed_time) == 0)
    {
        g_free(ret);
        return NULL;
    }
    
    return ret;
}

gchar *
timespec_nsec_to_string(const Timespec *ts)
{
    gchar *ret;

    ret = g_new(gchar, 22);

    g_snprintf(ret, 22, "%ld", ts->tv_nsec);

    return ret;
}

xmlNodePtr
timespec_to_dom_tree(const char *tag, const Timespec *spec)
{
    xmlNodePtr ret;
    gchar *date_str;
    gchar *ns_str;
    
    g_return_val_if_fail(spec, NULL);
    

    date_str = timespec_sec_to_string(spec);
    ns_str = timespec_nsec_to_string(spec);

    if((!date_str && !ns_str) || !date_str)
    {
        if(ns_str)
        {
            g_free(ns_str);
        }
        return NULL;
    }
    
    ret = xmlNewNode(NULL, tag);
    
    xmlNewTextChild(ret, NULL, "ts:date", date_str);

    if(ns_str)
    {
        xmlNewTextChild(ret, NULL, "ts:ns", ns_str);
    }

    g_free(date_str);
    g_free(ns_str);
    
    return ret;
}

xmlNodePtr
gnc_numeric_to_dom_tree(const char *tag, const gnc_numeric *num)
{
    xmlNodePtr ret;
    gchar *numstr;

    g_return_val_if_fail(num, NULL);
    
    numstr = gnc_numeric_to_string(*num);
    g_return_val_if_fail(numstr, NULL);

    ret = xmlNewNode(NULL, tag);

    xmlNodeAddContent(ret, numstr);

    g_free(numstr);

    return ret;
}

gchar*
double_to_string(double value)
{
    gchar *numstr;
#ifdef USE_GUILE_FOR_DOUBLE_CONVERSION 
    numstr = gh_scm2newstr(gh_call1(gh_eval_str("number->string"),
                                    gh_double2scm(value)),
                           NULL);

#else /* don't USE_GUILE_FOR_DOUBLE_CONVERSION */
    /*
     * we're just going to use plain-old libc for the double conversion.
     * There was some question as to whether libc is accurate enough
     * in its printf function for doubles, but I don't understand
     * how it couldn't be ...
     */
    numstr = g_strdup_printf ("%24.18g", value);

#endif /* USE_GUILE_FOR_DOUBLE_CONVERSION */
    if(!numstr)
    {
        return NULL;
        
    }
    else
    {
        return g_strstrip(numstr);
    }
}

static void
add_text_to_node(xmlNodePtr node, gchar *type, gchar *val)
{
    xmlSetProp(node, "type", type);
    xmlNodeSetContent(node, val);
    g_free(val);
}



static void
add_kvp_slot(gpointer key, gpointer value, gpointer data);

static void
add_kvp_value_node(xmlNodePtr node, gchar *tag, kvp_value* val)
{
    xmlNodePtr val_node;
    gchar *tmp_str1;
    
    val_node = xmlNewChild(node, NULL, tag, NULL);
    
    switch(kvp_value_get_type(val))
    {
    case KVP_TYPE_GINT64:
        add_text_to_node(val_node, "integer",
                         g_strdup_printf("%lld", kvp_value_get_gint64(val)));
        break;
    case KVP_TYPE_DOUBLE:
        add_text_to_node(val_node,"double",
                         double_to_string(kvp_value_get_double(val)));
        break;
    case KVP_TYPE_NUMERIC:
        add_text_to_node(val_node,"numeric",
                         gnc_numeric_to_string(kvp_value_get_numeric(val)));
        break;
    case KVP_TYPE_STRING:
        xmlSetProp(val_node, "type", "string");
        xmlNodeSetContent(val_node, kvp_value_get_string(val));
        break;
    case KVP_TYPE_GUID:
        add_text_to_node(val_node,"guid",
                         guid_to_string(kvp_value_get_guid(val)));
        break;
    case KVP_TYPE_BINARY:
    {
        guint64 size;
        void *binary_data = kvp_value_get_binary(val, &size);
        xmlSetProp(val_node, "type", "binary");
        g_return_if_fail(binary_data);
        tmp_str1 = binary_to_string(binary_data, size);
        xmlNodeSetContent(val_node, tmp_str1);
        g_free(tmp_str1);
    }
    break;
    case KVP_TYPE_GLIST:
    {
        GList *cursor;
        
        xmlSetProp(val_node, "type", "list");
        for(cursor = kvp_value_get_glist(val); cursor; cursor = cursor->next)
        {
            kvp_value *val = (kvp_value*)cursor->data;
            add_kvp_value_node(val_node, "slot:value", val);
        }
    }
    
    break;
    case KVP_TYPE_FRAME:
        xmlSetProp(val_node, "type", "frame");
        g_hash_table_foreach(kvp_frame_get_hash(kvp_value_get_frame(val)),
                             add_kvp_slot, val_node);
        break;
    default:
        break;
    }
}

static void
add_kvp_slot(gpointer key, gpointer value, gpointer data)
{
    xmlNodePtr slot_node;
    xmlNodePtr node = (xmlNodePtr)data;

    slot_node = xmlNewChild(node, NULL, "slot", NULL);

    xmlNewTextChild(slot_node, NULL, "slot:key", (gchar*)key);

    add_kvp_value_node(slot_node, "slot:value", (kvp_value*)value);
}
    
xmlNodePtr
kvp_frame_to_dom_tree(const char *tag, const kvp_frame *frame)
{
    xmlNodePtr ret;

    ret = xmlNewNode(NULL, tag);

    g_hash_table_foreach(kvp_frame_get_hash(frame), add_kvp_slot, ret);
    
    return ret;
}

