#include <fitz.h>
#include <mupdf.h>

static inline int getbit(const unsigned char *buf, int x)
{
	return ( buf[x >> 3] >> ( 7 - (x & 7) ) ) & 1;
}

static void loadtile1(pdf_image *src, fz_pixmap *dst)
{
	int x, y, k;
	int n = dst->n;
	for (y = dst->y; y < dst->y + dst->h; y++)
	{
		unsigned char *s = src->samples->bp + y * src->stride;
		unsigned char *d = dst->samples + y * dst->w * dst->n;
		for (x = dst->x; x < dst->x + dst->w; x++)
		{
			for (k = 0; k < n; k++)
				d[x * n + k] = getbit(s, x * n + k);
		}
	}
}

static void loadtile1a(pdf_image *src, fz_pixmap *dst)
{
	int x, y, k;
	int sn = src->super.n;
	int dn = dst->n;
	for (y = dst->y; y < dst->y + dst->h; y++)
	{
		unsigned char *s = src->samples->bp + y * src->stride;
		unsigned char *d = dst->samples + y * dst->w * dst->n;
		for (x = dst->x; x < dst->x + dst->w; x++)
		{
			d[x * dn + 0] = 255;
			for (k = 0; k < sn; k++)
				d[x * dn + k + 1] = getbit(s, x * sn + k);
		}
	}
}

static void loadtile8(pdf_image *src, fz_pixmap *dst)
{
	int x, y, k;
	int n = src->super.n;
	for (y = dst->y; y < dst->y + dst->h; y++)
	{
		unsigned char *s = src->samples->bp + y * src->stride;
		unsigned char *d = dst->samples + y * dst->w * dst->n;
		for (x = dst->x; x < dst->x + dst->w; x++)
			for (k = 0; k < n; k++)
				*d++ = *s++;
	}
}

static void loadtile8a(pdf_image *src, fz_pixmap *dst)
{
	int x, y, k;
	int n = dst->n;
	for (y = dst->y; y < dst->y + dst->h; y++)
	{
		unsigned char *s = src->samples->bp + y * src->stride;
		unsigned char *d = dst->samples + y * dst->w * dst->n;
		for (x = dst->x; x < dst->x + dst->w; x++)
		{
			*d++ = 255;
			for (k = 1; k < n; k++)
				*d++ = *s++;
		}
	}
}

static void
decodetile(fz_pixmap *pix, int bpc, int a, float *decode, int scale)
{
	unsigned char table[32][256];
	float twon = (1 << bpc) - 1;
	int x, y, k, i;

	for (i = 0; i < (1 << bpc); i++)
	{
		for (k = 0; k < pix->n - a; k++)
		{
			float min = decode[k * 2 + 0];
			float max = decode[k * 2 + 1];
			float f = min + i * (max - min) / twon;
			table[k][i] = f * scale;
		}
	}

	for (y = 0; y < pix->h; y++)
	{
		for (x = 0; x < pix->w; x++)
		{
			for (k = a; k < pix->n; k++)
			{
				i = pix->samples[ (y * pix->w + x) * pix->n + k];
				pix->samples[ (y * pix->w + x) * pix->n + k] = table[k-a][i];
			}
		}
	}
}

static fz_error *
loadtile(fz_image *img, fz_pixmap *tile)
{
	pdf_image *src = (pdf_image*)img;

	assert(tile->n == img->n + 1);
	assert(tile->x >= 0);
	assert(tile->y >= 0);
	assert(tile->x + tile->w <= img->w);
	assert(tile->y + tile->h <= img->h);

	if (img->a)
	{
		switch (src->bpc)
		{
		case 1: loadtile1(src, tile); break;
	//	case 2: loadtile2(src, tile); break;
	//	case 4: loadtile4(src, tile); break;
		case 8: loadtile8(src, tile); break;
		default:
			return fz_throw("rangecheck: unsupported bit depth: %d", src->bpc);
		}
	}
	else
	{
		switch (src->bpc)
		{
		case 1: loadtile1a(src, tile); break;
	//	case 2: loadtile2a(src, tile); break;
	//	case 4: loadtile4a(src, tile); break;
		case 8: loadtile8a(src, tile); break;
		default:
			return fz_throw("rangecheck: unsupported bit depth: %d", src->bpc);
		}
	}

	if (img->cs && !strcmp(img->cs->name, "Indexed"))
		decodetile(tile, src->bpc, !src->super.a, src->decode, 1);
	else
		decodetile(tile, src->bpc, !src->super.a, src->decode, 255);

	return nil;
}

fz_error *
pdf_loadimage(pdf_image **imgp, pdf_xref *xref, fz_obj *dict, fz_obj *ref)
{
	fz_error *error;
	pdf_image *img;
	fz_colorspace *cs;
	int ismask;
	fz_obj *obj;
	int i;

	img = fz_malloc(sizeof(pdf_image));
	if (!img)
		return fz_outofmem;

	img->super.loadtile = loadtile;
	img->super.free = nil;
	img->super.cs = nil;

	img->super.w = fz_toint(fz_dictgets(dict, "Width"));
	img->super.h = fz_toint(fz_dictgets(dict, "Height"));
	img->bpc = fz_toint(fz_dictgets(dict, "BitsPerComponent"));

printf("load image %d x %d @ %d\n", img->super.w, img->super.h, img->bpc);

	cs = nil;
	obj = fz_dictgets(dict, "ColorSpace");
	if (obj)
	{
		error = pdf_resolve(&obj, xref);
		if (error)
			return error;

		error = pdf_loadcolorspace(&cs, xref, obj);
		if (error)
			return error;

		fz_dropobj(obj);
	}

	ismask = fz_tobool(fz_dictgets(dict, "ImageMask"));
	if (!ismask)
	{
		img->super.cs = cs;
		img->super.n = cs->n;
		img->super.a = 0;
	}
	else
	{
		img->super.cs = nil;
		img->super.n = 0;
		img->super.a = 1;
	}

	img->stride = ((img->super.w * (img->super.n + img->super.a)) * img->bpc + 7) / 8;

	obj = fz_dictgets(dict, "Decode");
	if (fz_isarray(obj))
		for (i = 0; i < (img->super.n + img->super.a) * 2; i++)
			img->decode[i] = fz_toreal(fz_arrayget(obj, i));
	else
	{
		if (cs && !strcmp(cs->name, "IndexedXXX"))
			for (i = 0; i < (img->super.n + img->super.a) * 2; i++)
				img->decode[i] = i & 1 ? (1 << img->bpc) - 1 : 0;
		else
			for (i = 0; i < (img->super.n + img->super.a) * 2; i++)
				img->decode[i] = i & 1;
	}

printf("  colorspace %s\n", cs ? cs->name : "(null)");
printf("  mask %d\n", ismask);
printf("  decode [ ");
for (i = 0; i < (img->super.n + img->super.a) * 2; i++)
printf("%g ", img->decode[i]);
printf("]\n");

	error = pdf_loadstream(&img->samples, xref, fz_tonum(ref), fz_togen(ref));
	if (error)
	{
		/* TODO: colorspace? */
		fz_free(img);
		return error;
	}

	if (img->samples->wp - img->samples->bp != img->stride * img->super.h)
	{
		/* TODO: colorspace? */
		fz_freebuffer(img->samples);
		fz_free(img);
		return fz_throw("syntaxerror: truncated image data");
	}

	/* 0 means opaque and 1 means transparent, so we invert to get alpha */
	if (ismask)
	{
		unsigned char *p;
		for (p = img->samples->bp; p < img->samples->ep; p++)
			*p = ~*p;
	}

	*imgp = img;

	return nil;
}

