/* 
 * FILE:
 * checkpoint.c
 *
 * FUNCTION:
 * Account balance checkpointing.
 * Not used in single-user mode; vital for multi-user mode.
 *
 * HISTORY:
 * Copyright (c) 2000, 2001 Linas Vepstas
 * 
 */

#define _GNU_SOURCE
#include <glib.h>
#include <stdio.h>  
#include <string.h>  
#include <sys/types.h>  

#include <pgsql/libpq-fe.h>  

#include "Account.h"
#include "AccountP.h"
#include "Backend.h"
#include "BackendP.h"
#include "Group.h"
#include "gnc-commodity.h"
#include "gnc-engine-util.h"
#include "guid.h"
#include "GNCId.h"
#include "GNCIdP.h"

#include "builder.h"
#include "PostgresBackend.h"

#include "putil.h"

static short module = MOD_BACKEND; 

/* ============================================================= */
/* include autogenerated code */

#include "check-autogen.c"

/* ============================================================= */
/* recompute *all* checkpoints for the account */

static void
pgendAccountRecomputeAllCheckpoints (PGBackend *be, const GUID *acct_guid)
{
   Timespec this_ts, prev_ts;
   GMemChunk *chunk;
   GList *node, *checkpoints = NULL;
   PGresult *result;
   Checkpoint *bp;
   char *p;
   int i, nrows, nsplits;
   Account *acc;
   const char *commodity_name;

   if (!be) return;
   ENTER("be=%p", be);

   acc = xaccLookupEntity (acct_guid, GNC_ID_ACCOUNT);
   commodity_name = gnc_commodity_get_unique_name (xaccAccountGetCommodity(acc));

   chunk = g_mem_chunk_create (Checkpoint, 300, G_ALLOC_ONLY);

   /* prevent others from inserting any splits while we recompute 
    * the checkpoints. (hack alert -verify that this is the correct
    * lock) */
   p = "BEGIN WORK; "
       "LOCK TABLE gncEntry IN SHARE MODE; "
       "LOCK TABLE gncCheckpoint IN ACCESS EXCLUSIVE MODE; ";
   SEND_QUERY (be,p, );
   FINISH_QUERY(be->connection);

   /* Blow all the old checkpoints for this account out of the water.
    * This should help ensure against accidental corruption.
    */
   p = be->buff; *p = 0;
   p = stpcpy (p, "DELETE FROM gncCheckpoint WHERE accountGuid='");
   p = guid_to_string_buff (acct_guid, p);
   p = stpcpy (p, "';");
   SEND_QUERY (be,be->buff, );
   FINISH_QUERY(be->connection);

   /* and now, fetch *all* of the splits in this account */
   p = be->buff; *p = 0;
   p = stpcpy (p, "SELECT gncEntry.amountNum AS amountNum, "
                  "       gncEntry.reconciled AS reconciled,"
                  "       gncTransaction.date_posted AS date_posted "
                  "FROM gncEntry, gncTransaction "
                  "WHERE gncEntry.transGuid = gncTransaction.transGuid "
                  "AND accountGuid='");
   p = guid_to_string_buff (acct_guid, p);
   p = stpcpy (p, "' ORDER BY gncTransaction.date_posted ASC;");
   SEND_QUERY (be,be->buff, );

   /* malloc a new checkpoint, set it to the dawn of AD time ... */
   bp = g_chunk_new0 (Checkpoint, chunk);
   checkpoints = g_list_prepend (checkpoints, bp);
   this_ts = gnc_iso8601_to_timespec_local ("1970-04-15 08:35:46.00");
   bp->datetime = this_ts;
   bp->account_guid = acct_guid;
   bp->commodity = commodity_name;

   /* malloc a new checkpoint ... */
   nsplits = 0;
   bp = g_chunk_new0 (Checkpoint, chunk);
   checkpoints = g_list_prepend (checkpoints, bp);
   bp->account_guid = acct_guid;
   bp->commodity = commodity_name;

   /* start adding up balances */
   i=0; nrows=0;
   do {
      GET_RESULTS (be->connection, result);
      {
         int j, jrows;
         int ncols = PQnfields (result);
         jrows = PQntuples (result);
         nrows += jrows;
         PINFO ("query result %d has %d rows and %d cols",
            i, nrows, ncols);

         for (j=0; j<jrows; j++)
         {
            gint64 amt;
            char recn;
            
            /* lets see if its time to start a new checkpoint */
            /* look for splits that occur at least ten seconds apart */
            prev_ts = this_ts;
            prev_ts.tv_sec += 10;
            this_ts = gnc_iso8601_to_timespec_local (DB_GET_VAL("date_posted",j));
            if ((MIN_CHECKPOINT_COUNT < nsplits) &&
                (timespec_cmp (&prev_ts, &this_ts) < 0))
            {
               Checkpoint *next_bp;

               /* Set checkpoint five seconds back. This is safe,
                * because we looked for a 10 second gap above */
               this_ts.tv_sec -= 5;
               bp->datetime = this_ts;

               /* and now, build a new checkpoint */
               nsplits = 0;
               next_bp = g_chunk_new0 (Checkpoint, chunk);
               checkpoints = g_list_prepend (checkpoints, next_bp);
               *next_bp = *bp;
               bp = next_bp;
               bp->account_guid = acct_guid;
               bp->commodity = commodity_name;
            }
            nsplits ++;

            /* accumulate balances */
            amt = atoll (DB_GET_VAL("amountNum",j));
            recn = (DB_GET_VAL("reconciled",j))[0];
            bp->balance += amt;
            if (NREC != recn)
            {
               bp->cleared_balance += amt;
            }
            if (YREC == recn)
            {
               bp->reconciled_balance += amt;
            }

         }
      }

      PQclear (result);
      i++;
   } while (result);
   
   /* set the timestamp on the final checkpoint,
    *  8 seconds past the very last split */
   this_ts.tv_sec += 8;
   bp->datetime = this_ts;

   /* now store the checkpoints */
   for (node = checkpoints; node; node = node->next)
   {
      bp = (Checkpoint *) node->data;
      pgendStoreOneCheckpointOnly (be, bp, SQL_INSERT);
   }

   g_list_free (checkpoints);
   g_mem_chunk_destroy (chunk);

   p = "COMMIT WORK;";
   SEND_QUERY (be,p, );
   FINISH_QUERY(be->connection);

}

/* ============================================================= */
/* recompute fresh balance checkpoints for every account */

void
pgendGroupRecomputeAllCheckpoints (PGBackend *be, AccountGroup *grp)
{
   GList *acclist, *node;

   acclist = xaccGroupGetSubAccounts(grp);
   for (node = acclist; node; node=node->next)
   {
      Account *acc = (Account *) node->data;
      pgendAccountRecomputeAllCheckpoints (be, xaccAccountGetGUID(acc));
   }
   g_list_free (acclist);
}

/* ============================================================= */
/* get checkpoint value for the account 
 * We find the checkpoint which matches the account and commodity,
 * for the first date immediately preceeding the date.  
 * Then we fill in the balance fields for the returned query.
 */

static void
pgendAccountGetCheckpoint (PGBackend *be, Checkpoint *chk)
{
   PGresult *result;
   int i, nrows;
   char * p;

   if (!be || !chk) return;
   ENTER("be=%p", be);

   /* create the query we need */
   p = be->buff; *p = 0;
   p = stpcpy (p, "SELECT balance, cleared_balance, reconciled_balance "
                  "FROM gncCheckpoint "
                  "WHERE accountGuid='");
   p = guid_to_string_buff (chk->account_guid, p);
   p = stpcpy (p, "' AND commodity='");
   p = stpcpy (p, chk->commodity);
   p = stpcpy (p, "' AND date_xpoint <'");
   p = gnc_timespec_to_iso8601_buff (chk->datetime, p);
   p = stpcpy (p, "' ORDER BY date_xpoint DESC LIMIT 1;");
   SEND_QUERY (be,be->buff, );

   i=0; nrows=0;
   do {
      GET_RESULTS (be->connection, result);
      {
         int j=0, jrows;
         int ncols = PQnfields (result);
         jrows = PQntuples (result);
         nrows += jrows;
         PINFO ("query result %d has %d rows and %d cols",
            i, nrows, ncols);

         if (1 < nrows) 
         {
            PERR ("excess data");
            PQclear (result);
            return;
         }
         chk->balance = atoll(DB_GET_VAL("balance", j));
         chk->cleared_balance = atoll(DB_GET_VAL("cleared_balance", j));
         chk->reconciled_balance = atoll(DB_GET_VAL("reconciled_balance", j));
      }

      PQclear (result);
      i++;
   } while (result);

   LEAVE("be=%p", be);
}

/* ============================================================= */
/* get checkpoint value for all accounts */

void
pgendGroupGetAllCheckpoints (PGBackend *be, AccountGroup*grp)
{
   Checkpoint chk;
   GList *acclist, *node;

   if (!be || !grp) return;
   ENTER("be=%p", be);

   chk.datetime.tv_sec = time(0);
   chk.datetime.tv_nsec = 0;

   acclist = xaccGroupGetSubAccounts (grp);

   /* loop over all accounts */
   for (node=acclist; node; node=node->next)
   {
      Account *acc;
      const gnc_commodity *com;
      gint64 deno;
      gnc_numeric baln;
      gnc_numeric cleared_baln;
      gnc_numeric reconciled_baln;

      /* setupwhat we will match for */
      acc = (Account *) node->data;
      com = xaccAccountGetCommodity(acc);
      chk.commodity = gnc_commodity_get_unique_name(com);
      chk.account_guid = xaccAccountGetGUID (acc);
      chk.balance = 0;
      chk.cleared_balance = 0;
      chk.reconciled_balance = 0;

      /* get the checkpoint */
      pgendAccountGetCheckpoint (be, &chk);

      /* set the account balances */
      deno = gnc_commodity_get_fraction (com);
      baln = gnc_numeric_create (chk.balance, deno);
      cleared_baln = gnc_numeric_create (chk.cleared_balance, deno);
      reconciled_baln = gnc_numeric_create (chk.reconciled_balance, deno);

      xaccAccountSetStartingBalance (acc, baln,
                                     cleared_baln, reconciled_baln);
   }

   g_list_free (acclist);
   LEAVE("be=%p", be);
}

/* ======================== END OF FILE ======================== */
