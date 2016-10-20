-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION format" to load this file. \quit

CREATE OR REPLACE FUNCTION format(string TEXT, h HSTORE)
  RETURNS TEXT AS
'format_hstore', 'format_hstore'
LANGUAGE C IMMUTABLE STRICT;
