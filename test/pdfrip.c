#include <fitz.h>
#include <mupdf.h>

int showtree = 0;

void usage()
{
	fprintf(stderr, "usage: pdfrip [-d] [-p password] file.pdf [pages...]\n");
	exit(1);
}

/*
 * Draw page
 */

void runcsi(pdf_xref *xref, pdf_csi *csi, pdf_resources *rdb, fz_obj *stmref)
{
	fz_error *error;

	error = pdf_openstream(xref, fz_tonum(stmref), fz_togen(stmref));
	if (error) fz_abort(error);

	error = pdf_runcsi(csi, rdb, xref->stream);
	if (error) fz_abort(error);

	pdf_closestream(xref);
}

void showpage(pdf_xref *xref, fz_obj *page)
{
	fz_error *error;
	pdf_csi *csi;
	pdf_resources *rdb = nil;
	fz_obj *resources;
	fz_obj *contents;
	int i;

	fz_debugobj(page);
	printf("\n");

	resources = fz_dictgets(page, "Resources");
	if (resources)
	{
		error = pdf_resolve(&resources, xref);
		if (error) fz_abort(error);

		error = pdf_loadresources(&rdb, xref, resources);
		if (error) fz_abort(error);

		// parse resources into native res dict
		fz_dropobj(resources);
	}
	else
		fz_abort(fz_throw("syntaxerror: missing resource dictionary"));

	error = pdf_newcsi(&csi);
	if (error) fz_abort(error);

	contents = fz_dictgets(page, "Contents");
	if (contents)
	{
		if (fz_isarray(contents))
		{
			for (i = 0; i < fz_arraylen(contents); i++)
			{
				runcsi(xref, csi, rdb, fz_arrayget(contents, i));
			}
		}
		else
		{
			// XXX resolve and check if it is an array
			runcsi(xref, csi, rdb, contents);
		}
	}


	if (showtree)
	{
		printf("\nfitz tree:\n");
		fz_debugtree(csi->tree);
	}

	{
		fz_pixmap *pix;
		fz_renderer *gc;
		fz_matrix ctm;
		fz_rect bbox;

#define SCALE 1.0
#define W 700
#define H 900

		fz_newrenderer(&gc);

		bbox.min.x = 0;
		bbox.min.y = 0;
		bbox.max.x = W  * SCALE;
		bbox.max.y = H  * SCALE;

		//ctm = fz_scale(SCALE,SCALE);
		ctm = fz_concat(fz_translate(0, -H), fz_scale(SCALE,-SCALE));

printf("rendering!\n");
		fz_rendertree(&pix, gc, csi->tree, ctm, bbox);
printf("done!\n");

		fz_debugpixmap(pix);
		fz_freepixmap(pix);

		fz_freerenderer(gc);
	}

	pdf_freecsi(csi);
}

int main(int argc, char **argv)
{
	fz_error *error;
	char *filename;
	pdf_xref *xref;
	pdf_pagetree *pages;
	int c;

	char *password = "";

	while ((c = getopt(argc, argv, "dp:")) != -1)
	{
		switch (c)
		{
		case 'p': password = optarg; break;
		case 'd': ++showtree; break;
		default: usage();
		}
	}

	if (argc - optind == 0)
		usage();

	filename = argv[optind++];

	error = pdf_openpdf(&xref, filename);
	if (error)
		fz_abort(error);

	error = pdf_decryptpdf(xref);
	if (error)
		fz_abort(error);

	if (xref->crypt)
	{
		error = pdf_setpassword(xref->crypt, password);
		if (error) fz_abort(error);
	}

	error = pdf_loadpagetree(&pages, xref);
	if (error) fz_abort(error);

	if (optind == argc)
	{
		printf("pagetree\n");
		pdf_debugpagetree(pages);
		printf("\n");
	}

	for ( ; optind < argc; optind++)
	{
		int page = atoi(argv[optind]);
		if (page < 1 || page > pages->count)
			fprintf(stderr, "page out of bounds: %d\n", page);
		printf("page %d\n", page);
		showpage(xref, pages->pobj[page - 1]);
	}

	pdf_closepdf(xref);

	return 0;
}
