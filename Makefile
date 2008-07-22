# Makefile for building mupdf and related stuff
# Valid option to make:
# CFG=[rel|dbg] - dbg if not given
# WITH_JASPER=[yes|no] -default: yes

# Symbolic names for HOST variable
HOST_LINUX := Linux
HOST_MAC := Darwin
HOST_CYGWIN := CYGWIN_NT-6.0

# HOST can be: Linux, Darwin, CYGWIN_NT-6.0
HOST := $(shell uname -s)

VPATH=base:raster:world:stream:mupdf:apps

# make dbg default target if none provided
ifeq ($(CFG),)
CFG=dbg
endif

ifeq ($(WITH_JASPER),)
WITH_JASPER=yes
endif

INCS = -I include -I cmaps

FREETYPE_CFLAGS  = `freetype-config --cflags`
FREETYPE_LDFLAGS = `freetype-config --libs`

FONTCONFIG_CFLAGS  = `pkg-config fontconfig --cflags`
FONTCONFIG_LDFLAGS = `pkg-config fontconfig --libs`

# cc-option
# Usage: OP_CFLAGS+=$(call cc-option, -falign-functions=0, -malign-functions=0)

cc-option = $(shell if $(CC) $(OP_CFLAGS) $(1) -S -o /dev/null -xc /dev/null \
              > /dev/null 2>&1; then echo "$(1)"; else echo "$(2)"; fi ;)

CFLAGS += -g -Wall
CFLAGS += $(call cc-option, -Wno-pointer-sign, "")

ifeq ($(CFG),dbg)
CFLAGS += -O0  ${INCS}
else
CFLAGS += -O2 ${INCS} -DNDEBUG
endif

ifeq ($(HOST),$(HOST_LINUX))
CFLAGS += -std=gnu99 -DHAVE_C99
endif

ifeq ($(HOST),$(HOST_MAC))
CFLAGS += -std=gnu99 -DHAVE_C99
endif

CFLAGS += ${FREETYPE_CFLAGS} ${FONTCONFIG_CFLAGS}

LDFLAGS += ${FREETYPE_LDFLAGS} ${FONTCONFIG_LDFLAGS} -lm -ljpeg

ifeq ($(WITH_JASPER),yes)
CFLAGS += -DHAVE_JASPER
LDFLAGS += -ljasper
endif

CFLAGS += -DUSE_STATIC_CMAPS
CFLAGS += -DDUMP_STATIC_CMAPS

OUTDIR=obj-$(CFG)

BASE_SRC = \
	base_memory.c \
	base_error.c \
	base_hash.c \
	base_matrix.c \
	base_rect.c \
	base_rune.c \

ifeq ($(HOST),$(HOST_LINUX))
BASE_SRC += \
	util_strlcat.c \
	util_strlcpy.c

CFLAGS += -DNEED_STRLCPY
endif

#./base/base_cleanname.c
#./base/base_cpudep.c

STREAM_SRC = \
	crypt_aes.c \
	crypt_arc4.c \
	crypt_crc32.c \
	crypt_md5.c \
	filt_a85d.c \
	filt_a85e.c \
	filt_aes.c \
	filt_ahxd.c \
	filt_ahxe.c \
	filt_arc4.c \
	filt_faxd.c \
	filt_faxdtab.c \
	filt_faxe.c \
	filt_faxetab.c \
	filt_flate.c \
	filt_lzwd.c \
	filt_lzwe.c \
	filt_null.c \
	filt_pipeline.c \
	filt_predict.c \
	filt_rld.c \
	filt_rle.c \
	obj_array.c \
	obj_dict.c \
	obj_parse.c \
	obj_print.c \
	obj_simple.c \
	stm_buffer.c \
	stm_filter.c \
	stm_misc.c \
	stm_open.c \
	stm_read.c \
	stm_write.c \
	filt_dctd.c \
	filt_dcte.c \

ifeq ($(WITH_JASPER),yes)
STREAM_SRC += filt_jpxd.c
endif

#filt_jbig2d.c \

RASTER_SRC = \
	archx86.c \
	blendmodes.c \
	imagescale.c \
	pathfill.c \
	pixmap.c \
	glyphcache.c \
	imageunpack.c \
	pathscan.c \
	porterduff.c \
	imagedraw.c \
	meshdraw.c \
	pathstroke.c \
	render.c \

WORLD_SRC = \
	node_misc1.c \
	node_misc2.c \
	node_optimize.c \
	node_path.c \
	node_text.c \
	node_toxml.c \
	node_tree.c \
	res_colorspace.c \
	res_font.c \
	res_image.c \
	res_shade.c \

#node_tolisp.c \

MUPDF_SRC = \
	pdf_annot.c \
	pdf_build.c \
	pdf_cmap.c \
	pdf_colorspace1.c \
	pdf_colorspace2.c \
	pdf_crypt.c \
	pdf_debug.c \
	pdf_doctor.c \
	pdf_font.c \
	pdf_fontagl.c \
	pdf_fontenc.c \
	pdf_function.c \
	pdf_image.c \
	pdf_interpret.c \
	pdf_lex.c \
	pdf_nametree.c \
	pdf_open.c \
	pdf_outline.c \
	pdf_page.c \
	pdf_pagetree.c \
	pdf_parse.c \
	pdf_pattern.c \
	pdf_repair.c \
	pdf_resources.c \
	pdf_save.c \
	pdf_shade.c \
	pdf_shade1.c \
	pdf_shade4.c \
	pdf_store.c \
	pdf_stream.c \
	pdf_type3.c \
	pdf_unicode.c \
	pdf_xobject.c \
	pdf_xref.c \
	pdf_fontfilefc.c \

LIBS_SRC = \
	${BASE_SRC} \
	${STREAM_SRC} \
	${RASTER_SRC} \
	${WORLD_SRC} \
	${MUPDF_SRC}

LIBS_OBJ = $(patsubst %.c, $(OUTDIR)/FITZ_%.o, ${LIBS_SRC})
LIBS_DEP = $(patsubst %.o, %.d, $(LIBS_OBJ))

PDFTOOL_SRC = \
	pdftool.c
PDFTOOL_OBJ = $(patsubst %.c, $(OUTDIR)/FITZ_%.o, ${PDFTOOL_SRC})
PDFTOOL_DEP = $(patsubst %.o, %.d, $(PDFTOOL_OBJ))
PDFTOOL_APP = ${OUTDIR}/pdftool

PDFBENCH_SRC = \
	pdfbench.c
PDFBENCH_OBJ = $(patsubst %.c, $(OUTDIR)/FITZ_%.o, ${PDFBENCH_SRC})
PDFBENCH_DEP = $(patsubst %.o, %.d, $(PDFBENCH_OBJ))
PDFBENCH_APP = ${OUTDIR}/pdfbench

all: inform ${OUTDIR} ${PDFTOOL_APP} ${PDFBENCH_APP}

$(OUTDIR):
	@mkdir -p $(OUTDIR)

$(PDFTOOL_APP): ${LIBS_OBJ} ${PDFTOOL_OBJ}
	$(CC) -g -o $@ $^ ${LDFLAGS}

$(PDFBENCH_APP): ${LIBS_OBJ} ${PDFBENCH_OBJ}
	$(CC) -g -o $@ $^ ${LDFLAGS}

$(OUTDIR)/FITZ_%.o: %.c
	$(CC) -MD -c $(CFLAGS) -o $@ $<

-include $(LIBS_DEP)
-include $(PDFTOOL_DEP)
-include $(PDFBENCH_DEP)

inform:
ifneq ($(CFG),rel)
ifneq ($(CFG),dbg)
	@echo "Invalid configuration: '"$(CFG)"'"
	@echo "Valid configurations: rel, dbg (e.g. make CFG=dbg)"
	@exit 1
endif
endif

clean: inform
	rm -rf obj-$(CFG)

cleanall:
	rm -rf obj-*

#./apps/common/pdfapp.c
#./apps/mozilla/moz_main.c
#./apps/mozilla/npunix.c
#./apps/unix/x11pdf.c
#./apps/unix/ximage.c
