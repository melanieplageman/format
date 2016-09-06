-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION format" to load this file. \quit

CREATE OR REPLACE FUNCTION format(string TEXT, h HSTORE)
  RETURNS TEXT AS
'replace.so', 'replace'
LANGUAGE C IMMUTABLE STRICT;