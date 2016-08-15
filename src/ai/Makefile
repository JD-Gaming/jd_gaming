dirs := $(shell find . -maxdepth 1 -type d -a -not -wholename "\.")

all:	build

define build-target
  build:: ; make -C $1 lib
endef

$(foreach dir,$(dirs),$(eval $(call build-target,$(dir))))

.SILENT:
