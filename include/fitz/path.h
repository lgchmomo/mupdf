typedef enum fz_pathkind_e fz_pathkind;
typedef enum fz_pathelkind_e fz_pathelkind;
typedef struct fz_stroke_s fz_stroke;
typedef struct fz_dash_s fz_dash;
typedef union fz_pathel_s fz_pathel;

enum fz_pathkind_e { FZ_STROKE, FZ_FILL, FZ_EOFILL };
enum fz_pathelkind_e { FZ_MOVETO, FZ_LINETO, FZ_CURVETO, FZ_CLOSEPATH };

struct fz_stroke_s
{
	int linecap;
	int linejoin;
	float linewidth;
	float miterlimit;
};

struct fz_dash_s
{
	int len;
	float phase;
	float array[];
};

union fz_pathel_s
{
	fz_pathelkind k;
	float v;
};

struct fz_pathnode_s
{
	fz_node super;
	fz_pathkind paint;
	fz_dash *dash;
	int linecap;
	int linejoin;
	float linewidth;
	float miterlimit;
	int len, cap;
	fz_pathel *els;
};

fz_error *fz_newpathnode(fz_pathnode **pathp);
fz_error *fz_clonepath(fz_pathnode **pathp, fz_pathnode *oldpath);
fz_error *fz_moveto(fz_pathnode*, float x, float y);
fz_error *fz_lineto(fz_pathnode*, float x, float y);
fz_error *fz_curveto(fz_pathnode*, float, float, float, float, float, float);
fz_error *fz_curvetov(fz_pathnode*, float, float, float, float);
fz_error *fz_curvetoy(fz_pathnode*, float, float, float, float);
fz_error *fz_closepath(fz_pathnode*);
fz_error *fz_endpath(fz_pathnode*, fz_pathkind paint, fz_stroke *stroke, fz_dash *dash);

fz_rect fz_boundpathnode(fz_pathnode *node, fz_matrix ctm);
void fz_debugpathnode(fz_pathnode *node);

fz_error *fz_newdash(fz_dash **dashp, float phase, int len, float *array);
void fz_freedash(fz_dash *dash);

