MODULES = replace
EXTENSION = format
DATA = format--1.0.sql
REGRESS = format
REGRESS_OPTS = 

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
