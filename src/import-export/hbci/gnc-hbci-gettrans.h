/********************************************************************\
 * gnc-hbci-gettrans.h -- hbci get transactions function            *
 * Copyright (C) 2002 Christian Stimming                            *
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
\********************************************************************/

#ifndef GNC_HBCI_GETTRANS_H
#define GNC_HBCI_GETTRANS_H

#include <gnome.h>
#include <aqbanking/jobgettransactions.h>
#include "Account.h"

/** Start a GetTrans job. */
void
gnc_hbci_gettrans (GtkWidget *parent, Account *gnc_acc);

/** Finalizes all the things that have to be done with a GetTrans
 * job.  Returns true if everything has been finished succesfully. */
gboolean
gnc_hbci_gettrans_final(GtkWidget *parent, 
			Account *gnc_acc, 
			const AB_JOB *trans_job,
			gboolean run_until_done);


#endif /* GNC_HBCI_GETTRANS_H */
