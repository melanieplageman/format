MODULES = replace parse mparse
PGXS := $(shell pg_config --pgxs)
include $(PGXS)
