
-- these tables roughly mirror the c structs in 
-- TransactionP.h,  AccountP.h and GroupP.h
-- these tables are hand-built, but maybe they should be 
-- autobuilt with the m4 macros ...


-- each child of a group will have its own record.
DROP TABLE gncGroup;
CREATE TABLE gncGroup (
	groupGuid	CHAR(32) PRIMARY KEY, 
	parentGuid	CHAR(32)
);

CREATE INDEX gncGroup_pg_idx ON gncGroup (parentGuid);

-- hack alert -- add the kvp frames, the currency tables,
              -- the current balances, etc

DROP TABLE gncAccount;
CREATE TABLE gncAccount (
	accountGuid	CHAR(32) PRIMARY KEY,
--	parentGuid	CHAR(32),
--	childrenGuid	CHAR(32),
	accountName 	TEXT DEFAULT 'xoxo',
	accountCode 	TEXT,
	description 	TEXT,
	notes	 	TEXT,
	type		INT2,
	currency	TEXT
);

-- CREATE INDEX gncAccount_pg_idx ON gncAccount (parentGuid);
-- CREATE INDEX gncAccount_ch_idx ON gncAccount (childrenGuid);

-- hack alert -- docref ??

DROP TABLE gncTransaction;
CREATE TABLE gncTransaction (
	transGuid		CHAR(32) PRIMARY KEY,
	date_entered	 	DATETIME,
	date_posted	 	DATETIME,
	num			TEXT,
	description		TEXT
);

-- a gncEntry is what we call 'Split' elsewhere in the engine

DROP TABLE gncEntry;
CREATE TABLE gncEntry (
	entryGuid		CHAR(32) PRIMARY KEY,
	accountGuid		CHAR(32),
	transGuid		CHAR(32),
	memo			TEXT,
	action			TEXT,
	reconciled		CHAR,
	date_reconciled 	DATETIME,
	amount			FLOAT8 DEFAULT '0.0',
	share_price		FLOAT8 DEFAULT '0.0'
);

CREATE INDEX gncEntry_acc_idx ON gncEntry (accountGuid);
CREATE INDEX gncEntry_trn_idx ON gncEntry (transGuid);

-- populate with some bogus data
INSERT INTO gncAccount (accountGuid, accountName, description) VALUES
    ('9101752f77d6615dcdc0fffe24f0de2',
     'Swipe Trading Account', 
     'Swipe Brokers Margin Account');
INSERT INTO gncAccount (accountGuid, accountName, description) VALUES
    ('0d7c1819693c85c16d5556b37f6caf9d',
     'Stock Dividends &amp; Distributions',
     'Stock Dividends &amp; Distributions');

INSERT INTO gncTransaction (transGuid, date_entered, 
                date_posted, num, description) VALUES
    ('2ebc806e72c17bdc3c2c4e964b82eff8',
     '1998-07-01 11:00:00.345678 -0500',
     '1998-07-02 11:00:00.678945 -0500',
     '101aaa',
     'Interest at 3.5%');

