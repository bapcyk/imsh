CC = gcc

SIOD_OBJS = siod/slib.o siod/sliba.o siod/trace.o
SIOD_LIB = siod/libsiod.a
CFLAGS = -Isiod -I.

$(SIOD_LIB): $(SIOD_OBJS)
	ar rcs -o $(SIOD_LIB) $?

imsh: imsh.o $(SIOD_LIB) libfreeimage.a
	$(CC) -o $@ $^

test%: imsh
	@echo ">>> Do $@.scm..."
	@./imsh $@.scm

.PHONY: all
all: test algolist.html

algolist.html: algolist.awk *.desc
	ls -1 test*.scm|awk -v rooturl=https://github.com/bapcyk/imsh/blob/master/src -f algolist.awk > $@

.PHONY: test
tests = $(wildcard test[0-9]*.scm)
test: imsh
	@echo "Do tests:"
	@echo "========="
	@$(foreach f,$(tests),echo ">>> Do $f..."; ./imsh $f;)

.PHONY: cleanout
cleanout:
	-$(RM) out/*.*

.PHONY: clean
clean:
	-$(RM) *.exe *.o *.a
	-$(RM) siod/*.o siod/*.a

libfreeimage.a:
	pexports freeimage.dll | sed "s/^_//" > libfreeimage.d
	dlltool --add-underscore -d libfreeimage.def -l libfreeimage.a
