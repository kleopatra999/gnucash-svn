/********************************************************************\
 * Account.c -- the Account data structure                          *
 * Copyright (C) 1997 Robin D. Clark                                *
 * Copyright (C) 1997, 1998, 1999, 2000 Linas Vepstas               *
 *                                                                  *
 * This program is free software; you can redistribute it and/or    *
 * modify it under the terms of the GNU General Public License as   *
 * published by the Free Software Foundation; either version 2 of   *
 * the License, or (at your option) any later version.              *
 *                                                                  *
 * This program is distributed in the hope that it will be useful,  *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of   *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the    *
 * GNU General Public License for more details.                     *
 *                                                                  *
 * You should have received a copy of the GNU General Public License*
 * along with this program; if not, contact:                        *
 *                                                                  *
 * Free Software Foundation           Voice:  +1-617-542-5942       *
 * 59 Temple Place - Suite 330        Fax:    +1-617-542-2652       *
 * Boston, MA  02111-1307,  USA       gnu@gnu.org                   *
 *                                                                  *
\********************************************************************/

#include <assert.h>
#include <string.h>

#include "config.h"

#define DISABLE_GETTEXT_UNDERSCORE /* required to include messages.h */
#include "messages.h"
#undef DISABLE_GETTEXT_UNDERSCORE /* do we want this? */

#include "Account.h"
#include "AccountP.h"
#include "gnc-commodity.h"
#include "kvp_frame.h"
#include "date.h"
#include "GNCIdP.h"
#include "Group.h"
#include "GroupP.h"
#include "messages.h"
#include "Transaction.h"
#include "TransactionP.h"
#include "util.h"

int next_free_unique_account_id = 0;

static short module = MOD_ENGINE; 

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

/********************************************************************\
 * Because I can't use C++ for this project, doesn't mean that I    *
 * can't pretend to!  These functions perform actions on the        *
 * account data structure, in order to encapsulate the knowledge    *
 * of the internals of the Account in one file.                     *
\********************************************************************/

/********************************************************************\
\********************************************************************/

void
xaccInitAccount (Account * acc) {

  acc->parent   = NULL;
  acc->children = NULL;

  acc->balance = gnc_numeric_zero();
  acc->cleared_balance = gnc_numeric_zero();
  acc->reconciled_balance = gnc_numeric_zero();

  acc->share_balance = gnc_numeric_zero();
  acc->share_cleared_balance = gnc_numeric_zero();
  acc->share_reconciled_balance = gnc_numeric_zero();

  acc->type  = -1;

  acc->accountName = strdup("");
  acc->accountCode = strdup("");
  acc->description = strdup("");

  acc->kvp_data    = kvp_frame_new();

  acc->currency    = NULL;
  acc->security    = NULL;
  acc->currency_scu = 100000;
  acc->security_scu = 100000;

  acc->splits      = NULL;

  acc->editlevel = 0;
  acc->balance_dirty = FALSE;
  acc->sort_dirty = FALSE;

  xaccGUIDNew(&acc->guid);
  xaccStoreEntity(acc, &acc->guid, GNC_ID_ACCOUNT);
}

/********************************************************************\
\********************************************************************/

Account *
xaccMallocAccount( void )
{
  Account *acc = (Account *)_malloc(sizeof(Account));
  xaccInitAccount (acc);
  return acc;
}

/********************************************************************\
\********************************************************************/

void
xaccFreeAccount( Account *acc )
{
  Transaction *t;
  GList *lp;

  if (NULL == acc) return;

  xaccRemoveEntity(&acc->guid);

  /* First, recursively free children */
  xaccFreeAccountGroup (acc->children);

  /* Next, clean up the splits */
  /* any split pointing at this account needs to be unmarked */
  for(lp = acc->splits; lp; lp = lp->next) {
    Split *s = (Split *) lp->data;
    s->acc = NULL;
  }

  /* destroy all of the splits. The xaccCommitEdit() call
   * will automatically clean up orphaned transactions.
   */

  /* FIXME: is this right? */
  acc->editlevel = 0;

  for(lp = acc->splits; lp; lp = lp->next) {
    Split *s = (Split *) lp->data;
    t = s->parent;
    xaccTransBeginEdit (t, 1);
    xaccSplitDestroy (s);
    xaccTransCommitEdit (t);
  }

  /* free up array of split pointers */
  g_list_free(acc->splits);
  acc->splits = NULL;

  if (acc->accountName) free (acc->accountName);
  if (acc->accountCode) free (acc->accountCode);
  if (acc->description) free (acc->description);

  /* zero out values, just in case stray 
   * pointers are pointing here. */

  acc->currency = NULL;
  acc->security = NULL;

  acc->parent   = NULL;
  acc->children = NULL;

  acc->balance  = gnc_numeric_zero();
  acc->cleared_balance = gnc_numeric_zero();
  acc->reconciled_balance = gnc_numeric_zero();

  acc->share_balance = gnc_numeric_zero();
  acc->share_cleared_balance = gnc_numeric_zero();
  acc->share_reconciled_balance = gnc_numeric_zero();

  acc->type  = -1;

  acc->accountName = NULL;
  acc->description = NULL;
  acc->currency    = NULL;
  acc->security    = NULL;

  acc->editlevel = 0;
  acc->balance_dirty = FALSE;
  acc->sort_dirty = FALSE;

  _free(acc);
}

/********************************************************************\
\********************************************************************/

gboolean
xaccAccountEqual(Account *aa, Account *ab, gboolean check_guids) {
  if(!aa && !ab) return TRUE;
  if(!aa) return FALSE;
  if(!ab) return FALSE;

  if(aa->type != ab->type) {
    fprintf(stderr, "Account types don't match (%d != %d)\n",
            aa->type, ab->type);
    return FALSE;
  }

  if(safe_strcmp(aa->accountName, ab->accountName) != 0) return FALSE;
  if(safe_strcmp(aa->accountCode, ab->accountCode) != 0) return FALSE;
  if(safe_strcmp(aa->description, ab->description) != 0) return FALSE;
  if(!gnc_commodity_equiv(aa->currency, ab->currency)) return FALSE;
  if(!gnc_commodity_equiv(aa->security, ab->security)) return FALSE;

  if(check_guids) {
    if(!guid_equal(&aa->guid, &ab->guid)) {
      fprintf(stderr, "Account guids don't match for %s ?= %s\n",
              aa->accountName, ab->accountName);
      return FALSE;
    }
  }

  if(kvp_frame_compare(aa->kvp_data, ab->kvp_data) != 0) return FALSE;

  /* no parent; always compare downwards. */

  {
    GList *la = aa->splits;
    GList *lb = ab->splits;

    if( la && !lb) return FALSE;
    if(!la &&  lb) return FALSE;
    if(la && lb) {
      /* presume that the splits are in the same order */
      while(la && lb) {
        Split *sa = (Split *) la->data;
        Split *sb = (Split *) lb->data;
        if(!xaccSplitEqual(sa, sb, check_guids, FALSE)) return(FALSE);
        la = la->next;
        lb = lb->next;
      }
      if((la != NULL) || (lb != NULL)) return(FALSE);
    }
  }

  if(!xaccGroupEqual(aa->children, ab->children, check_guids)) return FALSE;

  return(TRUE);
}

/********************************************************************\
\********************************************************************/

static gint
split_sort_func(gconstpointer a, gconstpointer b) {
  /* don't coerce xaccSplitDateOrder so we'll catch changes */
  Split *sa = (Split *) a;
  Split *sb = (Split *) b;
  return(xaccSplitDateOrder(sa, sb));
}

void
xaccAccountSortSplits (Account *acc) {
  if(!acc) return;

  if(!acc->sort_dirty) return;
  if(acc->editlevel > 0) return;
  acc->splits = g_list_sort(acc->splits, split_sort_func);
  acc->sort_dirty = FALSE;
}

static void
xaccAccountBringUpToDate(Account *acc) {
  if(!acc) return;

  /* if a re-sort happens here, then everything will update, so the
     cost basis and balance calls are no-ops */
  xaccAccountSortSplits(acc);
  xaccAccountRecomputeBalance(acc);
}

void 
xaccAccountBeginEdit (Account *acc) {
   if (!acc) return;

   /* FIXME: we should check for editlevel overflow here and do
      something about it. */

   acc->editlevel++;
}

void 
xaccAccountCommitEdit (Account *acc) {
   if (!acc) return;
   acc->editlevel--;
   if(acc->editlevel < 0) {
     fprintf(stderr,
             "ERROR: unbalanced call to xaccAccountCommitEdit - resetting.\n");
   } else if(acc->editlevel == 0) {
     xaccAccountBringUpToDate(acc);
   }
}


/********************************************************************
 * xaccAccountGetSlots
 ********************************************************************/

kvp_frame * 
xaccAccountGetSlots(Account * account) {
  return(account->kvp_data);
}

/********************************************************************\
\********************************************************************/

const GUID *
xaccAccountGetGUID (Account *account)
{
  if (!account)
    return xaccGUIDNULL();

  return &account->guid;
}

/********************************************************************\
\********************************************************************/

void 
xaccAccountSetGUID (Account *account, GUID *guid)
{
  if (!account || !guid) return;

  xaccRemoveEntity(&account->guid);

  account->guid = *guid;

  xaccStoreEntity(account, &account->guid, GNC_ID_ACCOUNT);
}

/********************************************************************\
\********************************************************************/

Account *
xaccAccountLookup (const GUID *guid)
{
  if (!guid) return NULL;
  return xaccLookupEntity(guid, GNC_ID_ACCOUNT);
}

/********************************************************************\
\********************************************************************/

short
xaccAccountGetMark (Account *acc)
{
  if (!acc) return 0;
  return acc->mark;
}

void
xaccAccountSetMark (Account *acc, short m)
{
  if (!acc) return;
  acc->mark = m;
}

void
xaccClearMark (Account *acc, short val)
{
   AccountGroup *topgrp;

   if (!acc) return;
   topgrp = xaccGetAccountRoot (acc);
   if (topgrp) {
      int i, nacc = topgrp->numAcc;
      for (i=0; i<nacc; i++) {
         xaccClearMarkDown (topgrp->account[i], val);
      }
   } else {
      xaccClearMarkDown (acc, val);
   }
}

void
xaccClearMarkDown (Account *acc, short val)
{
   AccountGroup *chillin;
   if (!acc) return;
   acc->mark = val;

   chillin = acc->children;
   if (chillin) {
      int i, nacc = chillin->numAcc;
      for (i=0; i<nacc; i++) {
         xaccClearMarkDown (chillin->account[i], val);
      }
   }
}

void
xaccClearMarkDownGr (AccountGroup *grp, short val)
{
   int i, nacc;
   if (!grp) return;
   nacc = grp->numAcc;
   for (i=0; i<nacc; i++) {
      xaccClearMarkDown (grp->account[i], val);
   }
}


/********************************************************************\
\********************************************************************/

#define CHECK(acc) {					\
   if (acc->editlevel <= 0) {				\
      /* not today, some day in the future ... */	\
      /* PERR ("Account not open for editing\n"); */	\
      /* assert (0); */					\
      /* return; */					\
   }							\
  if (NULL != acc->parent) acc->parent->saved = FALSE;	\
}

/********************************************************************\
\********************************************************************/

void
xaccAccountInsertSplit ( Account *acc, Split *split ) {

  if (!acc) return;
  if (!split) return;

  /* Make sure the currencies in the transaction will still
   * be acceptable. This means either the currency or the security
   * of the new account must be 'in common' with the currencies used
   * in the transaction. */
#if 0
  if (xaccTransCountSplits(split->parent) > 1) {
    if (!xaccTransIsCommonCurrency(split->parent, acc->currency) &&
        !xaccTransIsCommonCurrency(split->parent, acc->security))
      
      return;
  }
#endif
    
  xaccAccountBeginEdit(acc);
  {
    Account *oldacc;
    CHECK (acc);
    
    acc->balance_dirty = TRUE;
    acc->sort_dirty = TRUE;
    
    /* convert the split to the new account's denominators */
    /* if the denominator can't be exactly converted, it's an error */
    /* FIXME : need to enforce ordering of insertion/value */
    split->damount = gnc_numeric_convert(split->damount, 
                                         xaccAccountGetSecuritySCU(acc),
                                         GNC_RND_ROUND);
    
    split->value   = gnc_numeric_convert(split->value, 
                                         xaccAccountGetCurrencySCU(acc),
                                         GNC_RND_ROUND);
    
    /* if this split belongs to another account, remove it from there
     * first.  We don't want to ever leave the system in an inconsistent
     * state.  Note that it might belong to the current account if we're
     * just using this call to re-order.  */
    oldacc = split->acc;
    if (split->acc) xaccAccountRemoveSplit (split->acc, split);
    split->acc = acc;
    
    if(acc->editlevel == 1) {
      acc->splits = g_list_insert_sorted(acc->splits, split, split_sort_func);
      acc->sort_dirty = FALSE;
    } else {
      acc->splits = g_list_prepend(acc->splits, split);
    }
  }
  xaccAccountCommitEdit(acc);
}

/********************************************************************\
\********************************************************************/

void
xaccAccountRemoveSplit ( Account *acc, Split *split ) {
  if (!acc) return;
  if (!split) return;
  
  xaccAccountBeginEdit(acc);
  {
    CHECK (acc);  
    acc->balance_dirty = TRUE;
    acc->splits = g_list_remove(acc->splits, split);
    split->acc = NULL;
  }
  xaccAccountCommitEdit(acc);
}


/********************************************************************\
 * xaccAccountRecomputeBalance                                      *
 *   recomputes the partial balances and the current balance for    *
 *   this account.                                                  *
 *                                                                  *
 * The way the computation is done depends on whether the partial   *
 * balances are for a monetary account (bank, cash, etc.) or a      *
 * certificate account (stock portfolio, mutual fund).  For bank    *
 * accounts, the invariant amount is the dollar amount. For share   *
 * accounts, the invariant amount is the number of shares. For      *
 * share accounts, the share price fluctuates, and the current      *
 * value of such an account is the number of shares times the       *
 * current share price.                                             *
 *                                                                  *
 * Part of the complexity of this computatation stems from the fact *
 * xacc uses a double-entry system, meaning that one transaction    *
 * appears in two accounts: one account is debited, and the other   *
 * is credited.  When the transaction represents a sale of shares,  *
 * or a purchase of shares, some care must be taken to compute      *
 * balances correctly.  For a sale of shares, the stock account must*
 * be debited in shares, but the bank account must be credited      *
 * in dollars.  Thus, two different mechanisms must be used to      *
 * compute balances, depending on account type.                     *
 *                                                                  *
 * Args:   account -- the account for which to recompute balances   *
 * Return: void                                                     *
\********************************************************************/

static gnc_numeric
price_xfer(Split * s, gnc_numeric share_count) {
  gnc_numeric temp;
  if(!gnc_numeric_zero_p(s->damount)) {
    temp = gnc_numeric_div(s->value, s->damount,
                           GNC_DENOM_AUTO, GNC_DENOM_EXACT);
    temp = gnc_numeric_mul(share_count, temp,
                           gnc_numeric_denom(s->value),
                           GNC_RND_ROUND);
    return temp;
  }
  else {
    return gnc_numeric_zero();
  }
}      
                              
void
xaccAccountRecomputeBalance( Account * acc ) {
  gnc_numeric  dbalance;
  gnc_numeric  dcleared_balance; 
  gnc_numeric  dreconciled_balance;
  gnc_numeric  share_balance; 
  gnc_numeric  share_cleared_balance; 
  gnc_numeric  share_reconciled_balance;
  Split   *last_split = NULL;
  GList   *lp;

  if(NULL == acc) return;
  if(acc->editlevel > 0) return;
  if(!acc->balance_dirty) return;

  dbalance = gnc_numeric_zero();
  dcleared_balance = gnc_numeric_zero();
  dreconciled_balance = gnc_numeric_zero();
  share_balance    = gnc_numeric_zero();
  share_cleared_balance = gnc_numeric_zero();
  share_reconciled_balance = gnc_numeric_zero();

  for(lp = acc->splits; lp; lp = lp->next) {
    Split *split = (Split *) lp->data;

    /* compute both dollar and share balances */
    share_balance = gnc_numeric_add_fixed(share_balance, split->damount);
    dbalance      = gnc_numeric_add_fixed(dbalance, split->value);
    
    if( NREC != split -> reconciled ) {
      share_cleared_balance = 
        gnc_numeric_add_fixed(share_cleared_balance, split->damount);
      dcleared_balance = 
        gnc_numeric_add_fixed(dcleared_balance, split->value);
    }

    if( YREC == split -> reconciled ) {
      share_reconciled_balance = 
        gnc_numeric_add_fixed(share_cleared_balance, split->damount);
      dreconciled_balance =  
        gnc_numeric_add_fixed(dreconciled_balance, split->value);
    }

    /* For bank accounts, the invariant subtotal is the dollar
     * amount.  For stock accounts, the invariant is the share amount */
    if ( (STOCK    == acc->type) ||
         (MUTUAL   == acc->type) ||
         (CURRENCY == acc->type) ) {
      split -> share_balance = share_balance;
      split -> share_cleared_balance = share_cleared_balance;
      split -> share_reconciled_balance = share_reconciled_balance;
      split -> balance = price_xfer(split, share_balance);
      split -> cleared_balance = price_xfer(split, share_cleared_balance);
      split -> reconciled_balance = 
        price_xfer(split, share_reconciled_balance);
    } 
    else {
      split -> share_balance = dbalance;
      split -> share_cleared_balance = dcleared_balance;
      split -> share_reconciled_balance = dreconciled_balance;
      split -> balance = dbalance;
      split -> cleared_balance = dcleared_balance;
      split -> reconciled_balance = dreconciled_balance;
    }
    
    last_split = split;
  }

  if ( (STOCK == acc->type) || ( MUTUAL == acc->type) ) {
    if (last_split) {
      acc -> share_balance = share_balance;
      acc -> share_cleared_balance = share_cleared_balance;
      acc -> share_reconciled_balance = share_reconciled_balance;
      acc -> balance = price_xfer(last_split, share_balance);
      acc -> cleared_balance = price_xfer(last_split, share_cleared_balance);
      acc -> reconciled_balance = 
        price_xfer(last_split, share_reconciled_balance);
      
    } 
    else {
      acc -> share_balance = gnc_numeric_zero();
      acc -> share_cleared_balance = gnc_numeric_zero();
      acc -> share_reconciled_balance = gnc_numeric_zero();
      acc -> balance = gnc_numeric_zero();
      acc -> cleared_balance = gnc_numeric_zero();
      acc -> reconciled_balance = gnc_numeric_zero();
    }
  } else {
    acc -> share_balance = dbalance;
    acc -> share_cleared_balance = dcleared_balance;
    acc -> share_reconciled_balance = dreconciled_balance;
    acc -> balance = dbalance;
    acc -> cleared_balance = dcleared_balance;
    acc -> reconciled_balance = dreconciled_balance;
  }

  acc->balance_dirty = FALSE;
  return;
}


/********************************************************************\
 * xaccAccountFixSplitDateOrder                                     *
 *   check this split to see if the date is in correct order        *
 *   If it is not, reorder the transactions ...                     *
 *                                                                  *
 * Args:   acc   -- the account to check                            *
 *         split -- the split to check                              *
 *                                                                  *
 * Return: int -- non-zero if out of order                          *
\********************************************************************/

void
xaccAccountFixSplitDateOrder (Account * acc, Split *split ) {
  if (NULL == acc) return;
  if (NULL == split) return;

  xaccAccountBeginEdit(acc);
  {
    acc->sort_dirty = TRUE;
    acc->balance_dirty = TRUE;
  }
  xaccAccountCommitEdit(acc);
}

/********************************************************************\
 * xaccCheckTransDateOrder                                          *
 *   check this transaction to see if the date is in correct order  *
 *   If it is not, reorder the transactions ...                     *
 *   This routine perfroms the check for both of the double-entry   *
 *   transaction entries ...                                        *
 *                                                                  *
 * Args:   trans -- the transaction to check                        *
 * Return: int -- non-zero if out of order                          *
\********************************************************************/

void
xaccTransFixSplitDateOrder (Transaction *trans )
{
  Account * acc;
  Split *s;
  int i = 0;

  if (NULL == trans) return;

  i=0;
  s = trans->splits[0];
  while (s) {
    acc = (Account *) (s->acc);
    xaccAccountFixSplitDateOrder (acc, s);
    i++;
    s = trans->splits[i];
  }
}

/********************************************************************\
\********************************************************************/

/* The sort order is used to implicitly define an 
 * order for report generation */

static int typeorder[NUM_ACCOUNT_TYPES] = {
     BANK, STOCK, MUTUAL, CURRENCY, CASH, ASSET, 
     CREDIT, LIABILITY, INCOME, EXPENSE, EQUITY };

static int revorder[NUM_ACCOUNT_TYPES] = {
     -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };


int
xaccAccountOrder (Account **aa, Account **ab) {
  char *da, *db;
  char *endptr = NULL;
  int ta, tb;
  long la, lb;

  if ( (*aa) && !(*ab) ) return -1;
  if ( !(*aa) && (*ab) ) return +1;
  if ( !(*aa) && !(*ab) ) return 0;

  /* sort on accountCode strings */
  da = (*aa)->accountCode;
  db = (*ab)->accountCode;

  /* If accountCodes are both base 36 integers do an integer sort */
  la = strtoul (da, &endptr, 36);
  if((*da != '\0') && (*endptr == '\0')) {
    lb = strtoul (db, &endptr, 36);
    if((*db != '\0') && (*endptr == '\0')) {
      if (la < lb) return -1;
      if (la > lb) return +1;
    }
  }

  /* Otherwise do a string sort */
  SAFE_STRCMP (da, db);

  /* if acccount-type-order array not initialized, initialize it */
  /* this will happen at most once during program invocation */
  if (-1 == revorder[0]) {
    int i;
    for (i=0; i<NUM_ACCOUNT_TYPES; i++) {
      revorder [typeorder[i]] = i;
    }
  }

  /* otherwise, sort on account type */
  ta = (*aa)->type;
  tb = (*ab)->type;
  ta = revorder[ta];
  tb = revorder[tb];
  if (ta < tb) return -1;
  if (ta > tb) return +1;

  /* otherwise, sort on accountName strings */
  da = (*aa)->accountName;
  db = (*ab)->accountName;
  SAFE_STRCMP (da, db);

  return 0;
}

/********************************************************************\
\********************************************************************/
/* account codes will be assigned base-36, with three digits */

#define BASE 36

void 
xaccAccountAutoCode (Account *acc, int digits) {
  if (!acc) return;
  if (acc->accountCode) return;   /* no-op if code already assinged */
  if (!(acc->parent)) return; 

  xaccAccountBeginEdit(acc);
  {
    acc->accountCode = xaccGroupGetNextFreeCode (acc->parent, digits);
    acc->parent->saved = FALSE;
  }
  xaccAccountCommitEdit(acc);
}

/********************************************************************\
\********************************************************************/

int
xaccIsAccountInList (Account * acc, Account **list)
{
   Account * chk;
   int nacc = 0;
   int nappearances = 0;
   if (!acc) return 0;
   if (!list) return 0;

   chk = list[0];
   while (chk) {
      if (acc == chk) nappearances ++;
      nacc++;
      chk = list[nacc];
   }
   return nappearances;
}

/********************************************************************\
\********************************************************************/

void
xaccAccountRecomputeBalances( Account **list )
{
   Account * acc;
   int nacc = 0;
   if (!list) return;

   acc = list[0];
   while (acc) {
      xaccAccountRecomputeBalance (acc);
      nacc++;
      acc = list[nacc];
   }
}

/********************************************************************\
\********************************************************************/

void
xaccMoveFarEnd (Split *split, Account *new_acc)
{
   Split *partner_split = 0x0;
   Transaction *trans;
   Account * acc;
   int numsplits = 0;

   if (!split) return;
   
   /* if the transaction has two splits, then the "far end" 
    * is the other one. Otherwise, far end is undefined. 
    * If the new destination does not match the current dest,
    * then move the far end of the split to the new location.
    */
   trans = (Transaction *) (split->parent);
   assert (trans);
   assert (trans->splits);

   numsplits = xaccCountSplits (trans->splits);
   if (2 < numsplits) return;

   if (split == trans->splits[0]) {
      partner_split = trans->splits [1];
   } else

   if (split == trans->splits[1]) {
      partner_split = trans->splits [0];
   } else 

   if (new_acc) {
      /* Gosh, the far end doesn't exist! create it! */
      partner_split = xaccMallocSplit ();
      xaccTransAppendSplit (trans, partner_split);
      xaccAccountInsertSplit (new_acc, partner_split);
      return;
   } else {
      /* no partner split, AND no far-end accouont. return */
      return;
   }

   /* move the partner split from the old account to the new */ 
   acc = (Account *) (partner_split->acc);
   if (acc != new_acc) {
      xaccAccountInsertSplit (new_acc, partner_split);
   }
}

/********************************************************************\
\********************************************************************/

void
xaccMoveFarEndByName (Split *split, const char *new_acc_name)
{
   Account *acc;

   if (!split) return;
   if (0 == strcmp (SPLIT_STR, new_acc_name)) return;

   acc = (Account *) split->acc;
   acc = xaccGetPeerAccountFromName (acc, new_acc_name);
   xaccMoveFarEnd (split, acc);
}

/********************************************************************\
\********************************************************************/

void 
xaccAccountSetType (Account *acc, int tip) {

  if (!acc) return;

  xaccAccountBeginEdit(acc);
  {
    CHECK (acc);
    
    /* refuse invalid account types, and don't bother if not new type. */
    if((NUM_ACCOUNT_TYPES > tip) && (acc->type != tip)) {
      acc->type = tip;
      acc->balance_dirty = TRUE; /* new type may affect balance computation */
    }
  }
  xaccAccountCommitEdit(acc);
}

void 
xaccAccountSetName (Account *acc, const char *str) {
   char * tmp;

   if ((!acc) || (!str)) return;

   xaccAccountBeginEdit(acc);
   {
     CHECK (acc);
     
     /* make strdup before freeing */
     tmp = strdup (str);
     if (acc->accountName) free (acc->accountName);
     acc->accountName = tmp;
   }
   xaccAccountCommitEdit(acc);
}

void 
xaccAccountSetCode (Account *acc, const char *str) {
   char * tmp;
   if ((!acc) || (!str)) return;

   xaccAccountBeginEdit(acc);
   {
     CHECK (acc);
     
     /* make strdup before freeing */
     tmp = strdup (str);
     if (acc->accountCode) free (acc->accountCode);
     acc->accountCode = tmp;
   }
   xaccAccountCommitEdit(acc);
}

void 
xaccAccountSetDescription (Account *acc, const char *str) {
   char * tmp;
   if ((!acc) || (!str)) return;

   xaccAccountBeginEdit(acc);
   {
     CHECK (acc);
     
     /* make strdup before freeing */
     tmp = strdup (str);
     if (acc->description) free (acc->description);
     acc->description = tmp;
   }
   xaccAccountCommitEdit(acc);
}

void 
xaccAccountSetNotes (Account *acc, const char *str) {
  kvp_value *new_value;
  
  if ((!acc) || (!str)) return;

  xaccAccountBeginEdit(acc);
  {
    CHECK (acc);
    
    new_value = kvp_value_new_string(str);
    if(new_value) {
      kvp_frame_set_slot(xaccAccountGetSlots(acc), "notes", new_value);
      kvp_value_delete(new_value);
    } else {
      fprintf(stderr, "xaccAccountSetNotes: failed to allocate kvp.\n");
    }
  }
  xaccAccountCommitEdit(acc);
}

/* FIXME : is this the right way to do this? */
static void
update_split_currency(Account * acc) {
  GList *lp;
  
  if(!acc) return;
  
  /* iterate over splits */
  for(lp = acc->splits; lp; lp = lp->next) {
    Split *s = (Split *) lp->data;
    s->value  = gnc_numeric_convert(s->value, acc->currency_scu, 
				    GNC_RND_ROUND);
    s->damount = gnc_numeric_convert(s->damount, acc->security_scu, 
				     GNC_RND_ROUND);    
  }
}

void 
xaccAccountSetCurrency (Account * acc, const gnc_commodity * currency) {

  if ((!acc) || (!currency)) return;
  
  xaccAccountBeginEdit(acc);
  {
    CHECK (acc);
    
    acc->currency     = currency;
    acc->currency_scu = gnc_commodity_get_fraction(currency);
    update_split_currency(acc);

    acc->sort_dirty = TRUE;
    acc->balance_dirty = TRUE;
  }
  xaccAccountCommitEdit(acc);
}

void 
xaccAccountSetSecurity (Account *acc, const gnc_commodity * security) {
  
  if ((!acc) || (!security)) return;
  
  xaccAccountBeginEdit(acc);
  {
    CHECK (acc);
    
    acc->security     = security;
    acc->security_scu = gnc_commodity_get_fraction(security);    
    update_split_currency(acc);
    
    acc->sort_dirty = TRUE;
    acc->balance_dirty = TRUE;
  }
  xaccAccountCommitEdit(acc);
}

void 
xaccAccountSetCurrencySCU (Account * acc, int scu) {

  if (!acc) return;

  xaccAccountBeginEdit(acc);
  {
    CHECK (acc);
    acc->currency_scu = scu;
  }
  xaccAccountCommitEdit(acc);
}

int
xaccAccountGetCurrencySCU (Account * acc) {
  if (!acc) return 0;
  CHECK (acc);
  return acc->currency_scu;
}

int
xaccAccountGetSecuritySCU (Account * acc) {
  if (!acc) return 0;
  CHECK (acc);
  return acc->security_scu;
}


/********************************************************************\
\********************************************************************/

AccountGroup *
xaccAccountGetChildren (Account *acc)
{
   if (!acc) return NULL;
   return (acc->children);
}

AccountGroup *
xaccAccountGetParent (Account *acc)
{
   if (!acc) return NULL;
   return (acc->parent);
}

Account *
xaccAccountGetParentAccount (Account * acc)
{
  if (!acc) return NULL;
  return xaccGroupGetParentAccount(acc->parent);
}

GNCAccountType
xaccAccountGetType (Account *acc)
{
   if (!acc) return NO_TYPE;
   return (acc->type);
}

const char *
xaccAccountGetName (Account *acc)
{
   if (!acc) return NULL;
   return (acc->accountName);
}

char *
xaccAccountGetFullName(Account *account, const char separator)
{
  Account *a;
  char *fullname;
  const char *name;
  char *p;
  int length;

  if (account == NULL)
  {
    fullname = strdup("");
    assert(fullname != NULL);
    return fullname;
  }

  /* Figure out how much space is needed */
  length = 0;
  a = account;
  while (a != NULL)
  {
    name = xaccAccountGetName(a);

    length += strlen(name) + 1; /* plus one for the separator */

    a = xaccAccountGetParentAccount(a);
  }

  /* length has one extra separator in it, that's ok, because it will
   * hold the null character at the end. */

  /* allocate the memory */
  fullname = malloc(length * sizeof(char));
  assert(fullname != 0);

  /* go to end of string */
  p = fullname + length - 1;

  /* put in the null character and move to the previous char */
  *p-- = 0;

  a = account;
  while (a != NULL)
  {
    name = xaccAccountGetName(a);
    length = strlen(name);

    /* copy the characters going backwards */
    while (length > 0)
      *p-- = name[--length];

    a = xaccAccountGetParentAccount(a);

    /* if we're not at the root, add another separator */
    if (a != NULL)
      *p-- = separator;
  }

  return fullname;
}

const char *
xaccAccountGetCode (Account *acc)
{
   if (!acc) return NULL;
   return (acc->accountCode);
}

const char * 
xaccAccountGetDescription (Account *acc)
{
   if (!acc) return NULL;
   return (acc->description);
}

const char * 
xaccAccountGetNotes (Account *acc) {
  kvp_value *v;

  if (!acc) return NULL;
  v = kvp_frame_get_slot(xaccAccountGetSlots(acc), "notes");
  if(v) return(kvp_value_get_string(v));
   return(NULL);
}

const gnc_commodity * 
xaccAccountGetCurrency (Account *acc)
{
   if (!acc) return NULL;
   return (acc->currency);
}

const gnc_commodity * 
xaccAccountGetSecurity (Account *acc)
{
   if (!acc) return NULL;
   return (acc->security);
}

double
DxaccAccountGetBalance (Account *acc)
{
  return gnc_numeric_to_double(xaccAccountGetBalance(acc));
}

double
DxaccAccountGetClearedBalance (Account *acc)
{
  return gnc_numeric_to_double(xaccAccountGetClearedBalance(acc));
}

double
DxaccAccountGetReconciledBalance (Account *acc)
{
  return gnc_numeric_to_double(xaccAccountGetReconciledBalance(acc));
}

double
DxaccAccountGetShareBalance (Account *acc)
{
  return gnc_numeric_to_double(xaccAccountGetShareBalance(acc));
}

double
DxaccAccountGetShareClearedBalance (Account *acc)
{
  return gnc_numeric_to_double(xaccAccountGetShareClearedBalance(acc));
}

double
DxaccAccountGetShareReconciledBalance (Account *acc)
{
  return gnc_numeric_to_double(xaccAccountGetShareReconciledBalance(acc));
}

gnc_numeric
xaccAccountGetBalance (Account *acc) {
  if (!acc) return gnc_numeric_zero();
  return acc->balance;
}

gnc_numeric
xaccAccountGetClearedBalance (Account *acc)
{
   if (!acc) return gnc_numeric_zero();
   return acc->cleared_balance;
}

gnc_numeric
xaccAccountGetReconciledBalance (Account *acc)
{
   if (!acc) return gnc_numeric_zero();
   return acc->reconciled_balance;
}

gnc_numeric
xaccAccountGetShareBalance (Account *acc)
{
   if (!acc) return gnc_numeric_zero();
   return acc->share_balance;
}

gnc_numeric
xaccAccountGetShareClearedBalance (Account *acc)
{
   if (!acc) return gnc_numeric_zero();
   return acc->share_cleared_balance;
}

gnc_numeric
xaccAccountGetShareReconciledBalance (Account *acc)
{
  if (!acc) return gnc_numeric_zero();
  return acc->share_reconciled_balance;
}

Split *
xaccAccountGetSplit(Account *acc, int i) {
  GList *result;
  fprintf(stderr, "Calling xaccAccountGetSplit: welcome to pokeyland.\n");

  if (!acc) return(NULL);
  result = g_list_nth(acc->splits, i);
  if(!result) return(NULL);
  return((Split *) result->data);
}

GList *
xaccAccountGetSplitList (Account *acc) {
  if (!acc) return NULL;
  return (acc->splits);
}

int
xaccAccountGetNumSplits (Account *acc) {
  fprintf(stderr, "Calling xaccAccountGetNumSplits: welcome to pokeyland.\n");
  if (!acc) return 0;
  return g_list_length(acc->splits);
}

/********************************************************************\
\********************************************************************/

Account * 
IthAccount (Account **list, int i)
{
   if (!list || 0 > i) return NULL;
   return list[i];
}

/********************************************************************\
\********************************************************************/

gboolean
xaccAccountsHaveCommonCurrency(Account *account_1, Account *account_2)
{
  if ((account_1 == NULL) || (account_2 == NULL))
    return FALSE;

  return xaccIsCommonCurrency(account_1->currency, account_1->security,
			      account_2->currency, account_2->security);
}

/********************************************************************\
\********************************************************************/

gboolean
xaccAccountHasAncestor (Account *account, Account * ancestor)
{
  Account *parent;

  if ((account == NULL) || (ancestor == NULL))
    return FALSE;

  parent = xaccAccountGetParentAccount(account);
  while (parent != NULL)
  {
    if (parent == ancestor)
      return TRUE;

    parent = xaccAccountGetParentAccount(parent);
  }

  return FALSE;
}

/********************************************************************\
\********************************************************************/

/* You must edit the functions in this block in tandem.  KEEP THEM IN
   SYNC! */

#define GNC_RETURN_ENUM_AS_STRING(x) case (x): return #x;

char *
xaccAccountTypeEnumAsString(int type) {
  switch(type) {
    GNC_RETURN_ENUM_AS_STRING(NO_TYPE);
    GNC_RETURN_ENUM_AS_STRING(BANK);
    GNC_RETURN_ENUM_AS_STRING(CASH);
    GNC_RETURN_ENUM_AS_STRING(CREDIT);
    GNC_RETURN_ENUM_AS_STRING(ASSET);
    GNC_RETURN_ENUM_AS_STRING(LIABILITY);
    GNC_RETURN_ENUM_AS_STRING(STOCK);
    GNC_RETURN_ENUM_AS_STRING(MUTUAL);
    GNC_RETURN_ENUM_AS_STRING(CURRENCY);
    GNC_RETURN_ENUM_AS_STRING(INCOME);
    GNC_RETURN_ENUM_AS_STRING(EXPENSE);
    GNC_RETURN_ENUM_AS_STRING(EQUITY);
    GNC_RETURN_ENUM_AS_STRING(CHECKING);
    GNC_RETURN_ENUM_AS_STRING(SAVINGS);
    GNC_RETURN_ENUM_AS_STRING(MONEYMRKT);
    GNC_RETURN_ENUM_AS_STRING(CREDITLINE);
    default:
      PERR ("asked to translate unknown account type %d.\n", type);
      break;
  }
  return(NULL);
}

#undef GNC_RETURN_ENUM_AS_STRING

#define GNC_RETURN_ON_MATCH(x) \
  if(safe_strcmp(#x, (str)) == 0) { *type = x; return(TRUE); }

gboolean
xaccAccountStringToType(const char* str, int *type) {

  GNC_RETURN_ON_MATCH(NO_TYPE);
  GNC_RETURN_ON_MATCH(BANK);
  GNC_RETURN_ON_MATCH(CASH);
  GNC_RETURN_ON_MATCH(CREDIT);
  GNC_RETURN_ON_MATCH(ASSET);
  GNC_RETURN_ON_MATCH(LIABILITY);
  GNC_RETURN_ON_MATCH(STOCK);
  GNC_RETURN_ON_MATCH(MUTUAL);
  GNC_RETURN_ON_MATCH(CURRENCY);
  GNC_RETURN_ON_MATCH(INCOME);
  GNC_RETURN_ON_MATCH(EXPENSE);
  GNC_RETURN_ON_MATCH(EQUITY);
  GNC_RETURN_ON_MATCH(CHECKING);
  GNC_RETURN_ON_MATCH(SAVINGS);
  GNC_RETURN_ON_MATCH(MONEYMRKT);
  GNC_RETURN_ON_MATCH(CREDITLINE);

  PERR("asked to translate unknown account type string %s.\n", str);
  return(FALSE);
}

#undef GNC_RETURN_ON_MATCH

/********************************************************************\
\********************************************************************/

static char *
account_type_name[NUM_ACCOUNT_TYPES] = { 
  BANK_STR, 
  CASH_STR, 
  ASSET_STR, 
  CREDIT_CARD_STR,
  LIABILITY_STR, 
  STOCK_STR, 
  MUTUAL_FUND_STR,
  CURRENCY_STR,
  INCOME_STR, 
  EXPENSE_STR, 
  EQUITY_STR
  /*
    CHECKING_STR,
    SAVINGS_STR,
    MONEYMRKT_STR,
    CREDITLINE_STR
  */
};

char *
xaccAccountGetTypeStr(int type) {
  if (0 > type) return "";
  if (NUM_ACCOUNT_TYPES <= type) return "";
  return gettext (account_type_name [type]);
}

/********************************************************************\
\********************************************************************/

gboolean
xaccAccountTypesCompatible (int parent_type, int child_type)
{
  gboolean compatible = FALSE;

  switch(parent_type)
  {
    case BANK:
    case CASH: 
    case ASSET:
    case STOCK:
    case MUTUAL:
    case CURRENCY:
    case CREDIT:
    case LIABILITY:
      compatible = ((child_type == BANK)     ||
		    (child_type == CASH)     ||
		    (child_type == ASSET)    ||
		    (child_type == STOCK)    ||
		    (child_type == MUTUAL)   ||
		    (child_type == CURRENCY) ||
                    (child_type == CREDIT)   ||
                    (child_type == LIABILITY));
      break;
    case INCOME:
    case EXPENSE:
      compatible = ((child_type == INCOME) ||
                    (child_type == EXPENSE));
      break;
    case EQUITY:
      compatible = (child_type == EQUITY);
      break;
    default:
      PERR("bad account type: %d", parent_type);
      break;
  }

  return compatible;
}

/********************************************************************\
\********************************************************************/

void
xaccAccountSetPriceSrc(Account *acc, const char *src) {

  if(!acc) return;
  if(!src) return;

  xaccAccountBeginEdit(acc);
  {
    GNCAccountType t = xaccAccountGetType(acc);

    if((t == STOCK) || (t == MUTUAL)) {
      kvp_value *new_value = kvp_value_new_string(src);
      if(new_value) {
        kvp_frame_set_slot(xaccAccountGetSlots(acc),
                           "old-price-source",
                           new_value);
        kvp_value_delete(new_value);
      } else {
        PERR ("xaccAccountSetPriceSrc: failed to allocate kvp_value.");
      }
    }
  }
  xaccAccountCommitEdit(acc);
}

/********************************************************************\
\********************************************************************/

const char*
xaccAccountGetPriceSrc(Account *acc) {
  const char *result = NULL;

  if(!acc) {
    result = NULL;
  } else {
    GNCAccountType t = xaccAccountGetType(acc);
    if((t == STOCK) || (t == MUTUAL)) {
      kvp_value *value = kvp_frame_get_slot(xaccAccountGetSlots(acc),
                                            "old-price-source");
      if(value) {
        result = kvp_value_get_string(value);
      }
    }
  }
  return(result);
}

/********************************************************************\
\********************************************************************/

gboolean
xaccAccountVisitUnvisitedTransactions(Account *acc,
                                      gboolean (*proc)(Transaction *t,
                                                       void *data),
                                      void *data,
                                      GHashTable *visited_txns) {
  gboolean keep_going = TRUE;
  GList *lp;

  if(!acc) return(FALSE);
  if(!proc) return(FALSE);
  if(!visited_txns) return(FALSE);

  for(lp = xaccAccountGetSplitList(acc); lp && keep_going; lp = lp->next) {
    Split *s = (Split *) lp->data;
    Transaction *t = xaccSplitGetParent(s);
    
    if(t) {
      const GUID *guid = xaccTransGetGUID(t);
      gpointer been_here = g_hash_table_lookup(visited_txns, guid);
      
      if(!been_here) {
        g_hash_table_insert(visited_txns, (gpointer) guid, (gpointer) 1);
        if(!proc(t, data)) {
          keep_going = FALSE;
        }
      }
    }
  }
  return(keep_going);
}

gboolean
xaccAccountForEachTransaction(Account *acc,
                              gboolean (*proc)(Transaction *t, void *data),
                              void *data) {
  GHashTable *visited_txns = NULL;
  gboolean result = FALSE;

  if(!acc) return(FALSE);
  if(!proc) return(FALSE);
  
  visited_txns = guid_hash_table_new();
  if(visited_txns) {
    result =
      xaccAccountVisitUnvisitedTransactions(acc, proc, data, visited_txns);
  }
  
  /* cleanup */
  if(visited_txns) g_hash_table_destroy(visited_txns);  
  return(result);
}

/********************************************************************\
\********************************************************************/

