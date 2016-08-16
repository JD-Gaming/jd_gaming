dirs := games ai apps

all:	build

define build-target
  build:: ; make -C $1
endef

$(foreach dir,$(dirs),$(eval $(call build-target,$(dir))))

.SILENT:
