#include "fitz.h"
#include "mupdf.h"

/* TODO: error check prints */

#define TIGHT 1

static fz_error *
writestream(fz_stream *out, pdf_xref *xref, pdf_crypt *encrypt, int oid, int gen)
{
	fz_error *error;
	fz_stream *dststm;
	fz_stream *srcstm;
	unsigned char buf[4096];
	fz_filter *ef;
	int n;

	fz_print(out, "stream\n");

	if (encrypt)
	{
		error = pdf_cryptstream(&ef, encrypt, oid, gen);
		if (error)
			return fz_rethrow(error, "cannot create encryption filter");

		error = fz_openwfilter(&dststm, ef, out);
		fz_dropfilter(ef);
		if (error)
			return fz_rethrow(error, "cannot open encryption stream");
	}
	else
	{
		dststm = fz_keepstream(out);
	}

	error = pdf_openrawstream(&srcstm, xref, oid, gen);
	if (error)
	{
		error = fz_rethrow(error, "cannot open raw stream");
		goto cleanupdst;
	}

	while (1)
	{
		error = fz_read(&n, srcstm, buf, sizeof buf);
		if (error)
		{
			error = fz_rethrow(error, "cannot read stream");
			goto cleanupsrc;
		}
		if (n == 0)
			break;

		error = fz_write(dststm, buf, n);
		if (error)
		{
			error = fz_rethrow(error, "cannot write stream");
			goto cleanupsrc;
		}
	}

	fz_dropstream(srcstm);
	fz_dropstream(dststm);

	fz_print(out, "endstream\n");

	return fz_okay;

cleanupsrc:
	fz_dropstream(srcstm);
cleanupdst:
	fz_dropstream(dststm);
	return error; /* already rethrown */
}

static fz_error *
writeobject(fz_stream *out, pdf_xref *xref, pdf_crypt *encrypt, int oid, int gen)
{
	pdf_xrefentry *x = xref->table + oid;
	fz_error *error;

	error = pdf_cacheobject(xref, oid, gen);
	if (error)
		return fz_rethrow(error, "cannot load object");

	if (encrypt)
		pdf_cryptobj(encrypt, x->obj, oid, gen);

	fz_print(out, "%d %d obj\n", oid, gen);
	fz_printobj(out, x->obj, TIGHT);
	fz_print(out, "\n");

	if (encrypt)
		pdf_cryptobj(encrypt, x->obj, oid, gen);

	if (pdf_isstream(xref, oid, gen))
	{
		error = writestream(out, xref, encrypt, oid, gen);
		if (error)
			return fz_rethrow(error, "cannot write stream");
	}

	fz_print(out, "endobj\n\n");

	return fz_okay;
}

static int countmodified(pdf_xref *xref, int oid)
{
	int i;
	for (i = oid; i < xref->len; i++)
		if (xref->table[i].type != 'a' && xref->table[i].type != 'd')
			return i - oid;
	return i - oid;
}

fz_error *
pdf_updatexref(pdf_xref *xref, char *path)
{
	fz_error *error;
	fz_stream *out;
	int oid;
	int i, n;
	int startxref;
	fz_obj *obj;

	pdf_logxref("updatexref '%s' %p\n", path, xref);

	error = fz_openafile(&out, path);
	if (error)
		return fz_rethrow(error, "cannot open output file");

	fz_print(out, "\n");

	for (oid = 0; oid < xref->len; oid++)
	{
		if (xref->table[oid].type == 'a')
		{
			xref->table[oid].ofs = fz_tell(out);
			error = writeobject(out, xref, xref->crypt, oid, xref->table[oid].gen);
			if (error)
			{
				fz_dropstream(out);
				return fz_rethrow(error, "cannot write object (oid=%d)", oid);
			}
		}
	}

	/* always write out entry 0 in appended xref sections */
	xref->table[0].type = 'd';

	startxref = fz_tell(out);
	fz_print(out, "xref\n");

	oid = 0;
	while (oid < xref->len)
	{
		n = countmodified(xref, oid);

		pdf_logxref("  section %d +%d\n", oid, n);

		fz_print(out, "%d %d\n", oid, n);

		for (i = 0; i < n; i++)
		{
			if (xref->table[oid + i].type == 'd')
				xref->table[oid + i].type = 'f';
			if (xref->table[oid + i].type == 'a')
				xref->table[oid + i].type = 'n';

			fz_print(out, "%010d %05d %c \n",
					xref->table[oid + i].ofs,
					xref->table[oid + i].gen,
					xref->table[oid + i].type);
		}

		oid += n;
		while (oid < xref->len &&
				xref->table[oid].type != 'a' &&
				xref->table[oid].type != 'd')
			oid ++;
	}

	fz_print(out, "\n");

	fz_print(out, "trailer\n<<\n  /Size %d\n  /Prev %d", xref->len, xref->startxref);

	obj = fz_dictgets(xref->trailer, "Root");
	fz_print(out,"\n  /Root %d %d R", fz_tonum(obj), fz_togen(obj));

	obj = fz_dictgets(xref->trailer, "Info");
	if (obj)
		fz_print(out,"\n  /Info %d %d R", fz_tonum(obj), fz_togen(obj));

	obj = fz_dictgets(xref->trailer, "Encrypt");
	if (obj) {
		fz_print(out,"\n  /Encrypt ");
		fz_printobj(out, obj, TIGHT);
	}

	obj = fz_dictgets(xref->trailer, "ID");
	if (obj) {
		fz_print(out,"\n  /ID ");
		fz_printobj(out, obj, TIGHT);
	}

	fz_print(out, "\n>>\n\n");

	fz_print(out, "startxref\n");
	fz_print(out, "%d\n", startxref);
	fz_print(out, "%%%%EOF\n");

	xref->startxref = startxref;

	/* TODO: check for write errors */

	fz_dropstream(out);
	return fz_okay;
}

fz_error *
pdf_savexref(pdf_xref *xref, char *path, pdf_crypt *encrypt)
{
	fz_error *error;
	fz_stream *out;
	int oid;
	int startxref;
	int *ofsbuf;
	fz_obj *obj;
	int eoid, egen;

	pdf_logxref("savexref '%s' %p\n", path, xref);

	/* need to add encryption object for acrobat < 6 */
	if (encrypt)
	{
		pdf_logxref("make encryption dict\n");

		error = pdf_allocobject(xref, &eoid, &egen);
		if (error)
			return fz_rethrow(error, "cannot allocate encryption object");

		pdf_cryptobj(encrypt, encrypt->encrypt, eoid, egen);

		error = pdf_updateobject(xref, eoid, egen, encrypt->encrypt);
		if (error)
			return fz_rethrow(error, "cannot update encryption object");
	}

	ofsbuf = fz_malloc(sizeof(int) * xref->len);
	if (!ofsbuf)
		return fz_throw("outofmem: offset buffer");

	error = fz_openwfile(&out, path);
	if (error)
	{
		fz_free(ofsbuf);
		return fz_rethrow(error, "cannot open output file");
	}

	fz_print(out, "%%PDF-%d.%df\n", xref->version / 10, xref->version % 10);
	fz_print(out, "%%\342\343\317\323\n\n");

	for (oid = 0; oid < xref->len; oid++)
	{
		pdf_xrefentry *x = xref->table + oid;
		if (x->type == 'n' || x->type == 'o' || x->type == 'a')
		{
			ofsbuf[oid] = fz_tell(out);
			error = writeobject(out, xref, encrypt, oid, x->type == 'o' ? 0 : x->gen);
			if (error)
			{
				if (ofsbuf) fz_free(ofsbuf);
				fz_dropstream(out);
				return fz_rethrow(error, "cannot write object (oid=%d)", oid);
			}
		}
		else
		{
			ofsbuf[oid] = x->ofs;
		}
	}

	startxref = fz_tell(out);
	fz_print(out, "xref\n");
	fz_print(out, "0 %d\n", xref->len);

	for (oid = 0; oid < xref->len; oid++)
	{
		int gen = xref->table[oid].gen;
		int type = xref->table[oid].type;
		if (type == 'o')
			gen = 0;
		if (type == 'a' || type == 'o')
			type = 'n';
		if (type == 'd')
			type = 'f';
		fz_print(out, "%010d %05d %c \n", ofsbuf[oid], gen, type);
	}

	fz_print(out, "\n");

	fz_print(out, "trailer\n<<\n  /Size %d", xref->len);
	obj = fz_dictgets(xref->trailer, "Root");
	fz_print(out, "\n  /Root %d %d R", fz_tonum(obj), fz_togen(obj));
	obj = fz_dictgets(xref->trailer, "Info");
	if (obj)
		fz_print(out, "\n  /Info %d %d R", fz_tonum(obj), fz_togen(obj));
	if (encrypt)
	{
		fz_print(out, "\n  /Encrypt %d %d R", eoid, egen);
		fz_print(out, "\n  /ID [");
		fz_printobj(out, encrypt->id, 1);
		fz_printobj(out, encrypt->id, 1);
		fz_print(out, "]");

		pdf_cryptobj(encrypt, encrypt->encrypt, eoid, egen);
	}
	fz_print(out, "\n>>\n\n");

	fz_print(out, "startxref\n");
	fz_print(out, "%d\n", startxref);
	fz_print(out, "%%%%EOF\n");

	xref->startxref = startxref;

	/* TODO: check for write errors */

	if(ofsbuf) fz_free(ofsbuf);
	fz_dropstream(out);
	return fz_okay;
}

