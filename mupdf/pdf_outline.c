#include "fitz.h"
#include "mupdf.h"

/* TODO: we leak node on errors */
static fz_error *
loadoutline(pdf_outline **nodep, pdf_xref *xref, fz_obj *dict)
{
	fz_error *error;
	pdf_outline *node;
	fz_obj *obj;

	node = fz_malloc(sizeof(pdf_outline));
	node->title = nil;
	node->link = nil;
	node->child = nil;
	node->next = nil;

	pdf_logpage("load outline {\n");

	obj = fz_dictgets(dict, "Title");
	if (obj)
	{
		error = pdf_resolve(&obj, xref);
		if (error)
			return fz_rethrow(error, "cannot resolve /Title");
		error = pdf_toutf8(&node->title, obj);
		if (error)
			return fz_rethrow(error, "cannot convert Title to UTF-8");
		pdf_logpage("title %s\n", node->title ? node->title : "<unknown>");
	}
	if (!node->title)
		node->title = fz_strdup("<unknown>");

	if (fz_dictgets(dict, "Dest") || fz_dictgets(dict, "A"))
	{
		error = pdf_loadlink(&node->link, xref, dict);
		if (error)
			return fz_rethrow(error, "cannot load link");
	}

	obj = fz_dictgets(dict, "First");
	if (obj)
	{
		error = pdf_resolve(&obj, xref);
		if (error)
			return fz_rethrow(error, "cannot resolve /First");
		error = loadoutline(&node->child, xref, obj);
		fz_dropobj(obj);
		if (error)
			return fz_rethrow(error, "cannot load outline");
	}

	pdf_logpage("}\n");

	obj = fz_dictgets(dict, "Next");
	if (obj)
	{
		error = pdf_resolve(&obj, xref);
		if (error)
			return fz_rethrow(error, "cannot resolve /Next");
		error = loadoutline(&node->next, xref, obj);
		fz_dropobj(obj);
		if (error)
			return fz_rethrow(error, "cannot load outline");
	}

	*nodep = node;
	return fz_okay;
}

fz_error *
pdf_loadoutline(pdf_outline **nodep, pdf_xref *xref)
{
	fz_error *error;
	pdf_outline *node;
	fz_obj *obj;
	fz_obj *first;

	pdf_logpage("load outlines {\n");

	node = nil;

	obj = fz_dictgets(xref->root, "Outlines");
	if (obj)
	{
		error = pdf_resolve(&obj, xref);
		if (error)
			return fz_rethrow(error, "cannot resolve /Outlines");

		first = fz_dictgets(obj, "First");
		if (first)
		{
			error = pdf_resolve(&first, xref);
			fz_dropobj(obj);
			if (error)
				return fz_rethrow(error, "cannot resolve /First");
			error = loadoutline(&node, xref, first);
			fz_dropobj(first);
			if (error)
				return fz_rethrow(error, "cannot load outline");
		}
		else
			fz_dropobj(obj);
	}

	pdf_logpage("}\n");

	*nodep = node;
	return fz_okay;
}

void
pdf_dropoutline(pdf_outline *outline)
{
	if (outline->child)
		pdf_dropoutline(outline->child);
	if (outline->next)
		pdf_dropoutline(outline->next);
	pdf_droplink(outline->link);
	fz_free(outline->title);
	fz_free(outline);
}

void
pdf_debugoutline(pdf_outline *outline, int level)
{
	int i;
	while (outline)
	{
		for (i = 0; i < level; i++)
			putchar(' ');

		if (outline->title)
		    printf("%s ", outline->title);
		else
		    printf("<nil> ");

		if (outline->link)
			fz_debugobj(outline->link->dest);
		else
			printf("<nil>\n");

		if (outline->child)
			pdf_debugoutline(outline->child, level + 2);

		outline = outline->next;
	}
}

