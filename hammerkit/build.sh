# Requires libc6-dev:i386 and libssl-dev:i386 gcc-multilib
MRUBY_CONFIG=hammerkit/hammerkit_config.rb make
for f in `ls build/i386/bin/*`; do
	cp $f bin/`basename $f`-i386
done
