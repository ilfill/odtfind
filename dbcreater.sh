#!/bin/sh

rm df.sql

sqlite3 df.sql "PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE hash (id integer primary key, md5 text, path text);
CREATE TABLE docs (id integer primary key, path text, body text);
COMMIT;"