#include <fitz.h>

void fz_freemetanode(fz_metanode* node);
void fz_freelinknode(fz_linknode* node);
void fz_freepathnode(fz_pathnode* node);
void fz_freetextnode(fz_textnode* node);
void fz_freeimagenode(fz_imagenode* node);

fz_rect fz_boundtransformnode(fz_transformnode* node, fz_matrix ctm);
fz_rect fz_boundovernode(fz_overnode* node, fz_matrix ctm);
fz_rect fz_boundmasknode(fz_masknode* node, fz_matrix ctm);
fz_rect fz_boundblendnode(fz_blendnode* node, fz_matrix ctm);
fz_rect fz_boundcolornode(fz_colornode* node, fz_matrix ctm);
fz_rect fz_boundpathnode(fz_pathnode* node, fz_matrix ctm);
fz_rect fz_boundtextnode(fz_textnode* node, fz_matrix ctm);
fz_rect fz_boundimagenode(fz_imagenode* node, fz_matrix ctm);
fz_rect fz_boundshadenode(fz_shadenode* node, fz_matrix ctm);
fz_rect fz_boundlinknode(fz_linknode* node, fz_matrix ctm);
fz_rect fz_boundmetanode(fz_metanode* node, fz_matrix ctm);

void
fz_initnode(fz_node *node, fz_nodekind kind)
{
	node->kind = kind;
	node->parent = nil;
	node->child = nil;
	node->next = nil;
}

void
fz_freenode(fz_node *node)
{
	if (node->child)
		fz_freenode(node->child);
	if (node->next)
		fz_freenode(node->next);

	switch (node->kind)
	{
	case FZ_NTRANSFORM:
	case FZ_NOVER:
	case FZ_NMASK:
	case FZ_NBLEND:
	case FZ_NCOLOR:
		break;
	case FZ_NPATH:
		fz_freepathnode((fz_pathnode *) node);
		break;
	case FZ_NTEXT:
		fz_freetextnode((fz_textnode *) node);
		break;
	case FZ_NIMAGE:
		fz_freeimagenode((fz_imagenode *) node);
		break;
	case FZ_NSHADE:
		// XXX fz_freeshadenode((fz_shadenode *) node);
		break;
	case FZ_NLINK:
		fz_freelinknode((fz_linknode *) node);
		break;
	case FZ_NMETA:
		fz_freemetanode((fz_metanode *) node);
		break;
	}

	fz_free(node);
}

fz_rect
fz_boundnode(fz_node *node, fz_matrix ctm)
{
	switch (node->kind)
	{
	case FZ_NTRANSFORM:
		return fz_boundtransformnode((fz_transformnode *) node, ctm);
	case FZ_NOVER:
		return fz_boundovernode((fz_overnode *) node, ctm);
	case FZ_NMASK:
		return fz_boundmasknode((fz_masknode *) node, ctm);
	case FZ_NBLEND:
		return fz_boundblendnode((fz_blendnode *) node, ctm);
	case FZ_NCOLOR:
		return fz_boundcolornode((fz_colornode *) node, ctm);
	case FZ_NPATH:
		return fz_boundpathnode((fz_pathnode *) node, ctm);
	case FZ_NTEXT:
		return fz_boundtextnode((fz_textnode *) node, ctm);
	case FZ_NIMAGE:
		return fz_boundimagenode((fz_imagenode *) node, ctm);
	case FZ_NSHADE:
		// XXX return fz_boundshadenode((fz_shadenode *) node, ctm);
	case FZ_NLINK:
		return fz_boundlinknode((fz_linknode *) node, ctm);
	case FZ_NMETA:
		return fz_boundmetanode((fz_metanode *) node, ctm);
	}
	return FZ_INFRECT;
}

int
fz_istransformnode(fz_node *node)
{
	return node ? node->kind == FZ_NTRANSFORM : 0;
}

int
fz_isovernode(fz_node *node)
{
	return node ? node->kind == FZ_NOVER : 0;
}

int
fz_ismasknode(fz_node *node)
{
	return node ? node->kind == FZ_NMASK : 0;
}

int
fz_isblendnode(fz_node *node)
{
	return node ? node->kind == FZ_NBLEND : 0;
}

int
fz_iscolornode(fz_node *node)
{
	return node ? node->kind == FZ_NCOLOR : 0;
}

int
fz_ispathnode(fz_node *node)
{
	return node ? node->kind == FZ_NPATH : 0;
}

int
fz_istextnode(fz_node *node)
{
	return node ? node->kind == FZ_NTEXT : 0;
}

int
fz_isimagenode(fz_node *node)
{
	return node ? node->kind == FZ_NIMAGE : 0;
}

int
fz_isshadenode(fz_node *node)
{
	return node ? node->kind == FZ_NSHADE : 0;
}

int
fz_islinknode(fz_node *node)
{
	return node ? node->kind == FZ_NLINK : 0;
}

int
fz_ismetanode(fz_node *node)
{
	return node ? node->kind == FZ_NMETA : 0;
}

