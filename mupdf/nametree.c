#include <fitz.h>
#include <mupdf.h>

#define SAFE_FREE_OBJ(obj) if ( (obj) && !fz_isindirect(obj) ) fz_dropobj(obj); obj = nil;

static fz_error *
grownametree(pdf_nametree *nt)
{
	int newcap;
	struct fz_keyval_s *newitems;

	newcap = nt->cap * 2;

	newitems = fz_realloc(nt->items, sizeof(struct fz_keyval_s) * newcap);
	if (!newitems)
		return fz_outofmem;

	nt->items = newitems;
	nt->cap = newcap;

	memset(nt->items + nt->cap, 0, sizeof(struct fz_keyval_s) * (newcap - nt->cap));

	return nil;
}

static fz_error *
nametreepush(pdf_nametree *nt, fz_obj *key, fz_obj *val)
{
	fz_error *error = nil;

	if (nt->len + 1 > nt->cap) {
		error = grownametree(nt);
		if (error) return error;
	}

	nt->items[nt->len].k = fz_keepobj(key);
	nt->items[nt->len].v = fz_keepobj(val);
	++nt->len;

	return nil;
}

static fz_error *
loadnametree(pdf_nametree *nt, pdf_xref *xref, fz_obj *root)
{
	fz_obj *localroot = root;
	fz_error *error = nil;
	fz_obj *names = nil;
	fz_obj *kids = nil;
	fz_obj *key = nil;
/*XXX please dont use _ in variable names: */
	fz_obj *ref_val = nil;
	fz_obj *ref = nil;
	int i, len;

	error = pdf_resolve(&localroot, xref);
	if (error) goto cleanup;

	names = fz_dictgets(localroot, "Names");

	/* Leaf node */
	if (names)
	{
		error = pdf_resolve(&names, xref);
		if (error) goto cleanup;

		if (!fz_isarray(names)) {
			error = fz_throw("type check in nametree");
			goto cleanup;
		}

		len = fz_arraylen(names);

		if (len % 2)
/*XXX with null error object? */
			goto cleanup;

		len /= 2;

		for (i = 0; i < len; ++i) {
			key = fz_arrayget(names, i*2);
			ref_val = fz_arrayget(names, i*2 + 1);

			error = pdf_resolve(&key, xref);
			if (error) goto cleanup;

			if (!fz_isstring(key)) {
				error = fz_throw("type check in nametree");
				goto cleanup;
			}

			nametreepush(nt, key, ref_val);
/*XXX check error! */

			fz_dropobj(key);
			key = nil;
		}
/*XXX you leak names */
	}

	/* Intermediate node */
	else
	{
		kids = fz_dictgets(localroot, "Kids");

		if (kids) {
			error = pdf_resolve(&kids, xref);
			if (error) goto cleanup;

			if (!fz_isarray(kids)) {
				error = fz_throw("type check in nametree");
				goto cleanup;
			}

			len = fz_arraylen(kids);
			for (i = 0; i < len; ++i) {
				ref = fz_arrayget(kids, i);
				loadnametree(nt, xref, ref);
/*XXX check error! */
			}
		}
		else {
			/* Invalid name tree dict node */
			error = fz_throw("invalid nametree node: there's no Names and Kids key");
			goto cleanup;
		}
/*XXX you leak kids */
	}

	return nil;

cleanup:
/*XXX i dont like BIG_FAT_MACROS, use: if (obj) fz_dropobj(obj) */
	SAFE_FREE_OBJ(localroot);
	SAFE_FREE_OBJ(names);
	SAFE_FREE_OBJ(kids);
	SAFE_FREE_OBJ(key);
	return error;
}

static int
compare(const void *elem1, const void *elem2)
{
	struct fz_keyval_s *keyval1 = (struct fz_keyval_s *)elem1;
	struct fz_keyval_s	*keyval2 = (struct fz_keyval_s *)elem2;
	int strlen1 = fz_tostringlen(keyval1->k);
	int strlen2 = fz_tostringlen(keyval2->k);
	int memcmpval = memcmp(fz_tostringbuf(keyval1->k),
		fz_tostringbuf(keyval2->k), MIN(strlen1, strlen2));

	if (memcmpval != 0)
		return memcmpval;

	return strlen1 - strlen2;
}

static fz_error *
sortnametree(pdf_nametree *nt)
{
	qsort(nt->items, nt->len, sizeof(nt->items[0]), compare);
	return nil;
}

fz_error *
pdf_loadnametree(pdf_nametree **pnt, pdf_xref *xref, char* key)
{
	fz_error *error;
	pdf_nametree *nt = nil;
	fz_obj *catalog = nil;
	fz_obj *names = nil;
	fz_obj *trailer;
	fz_obj *ref;
	fz_obj *root = nil;

	trailer = xref->trailer;

	ref = fz_dictgets(trailer, "Root");
	error = pdf_loadindirect(&catalog, xref, ref);
	if (error) goto cleanup;

#if 1
	names = fz_dictgets(catalog, "Names");
	if (!names)
	{
		nt = *pnt = fz_malloc(sizeof(pdf_nametree));
		if (!nt) { error = fz_outofmem; goto cleanup; }
		nt->cap = 0;
		nt->len = 0;
		nt->items = 0;
		return nil;
	}

	error = pdf_resolve(&names, xref);
	if (error) goto cleanup;

	root = fz_dictgets(names, key);
	if (!root)
	{
		nt = *pnt = fz_malloc(sizeof(pdf_nametree));
		if (!nt) { error = fz_outofmem; goto cleanup; }
		nt->cap = 0;
		nt->len = 0;
		nt->items = 0;
		return nil;
	}
	error = pdf_resolve(&root, xref);
	if (error) goto cleanup;

#else
/*XXX create empty nametree instead of failing */
	names = fz_dictgets(catalog, "Names");
/*XXX never resolve something that can be null */
	error = pdf_resolve(&names, xref);
	if (error) goto cleanup;

	root = fz_dictgets(names, key);
/*XXX never resolve something that can be null */
	error = pdf_resolve(&root, xref);
	if (error) goto cleanup;
#endif

	nt = *pnt = fz_malloc(sizeof(pdf_nametree));
	if (!nt) { error = fz_outofmem; goto cleanup; }
	nt->cap = 8;
	nt->len = 0;
	nt->items = 0;

	nt->items = fz_malloc(sizeof(struct fz_keyval_s) * nt->cap);

	if (!nt->items) {
		error = fz_outofmem;
		goto cleanup;
	}

	error = loadnametree(nt, xref, root);
	if (error) goto cleanup;

/*XXX not necessary. tree is sorted when you load it. */
	sortnametree(nt);

/*XXX please have separate cleanup and okay cases... */
/*XXX  return nil; here */

cleanup:
/*XXX no BIG_FAT_MACROS, please :) */
	SAFE_FREE_OBJ(root);
	SAFE_FREE_OBJ(names);
	if (catalog) fz_dropobj(catalog);
	if (error && nt) {
		fz_free(nt);
	}
	return error;
}

void
pdf_dropnametree(pdf_nametree *nt)
{
	int i;
	for (i = 0; i < nt->len; i++) {
			fz_dropobj(nt->items[i].k);
			fz_dropobj(nt->items[i].v);
	}
	if (nt->items) fz_free(nt->items);
	fz_free(nt);
}

void
pdf_debugnametree(pdf_nametree *nt)
{
	int i;
	for (i = 0; i < nt->len; i++) {
		printf("   ");
		fz_debugobj(nt->items[i].k);
		printf("   ");
		fz_debugobj(nt->items[i].v);
		printf("\n");
	}
}

fz_obj *
pdf_lookupname(pdf_nametree *nt, fz_obj *name)
{
	struct fz_keyval_s item;
	struct fz_keyval_s *found;
	if (fz_isstring(name)) {
		item.k = name;
		item.v = nil;
/*XXX bsearch is non-standard. please dont use it. */
		found = bsearch(&item, nt->items, nt->len, sizeof(nt->items[0]), compare);
		return found->v;
	}
	return nil;
}

fz_obj *
pdf_lookupnames(pdf_nametree *nt, char *name)
{
	fz_obj *key;
	fz_obj *ref;
	int len = strlen(name);
/*XXX please dont create a new string just to do a lookup. */
/*XXX change this to be the "standard" function and call it from */
/*XXX pdf_lookupname instead. compare name with the fz_tostringbuf() & co */
/*XXX return values. */
	fz_newstring(&key, name, len);
	ref = pdf_lookupname(nt, key);
	fz_dropobj(key);
	return ref;
}

