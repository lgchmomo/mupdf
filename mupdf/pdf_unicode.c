#include <fitz.h>
#include <mupdf.h>

/*
 * ToUnicode map for fonts
 */

fz_error *
pdf_loadtounicode(pdf_font *font, pdf_xref *xref,
	char **strings, char *collection, fz_obj *cmapstm)
{
	fz_error *error;
	fz_cmap *cmap;
	int cid;
	int ucs;
	int i;

	if (fz_isindirect(cmapstm))
	{
		pdf_logfont("tounicode embedded cmap\n");

		error = pdf_loadembeddedcmap(&cmap, xref, cmapstm);
		if (error)
			return error;

		error = fz_newcmap(&font->tounicode);
		if (error)
			goto cleanup;

		for (i = 0; i < (strings ? 256 : 65536); i++)
		{
			cid = fz_lookupcid(font->encoding, i);
			if (cid > 0)
			{
				ucs = fz_lookupcid(cmap, i);
				error = fz_addcidrange(font->tounicode, cid, cid, ucs);
				if (error)
					goto cleanup;
			}
		}

		error = fz_endcidrange(font->tounicode);
		if (error)
			goto cleanup;

	cleanup:
		fz_dropcmap(cmap);
		return error;
	}

	else if (collection)
	{
		pdf_logfont("tounicode cid collection\n");

		if (!strcmp(collection, "Adobe-CNS1"))
			return pdf_loadsystemcmap(&font->tounicode, "Adobe-CNS1-UCS2");
		else if (!strcmp(collection, "Adobe-GB1"))
			return pdf_loadsystemcmap(&font->tounicode, "Adobe-GB1-UCS2");
		else if (!strcmp(collection, "Adobe-Japan1"))
			return pdf_loadsystemcmap(&font->tounicode, "Adobe-Japan1-UCS2");
		else if (!strcmp(collection, "Adobe-Japan2"))
			return pdf_loadsystemcmap(&font->tounicode, "Adobe-Japan2-UCS2");
		else if (!strcmp(collection, "Adobe-Korea1"))
			return pdf_loadsystemcmap(&font->tounicode, "Adobe-Korea1-UCS2");
	}

	if (strings)
	{
		pdf_logfont("tounicode strings\n");

		font->ncidtoucs = 256;
		font->cidtoucs = fz_malloc(256 * sizeof(unsigned short));
		if (!font->cidtoucs)
			return fz_outofmem;

		for (i = 0; i < 256; i++)
		{
			if (strings[i])
				font->cidtoucs[i] = pdf_lookupagl(strings[i]);
			else
				font->cidtoucs[i] = '?';
		}

		return nil;
	}

	pdf_logfont("tounicode impossible");
	return nil;
}

/*
 * Extract lines of text from display tree
 */

fz_error *
pdf_newtextline(pdf_textline **linep)
{
	pdf_textline *line;
	line = *linep = fz_malloc(sizeof(pdf_textline));
	if (!line)
		return fz_outofmem;
	line->height.x = 0;	/* bad default value... */
	line->height.y = 10;
	line->len = 0;
	line->cap = 0;
	line->text = nil;
	line->next = nil;
	return nil;
}

void
pdf_droptextline(pdf_textline *line)
{
	if (line->next)
		pdf_droptextline(line->next);
	fz_free(line->text);
	fz_free(line);
}

static fz_error *
addtextchar(pdf_textline *line, int x, int y, int c)
{
	pdf_textchar *newtext;
	int newcap;

	if (line->len + 1 >= line->cap)
	{
		newcap = line->cap ? line->cap * 2 : 80; 
		newtext = fz_realloc(line->text, sizeof(pdf_textchar) * newcap);
		if (!newtext)
			return fz_outofmem;
		line->cap = newcap;
		line->text = newtext;
	}

	line->text[line->len].x = x;
	line->text[line->len].y = y;
	line->text[line->len].c = c;
	line->len ++;

	return nil;
}

/* XXX global! not reentrant! */
static fz_point oldpt = { 0, 0 };

static fz_error *
extracttext(pdf_textline **line, fz_node *node, fz_matrix ctm)
{
	fz_error *error;

	if (fz_istextnode(node))
	{
		fz_textnode *text = (fz_textnode*)node;
		pdf_font *font = (pdf_font*)text->font;
		fz_matrix inv = fz_invertmatrix(text->trm);
		fz_matrix tm = text->trm;
		fz_matrix trm;
		float dx, dy, t;
		fz_point p;
		fz_vmtx v;
		fz_hmtx h;
		int i, g, x, y;
		int c;

		/* get line height */
		trm = fz_concat(tm, ctm);
		trm.e = 0;
		trm.f = 0;
		p.x = 0;
		p.y = 1;
		(*line)->height = fz_transformpoint(trm, p);

		for (i = 0; i < text->len; i++)
		{
			g = text->els[i].cid;

			tm.e = text->els[i].x;
			tm.f = text->els[i].y;
			trm = fz_concat(tm, ctm);
			x = fz_floor(trm.e);
			y = fz_floor(trm.f);

			p.x = text->els[i].x;
			p.y = text->els[i].y;
			p = fz_transformpoint(inv, p);
			dx = oldpt.x - p.x;
			dy = oldpt.y - p.y;
			oldpt = p;

			if (text->font->wmode == 0)
			{
				h = fz_gethmtx(text->font, g);
				oldpt.x += h.w * 0.001;
			}
			else
			{
				v = fz_getvmtx(text->font, g);
				oldpt.y += v.w;
				t = dy; dy = dx; dx = t;
			}

			if (fabs(dy) > 0.2)
			{
				pdf_textline *newline;
				error = pdf_newtextline(&newline);
				if (error)
					return error;
				(*line)->next = newline;
				*line = newline;
			}
			else if (fabs(dx) > 0.2)
			{
				error = addtextchar(*line, x, y, ' ');
				if (error)
					return error;
			}

			if (font->tounicode)
				c = fz_lookupcid(font->tounicode, g);
			else if (g < font->ncidtoucs)
				c = font->cidtoucs[g];
			else
				c = g;

			error = addtextchar(*line, x, y, c);
			if (error)
				return error;
		}
	}

	if (fz_istransformnode(node))
		ctm = fz_concat(((fz_transformnode*)node)->m, ctm);

	for (node = node->first; node; node = node->next)
	{
		error = extracttext(line, node, ctm);
		if (error)
			return error;
	}

	return nil;
}

fz_error *
pdf_loadtextfromtree(pdf_textline **outp, fz_tree *tree, fz_matrix ctm)
{
	pdf_textline *root;
	pdf_textline *line;
	fz_error *error;

	oldpt.x = -1;
	oldpt.y = -1;

	error = pdf_newtextline(&root);
	if (error)
		return error;

	line = root;

	error = extracttext(&line, tree->root, ctm);
	if (error)
	{
		pdf_droptextline(root);
		return error;
	}

	*outp = root;
	return nil;
}

void
pdf_debugtextline(pdf_textline *line)
{
	char buf[10];
	int c, n, k, i;

	for (i = 0; i < line->len; i++)
	{
		c = line->text[i].c;
		if (c < 128)
			putchar(c);
		else
		{
			n = runetochar(buf, &c);
			for (k = 0; k < n; k++)
				putchar(buf[k]);
		}
	}
	putchar('\n');

	if (line->next)
		pdf_debugtextline(line->next);
}

