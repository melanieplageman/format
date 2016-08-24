MODULES = replace
PGXS := $(shell pg_config --pgxs)
include $(PGXS)
