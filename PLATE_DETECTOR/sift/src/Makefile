INCLUDE=-I ../include/
WFLAGS=-Wall -Wextra
CFLAGS=$(shell pkg-config --cflags opencv) -g -O
LIBS=$(shell pkg-config --libs opencv)
BIN_DIR=../bin


BUILDFLAGS=$(INCLUDE) $(CFLAGS) $(LIBS) $(WFLAGS)

BINARIES=siftfeat match match_imgdb match_db dspfeat siftDemo

all: $(BINARIES)


siftfeat: sift.o siftfeat.o utils.o imgfeatures.o
	gcc $^ -o $(BIN_DIR)/$@ $(BUILDFLAGS)

match: match.o utils.o sift.o kdtree.o imgfeatures.o minpq.o xform.o
	gcc $^ -o $(BIN_DIR)/$@ $(BUILDFLAGS)

match_imgdb: match_imgdb.o utils.o sift.o kdtree.o imgfeatures.o minpq.o xform.o
	gcc $^ -o $(BIN_DIR)/$@ $(BUILDFLAGS)

match_db: match_db.o utils.o sift.o kdtree.o imgfeatures.o minpq.o xform.o
	gcc $^ -o $(BIN_DIR)/$@ $(BUILDFLAGS)

dspfeat: dspfeat.o utils.o imgfeatures.o
	gcc $^ -o $(BIN_DIR)/$@ $(BUILDFLAGS)

siftDemo: siftDemo.o sift.o xform.o utils.o kdtree.o minpq.o imgfeatures.o
	gcc $^ -o $(BIN_DIR)/$@ $(BUILDFLAGS)


# -c: non lanciare il linker
# $@: il target
# $<: la prima dipendenza
# $^: tutte le dipendenze
%.o:: %.c
	gcc $(BUILDFLAGS) -c -o $@ $<


clean:
	@ rm -f $(BIN_DIR)/* *.o
