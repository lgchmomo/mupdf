#include "fitz.h"
#include "mupdf.h"

/*
 * Sweep and mark reachable objects
 *
 * Don't bother chaining errors for this deep recursion
 */

static fz_error *sweepref(pdf_xref *xref, fz_obj *ref);

static fz_error *
sweepobj(pdf_xref *xref, fz_obj *obj)
{
	fz_error *error;
	int i;

	if (fz_isdict(obj))
	{
		for (i = 0; i < fz_dictlen(obj); i++)
		{
			error = sweepobj(xref, fz_dictgetval(obj, i));
			if (error)
				return error; /* too deeply nested for rethrow */
		}
	}

	if (fz_isarray(obj))
	{
		for (i = 0; i < fz_arraylen(obj); i++)
		{
			error = sweepobj(xref, fz_arrayget(obj, i));
			if (error)
				return error; /* too deeply nested for rethrow */
		}
	}

	if (fz_isindirect(obj))
		return sweepref(xref, obj);

	return fz_okay;
}

static fz_error *
sweepref(pdf_xref *xref, fz_obj *ref)
{
	fz_error *error;
	fz_obj *obj;
	int oid;

	oid = fz_tonum(ref);

	if (oid < 0 || oid >= xref->len)
		return fz_throw("object out of range %d", oid);

	if (xref->table[oid].mark)
		return fz_okay;

	xref->table[oid].mark = 1;

	error = pdf_loadindirect(&obj, xref, ref);
	if (error)
		return fz_rethrow(error, "cannot load indirect object");

	error = sweepobj(xref, obj);
	if (error)
	{
		fz_dropobj(obj);
		return error; /* too deeply nested for rethrow */
	}

	fz_dropobj(obj);
	return fz_okay;
}

/*
 * Garbage collect objects not reachable from
 * the trailer dictionary
 */

fz_error *
pdf_garbagecollect(pdf_xref *xref)
{
	fz_error *error;
	int i, g;

	pdf_logxref("garbage collect {\n");

	for (i = 0; i < xref->len; i++)
		xref->table[i].mark = 0;

	error = sweepobj(xref, xref->trailer);
	if (error)
		return fz_rethrow(error, "cannot mark used objects");

	for (i = 0; i < xref->len; i++)
	{
		pdf_xrefentry *x = xref->table + i;
		g = x->gen;
		if (x->type == 'o')
			g = 0;
		if (!x->mark && x->type != 'f' && x->type != 'd')
		{
			error = pdf_deleteobject(xref, i, g);
			if (error)
				return fz_rethrow(error, "cannot delete unmarked object %d", i);
		}
	}

	pdf_logxref("}\n");

	return fz_okay;
}

/*
 * Transplant (copy) objects and streams from one file to another
 */

struct pair
{
	int soid, sgen;
	int doid, dgen;
};

static fz_error *
remaprefs(fz_obj **newp, fz_obj *old, struct pair *map, int n)
{
	fz_error *error;
	int i, o, g;
	fz_obj *tmp, *key;

	if (fz_isindirect(old))
	{
		o = fz_tonum(old);
		g = fz_togen(old);
		for (i = 0; i < n; i++)
			if (map[i].soid == o && map[i].sgen == g)
			{
				error = fz_newindirect(newp, map[i].doid, map[i].dgen);
				if (error)
					return fz_rethrow(error, "cannot remap indirect reference");
			}
	}

	else if (fz_isarray(old))
	{
		error = fz_newarray(newp, fz_arraylen(old));
		if (error)
			return fz_rethrow(error, "cannot remap array");
		for (i = 0; i < fz_arraylen(old); i++)
		{
			tmp = fz_arrayget(old, i);
			error = remaprefs(&tmp, tmp, map, n);
			if (error)
				goto cleanup;
			error = fz_arraypush(*newp, tmp);
			fz_dropobj(tmp);
			if (error)
				goto cleanup;
		}
	}

	else if (fz_isdict(old))
	{
		error = fz_newdict(newp, fz_dictlen(old));
		if (error)
			return fz_rethrow(error, "cannot remap dictionary");
		for (i = 0; i < fz_dictlen(old); i++)
		{
			key = fz_dictgetkey(old, i);
			tmp = fz_dictgetval(old, i);
			error = remaprefs(&tmp, tmp, map, n);
			if (error)
				goto cleanup;
			error = fz_dictput(*newp, key, tmp);
			fz_dropobj(tmp);
			if (error)
				goto cleanup;
		}
	}

	else
	{
		*newp = fz_keepobj(old);
	}

	return fz_okay;

cleanup:
	fz_dropobj(*newp);
	return fz_rethrow(error, "cannot remap object");
}

/*
 * Recursively copy objects from src to dst xref.
 * Start with root object in src xref.
 * Put the dst copy of root into newp.
 */
fz_error *
pdf_transplant(pdf_xref *dst, pdf_xref *src, fz_obj **newp, fz_obj *root)
{
	fz_error *error;
	struct pair *map;
	fz_obj *old, *new;
	fz_buffer *stm;
	int i, n;

	pdf_logxref("transplant {\n");

	for (i = 0; i < src->len; i++)
		src->table[i].mark = 0;

	error = sweepobj(src, root);
	if (error)
		return fz_rethrow(error, "cannot mark used objects");

	for (n = 0, i = 0; i < src->len; i++)	
		if (src->table[i].mark)
			n++;

	pdf_logxref("marked %d\n", n);

	map = fz_malloc(sizeof(struct pair) * n);
	if (!map)
		return fz_throw("outofmem: remapping table");

	for (n = 0, i = 0; i < src->len; i++)	
	{
		if (src->table[i].mark)
		{
			map[n].soid = i;
			map[n].sgen = src->table[i].gen;
			if (src->table[i].type == 'o')
				map[n].sgen = 0;
			error = pdf_allocobject(dst, &map[n].doid, &map[n].dgen);
			if (error)
				goto cleanup;
			n++;
		}
	}

	error = remaprefs(newp, root, map, n);
	if (error)
		goto cleanup;

	for (i = 0; i < n; i++)	
	{
		pdf_logxref("copyfrom %d %d to %d %d\n",
				map[i].soid, map[i].sgen,
				map[i].doid, map[i].dgen);

		error = pdf_loadobject(&old, src, map[i].soid, map[i].sgen);
		if (error)
			goto cleanup;

		if (pdf_isstream(src, map[i].soid, map[i].sgen))
		{
			error = pdf_loadrawstream(&stm, src, map[i].soid, map[i].sgen);
			if (error)
				goto cleanup;
			pdf_updatestream(dst, map[i].doid, map[i].dgen, stm);
			fz_dropbuffer(stm);
		}

		error = remaprefs(&new, old, map, n);
		fz_dropobj(old);
		if (error)
			goto cleanup;

		error = pdf_updateobject(dst, map[i].doid, map[i].dgen, new);
		fz_dropobj(new);
		if (error)
			goto cleanup;
	}

	pdf_logxref("}\n");

	fz_free(map);
	return fz_okay;

cleanup:
	fz_free(map);
	return fz_rethrow(error, "cannot transplant objects");
}

