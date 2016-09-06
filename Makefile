MODULES = format_hstore
EXTENSION = format
DATA = format--0.1.sql
REGRESS = format

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
