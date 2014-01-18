NO_COLOR=\033[0m
OK_COLOR=\033[32;01m
ERROR_COLOR=\033[31;01m
WARN_COLOR=\033[33;01m
MRUBY_TMP=mruby-tmp
MGEM_TMP=$(MRUBY_TMP)/mrbgems/mruby-lxc-ghe
BIN_NAME=lxc-ghe

all: deps
	cd mruby-tmp && bash mruby_build.sh
	mkdir -p bin
	mv $(MRUBY_TMP)/bin/mruby bin/lxc-ghe

deps:
	@echo "$(OK_COLOR)==> Fetching mruby$(NO_COLOR)"
	@git clone --depth=1 https://github.com/rubiojr/mruby $(MRUBY_TMP) || /bin/true
	@cp mruby_config.rb mruby_build.sh $(MRUBY_TMP)
	@rm -rf $(MGEM_TMP) || true
	@cp -r mrbgem-skel $(MGEM_TMP)
	@cp loader/loader.c $(MRUBY_TMP)/mrbgems/mruby-bin-mruby/tools/mruby/mruby.c
	@cp loader/loader.rake $(MRUBY_TMP)/mrbgems/mruby-bin-mruby/mrbgem.rake
	@find lib/ -type f -name "*.rb"| xargs cat > $(MGEM_TMP)/mrblib/lxc-ghe.rb

clean:
	@rm -rf bin
	@rm -rf $(MRUBY_TMP)

install:
	@cp bin/* /usr/local/bin/

.PHONY: all deps
