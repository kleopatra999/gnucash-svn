/*********************************************************************
 * gncmod-registercore.c
 * module definition/initialization for core (gui-independent) register
 * 
 * Copyright (c) 2001 Linux Developers Group, Inc. 
 *********************************************************************/

#include <glib.h>
#include <guile/gh.h>

#include "gnc-module.h"
#include "gnc-module-api.h"

/* version of the gnc module system interface we require */
int libgncmod_register_core_LTX_gnc_module_system_interface = 0;

/* module versioning uses libtool semantics. */
int libgncmod_register_core_LTX_gnc_module_current  = 0;
int libgncmod_register_core_LTX_gnc_module_revision = 0;
int libgncmod_register_core_LTX_gnc_module_age      = 0;

char *
libgncmod_register_core_LTX_gnc_module_path(void) {
  return g_strdup("gnucash/register/register-core");
}

char * 
libgncmod_register_core_LTX_gnc_module_description(void) {
  return g_strdup("Toolkit-independent GUI for ledger-like table displays");
}

static void
lmod(char * mn) 
{
  char * form = g_strdup_printf("(use-modules %s)\n", mn);
  gh_eval_str(form);
  g_free(form);
}

int
libgncmod_register_core_LTX_gnc_module_init(int refcount)
{
  if(!gnc_module_load("gnucash/engine", 0)) 
  {
    return FALSE;
  }

  /* FIXME. We need this for the wide-character functions.
   * When fixing, get rid of gnome-utils includes, too. */
  if(!gnc_module_load("gnucash/gnome-utils", 0)) 
  {
    return FALSE;
  }

  lmod("(g-wrapped gw-register-core)");

  return TRUE;
}
