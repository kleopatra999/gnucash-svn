/*
 * Split.cpp
 * Copyright (C) 2010 Christian Stimming
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, contact:
 *
 * Free Software Foundation           Voice:  +1-617-542-5942
 * 51 Franklin Street, Fifth Floor    Fax:    +1-617-542-2652
 * Boston, MA  02110-1301,  USA       gnu@gnu.org
 */

#include "Split.hpp"

#include "gnc/Account.hpp"
#include "gnc/Book.hpp"
#include "gnc/Transaction.hpp"

namespace gnc
{

Book Split::getBook() const { return xaccSplitGetBook(get()); }

Account Split::getAccount() const { return xaccSplitGetAccount(get()); }
void Split::setAccount(Account& acc) { xaccSplitSetAccount(get(), acc.get()); }


Transaction Split::getParent() const { return xaccSplitGetParent(get()); }
void Split::setParent(Transaction& trans) { xaccSplitSetParent(get(), trans.get()); }



} // END namespace gnc
