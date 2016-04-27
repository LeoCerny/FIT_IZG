/******************************************************************************
 * Projekt - Zaklady pocitacove grafiky - IZG
 * spanel@fit.vutbr.cz
 *
 * $Id:$
 */

#include "student.h"
#include "transform.h"
#include "fragment.h"

#include <memory.h>
#include <math.h>
float myStudFrame = 0.0;

const S_Material	MAT_WHITE_AMBIENT = { 1.0, 1.0, 1.0, 1.0 };
const S_Material	MAT_WHITE_DIFFUSE = { 1.0, 1.0, 1.0, 1.0 };
const S_Material	MAT_WHITE_SPECULAR = { 1.0, 1.0, 1.0, 1.0 };

/*****************************************************************************
 * Globalni promenne a konstanty
 */


/* Typ/ID rendereru (nemenit) */
const int           STUDENT_RENDERER = 1;


/*****************************************************************************
 * Funkce vytvori vas renderer a nainicializuje jej
 */

S_Renderer * studrenCreate()
{
    S_StudentRenderer * renderer = (S_StudentRenderer *)malloc(sizeof(S_StudentRenderer));
    IZG_CHECK(renderer, "Cannot allocate enough memory");

    /* inicializace default rendereru */
    renderer->base.type = STUDENT_RENDERER;
    renInit(&renderer->base);

    /* nastaveni ukazatelu na upravene funkce */
    /* napr. renderer->base.releaseFunc = studrenRelease; */
    renderer->base.releaseFunc = studrenRelease;
    renderer->base.projectTriangleFunc = studrenProjectTriangle;

    /* inicializace nove pridanych casti */

    int width, height;
    S_RGBA *texture = loadBitmap(TEXTURE_FILENAME, &width, &height);

    renderer->height = height;
    renderer->width = width;
    renderer->texture = texture;
    return (S_Renderer *)renderer;
}

/*****************************************************************************
 * Funkce korektne zrusi renderer a uvolni pamet
 */

void studrenRelease(S_Renderer **ppRenderer)
{
    S_StudentRenderer * renderer;

    if( ppRenderer && *ppRenderer )
    {
        /* ukazatel na studentsky renderer */
        renderer = (S_StudentRenderer *)(*ppRenderer);

        /* pripadne uvolneni pameti */
        free(renderer->texture);
        
        /* fce default rendereru */
        renRelease(ppRenderer);
    }
}

/******************************************************************************
 * Nova fce pro rasterizaci trojuhelniku s podporou texturovani
 * Upravte tak, aby se trojuhelnik kreslil s texturami
 * (doplnte i potrebne parametry funkce - texturovaci souradnice, ...)
 * v1, v2, v3 - ukazatele na vrcholy trojuhelniku ve 3D pred projekci
 * n1, n2, n3 - ukazatele na normaly ve vrcholech ve 3D pred projekci
 * t0, t1, t2 - souradnice textury
 * x1, y1, ... - vrcholy trojuhelniku po projekci do roviny obrazovky
 */

void studrenDrawTriangle(S_Renderer *pRenderer,
                         S_Coords *v1, S_Coords *v2, S_Coords *v3,
                         S_Coords *n1, S_Coords *n2, S_Coords *n3,
						 S_Coords *t0, S_Coords *t1, S_Coords *t2,
                         int x1, int y1,
                         int x2, int y2,
                         int x3, int y3
                         )
{

	int         minx, miny, maxx, maxy;
	int         a1, a2, a3, b1, b2, b3, c1, c2, c3;
	int         s1, s2, s3;
	int         x, y, e1, e2, e3;
	double      alpha, beta, gamma, w1, w2, w3, z, u, v;
	S_RGBA      col1, col2, col3, color;

	IZG_ASSERT(pRenderer && v1 && v2 && v3 && n1 && n2 && n3);

	/* vypocet barev ve vrcholech */
	col1 = pRenderer->calcReflectanceFunc(pRenderer, v1, n1);
	col2 = pRenderer->calcReflectanceFunc(pRenderer, v2, n2);
	col3 = pRenderer->calcReflectanceFunc(pRenderer, v3, n3);

	/* obalka trojuhleniku */
	minx = MIN(x1, MIN(x2, x3));
	maxx = MAX(x1, MAX(x2, x3));
	miny = MIN(y1, MIN(y2, y3));
	maxy = MAX(y1, MAX(y2, y3));

	/* oriznuti podle rozmeru okna */
	miny = MAX(miny, 0);
	maxy = MIN(maxy, pRenderer->frame_h - 1);
	minx = MAX(minx, 0);
	maxx = MIN(maxx, pRenderer->frame_w - 1);

	/* Pineduv alg. rasterizace troj.
	   hranova fce je obecna rovnice primky Ax + By + C = 0
	   primku prochazejici body (x1, y1) a (x2, y2) urcime jako
	   (y1 - y2)x + (x2 - x1)y + x1y2 - x2y1 = 0 */

	/* normala primek - vektor kolmy k vektoru mezi dvema vrcholy, tedy (-dy, dx) */
	a1 = y1 - y2;
	a2 = y2 - y3;
	a3 = y3 - y1;
	b1 = x2 - x1;
	b2 = x3 - x2;
	b3 = x1 - x3;

	/* koeficient C */
	c1 = x1 * y2 - x2 * y1;
	c2 = x2 * y3 - x3 * y2;
	c3 = x3 * y1 - x1 * y3;

	/* vypocet hranove fce (vzdalenost od primky) pro protejsi body */
	s1 = a1 * x3 + b1 * y3 + c1;
	s2 = a2 * x1 + b2 * y1 + c2;
	s3 = a3 * x2 + b3 * y2 + c3;

	if ( !s1 || !s2 || !s3 )
	{
		return;
	}

	/* normalizace, aby vzdalenost od primky byla kladna uvnitr trojuhelniku */
	if( s1 < 0 )
	{
		a1 *= -1;
		b1 *= -1;
		c1 *= -1;
	}
	if( s2 < 0 )
	{
		a2 *= -1;
		b2 *= -1;
		c2 *= -1;
	}
	if( s3 < 0 )
	{
		a3 *= -1;
		b3 *= -1;
		c3 *= -1;
	}

	/* koeficienty pro barycentricke souradnice */
	alpha = 1.0 / ABS(s2);
	beta = 1.0 / ABS(s3);
	gamma = 1.0 / ABS(s1);

	S_RGBA newColor;
	/* vyplnovani... */
	for( y = miny; y <= maxy; ++y )
	{
		/* inicilizace hranove fce v bode (minx, y) */
		e1 = a1 * minx + b1 * y + c1;
		e2 = a2 * minx + b2 * y + c2;
		e3 = a3 * minx + b3 * y + c3;

		for( x = minx; x <= maxx; ++x )
		{
			if( e1 >= 0 && e2 >= 0 && e3 >= 0 )
			{
				/* interpolace pomoci barycentrickych souradnic
				   e1, e2, e3 je aktualni vzdalenost bodu (x, y) od primek */
				w1 = alpha * e2;
				w2 = beta * e3;
				w3 = gamma * e1;

				/* interpolace z-souradnice */
				z = w1 * v1->z + w2 * v2->z + w3 * v3->z;
				u = w1 * t0->x + w2 * t1->x + w3 * t2->x;
				v = w1 * t0->y + w2 * t1->y + w3 * t2->y;

				newColor = studrenTextureValue((S_StudentRenderer *)pRenderer, u, v);


				/* interpolace barvy */
				color.red = ROUND2BYTE(w1 * col1.red + w2 * col2.red + w3 * col3.red) * (newColor.red / 255.0);
				color.green = ROUND2BYTE(w1 * col1.green + w2 * col2.green + w3 * col3.green) * (newColor.green / 255.0);
				color.blue = ROUND2BYTE(w1 * col1.blue + w2 * col2.blue + w3 * col3.blue) * (newColor.blue / 255.0);
				color.alpha = 255;

				/* vykresleni bodu */
				if( z < DEPTH(pRenderer, x, y) )
				{
					PIXEL(pRenderer, x, y) = color;
					DEPTH(pRenderer, x, y) = z;
				}
			}

			/* hranova fce o pixel vedle */
			e1 += a1;
			e2 += a2;
			e3 += a3;
		}
	}
}

/******************************************************************************
 * Vykresli i-ty trojuhelnik n-teho klicoveho snimku modelu
 * pomoci nove fce studrenDrawTriangle()
 * Pred vykreslenim aplikuje na vrcholy a normaly trojuhelniku
 * aktualne nastavene transformacni matice!
 * Upravte tak, aby se model vykreslil interpolovane dle parametru n
 * (cela cast n udava klicovy snimek, desetinna cast n parametr interpolace
 * mezi snimkem n a n + 1)
 * i - index trojuhelniku
 * n - index klicoveho snimku (float pro pozdejsi interpolaci mezi snimky)
 */

void studrenProjectTriangle(S_Renderer *pRenderer, S_Model *pModel, int i, float n)
{
    S_Coords    aa, bb, cc;             /* souradnice vrcholu po transformaci */
    S_Coords    naa, nbb, ncc;          /* normaly ve vrcholech po transformaci */
    S_Coords    nn;                     /* normala trojuhelniku po transformaci */
    int         u1, v1, u2, v2, u3, v3; /* souradnice vrcholu po projekci do roviny obrazovky */
    S_Triangle  * triangle;
    int         vertexOffset, normalOffset; /* offset pro vrcholy a normalove vektory trojuhelniku */
    int         i0, i1, i2, in;             /* indexy vrcholu a normaly pro i-ty trojuhelnik n-teho snimku */
    S_Coords    aa1, bb1, cc1;             /* souradnice vrcholu po transformaci */
    S_Coords    naa1, nbb1, ncc1;          /* normaly ve vrcholech po transformaci */
    S_Coords    nn1;                     /* normala trojuhelniku po transformaci */
    int vertexOffset1, normalOffset1;
    int i01, i11, i21, in1;				/* indexy vrcholu a normaly pro i-ty trojuhelnik (n+1)-teho snimku */
    IZG_ASSERT(pRenderer && pModel && i >= 0 && i < trivecSize(pModel->triangles) && n >= 0 );

    int N = (int) floor(n);
    float xN = n - floor(n);

    /* z modelu si vytahneme i-ty trojuhelnik */
    triangle = trivecGetPtr(pModel->triangles, i);

    /* ziskame offset pro vrcholy n-teho snimku */
    vertexOffset = (((int) n) % pModel->frames) * pModel->verticesPerFrame;
    vertexOffset1 = (((int) (n + 1.0)) % pModel->frames) * pModel->verticesPerFrame;
    /* ziskame offset pro normaly trojuhelniku n-teho snimku */
    normalOffset = (((int) n) % pModel->frames) * pModel->triangles->size;
    normalOffset1 = (((int) (n + 1.0)) % pModel->frames) * pModel->triangles->size;

    /* indexy vrcholu pro i-ty trojuhelnik n-teho snimku - pricteni offsetu */
    i0 = triangle->v[ 0 ] + vertexOffset;
    i1 = triangle->v[ 1 ] + vertexOffset;
    i2 = triangle->v[ 2 ] + vertexOffset;

    i01 = triangle->v[ 0 ] + vertexOffset1;
	i11 = triangle->v[ 1 ] + vertexOffset1;
	i21 = triangle->v[ 2 ] + vertexOffset1;

    /* index normaloveho vektoru pro i-ty trojuhelnik n-teho snimku - pricteni offsetu */
    in = triangle->n + normalOffset;
    in1 = triangle->n + normalOffset1;

    /* transformace vrcholu matici model */
    trTransformVertex(&aa, cvecGetPtr(pModel->vertices, i0));
    trTransformVertex(&bb, cvecGetPtr(pModel->vertices, i1));
    trTransformVertex(&cc, cvecGetPtr(pModel->vertices, i2));
    trTransformVertex(&aa1, cvecGetPtr(pModel->vertices, i01));
	trTransformVertex(&bb1, cvecGetPtr(pModel->vertices, i11));
	trTransformVertex(&cc1, cvecGetPtr(pModel->vertices, i21));

	/* Interpolace vrcholu*/
	aa.x = (1 - xN) * aa.x + xN * aa1.x;
	aa.y = (1 - xN) * aa.y + xN * aa1.y;
	aa.z = (1 - xN) * aa.z + xN * aa1.z;

	bb.x = (1 - xN) * bb.x + xN * bb1.x;
	bb.y = (1 - xN) * bb.y + xN * bb1.y;
	bb.z = (1 - xN) * bb.z + xN * bb1.z;

	cc.x = (1 - xN) * cc.x + xN * cc1.x;
	cc.y = (1 - xN) * cc.y + xN * cc1.y;
	cc.z = (1 - xN) * cc.z + xN * cc1.z;

    /* promitneme vrcholy trojuhelniku na obrazovku */
    double h0 = trProjectVertex(&u1, &v1, &aa);
    double h1 = trProjectVertex(&u2, &v2, &bb);
    double h2 = trProjectVertex(&u3, &v3, &cc);

    /* pro osvetlovaci model transformujeme take normaly ve vrcholech */
    trTransformVector(&naa, cvecGetPtr(pModel->normals, i0));
    trTransformVector(&nbb, cvecGetPtr(pModel->normals, i1));
    trTransformVector(&ncc, cvecGetPtr(pModel->normals, i2));
    trTransformVector(&naa1, cvecGetPtr(pModel->normals, i01));
	trTransformVector(&nbb1, cvecGetPtr(pModel->normals, i11));
	trTransformVector(&ncc1, cvecGetPtr(pModel->normals, i21));

	/* Interpolace normal */
	naa.x = (1 - xN) * naa.x + xN * naa1.x;
	naa.y = (1 - xN) * naa.y + xN * naa1.y;
	naa.z = (1 - xN) * naa.z + xN * naa1.z;

	nbb.x = (1 - xN) * nbb.x + xN * nbb1.x;
	nbb.y = (1 - xN) * nbb.y + xN * nbb1.y;
	nbb.z = (1 - xN) * nbb.z + xN * nbb1.z;

	ncc.x = (1 - xN) * ncc.x + xN * ncc1.x;
	ncc.y = (1 - xN) * ncc.y + xN * ncc1.y;
	ncc.z = (1 - xN) * ncc.z + xN * ncc1.z;

    /* normalizace normal */
    coordsNormalize(&naa);
    coordsNormalize(&nbb);
    coordsNormalize(&ncc);

    /* transformace normaly trojuhelniku matici model */
    trTransformVector(&nn, cvecGetPtr(pModel->trinormals, in));
    trTransformVector(&nn1, cvecGetPtr(pModel->trinormals, in1));

    /* Interpolace normaly */
    nn.x = (1 - xN) * nn.x + xN * nn1.x;
    nn.y = (1 - xN) * nn.y + xN * nn1.y;
    nn.z = (1 - xN) * nn.z + xN * nn1.z;


    /* normalizace normaly */
    coordsNormalize(&nn);

    /* je troj. privraceny ke kamere, tudiz viditelny? */
    if( !renCalcVisibility(pRenderer, &aa, &nn) )
    {
        /* odvracene troj. vubec nekreslime */
        return;
    }
    S_Triangle *texture = trivecGetPtr(pModel->triangles, i);

    /* rasterizace trojuhelniku */
    studrenDrawTriangle(pRenderer,
                    &aa, &bb, &cc,
                    &naa, &nbb, &ncc,
					&triangle->t[0], &triangle->t[1], &triangle->t[2],
                    u1, v1, u2, v2, u3, v3,
					h0, h1, h2
                    );
}

/******************************************************************************
* Vraci hodnotu v aktualne nastavene texture na zadanych
* texturovacich souradnicich u, v
* Pro urceni hodnoty pouziva bilinearni interpolaci
* Pro otestovani vraci ve vychozim stavu barevnou sachovnici dle uv souradnic
* u, v - texturovaci souradnice v intervalu 0..1, ktery odpovida sirce/vysce textury
*/

S_RGBA studrenTextureValue( S_StudentRenderer * pRenderer, double u, double v )
{
	u = u * pRenderer->width;
	v = v * pRenderer->height;

	int U = (int) floor(u);
	int V = (int) floor(v);
	double xU = u - floor(u);
	double xV = u - floor(u);
	S_RGBA color;
    if (!isnan(u) && !isnan(v)){
    	color.red = pRenderer->texture[U * pRenderer->height + V].red * (1 - xU) * (1 - xV)
    	    			+ pRenderer->texture[U * pRenderer->height + (V + 1)].red * (1 - xU) * xV
    					+ pRenderer->texture[(U + 1) * pRenderer->height + V].red * xU * (1 - xV)
    					+ pRenderer->texture[(U + 1) * pRenderer->height + (V + 1)].red * xU * xV;
    	color.green = pRenderer->texture[U * pRenderer->height + V].green * (1 - xU) * (1 - xV)
						+ pRenderer->texture[U * pRenderer->height + (V + 1)].green * (1 - xU) * xV
						+ pRenderer->texture[(U + 1) * pRenderer->height + V].green * xU * (1 - xV)
						+ pRenderer->texture[(U + 1) * pRenderer->height + (V + 1)].green * xU * xV;
    	color.blue = pRenderer->texture[U * pRenderer->height + V].blue * (1 - xU) * (1 - xV)
						+ pRenderer->texture[U * pRenderer->height + (V + 1)].blue * (1 - xU) * xV
						+ pRenderer->texture[(U + 1) * pRenderer->height + V].blue * xU * (1 - xV)
						+ pRenderer->texture[(U + 1) * pRenderer->height + (V + 1)].blue * xU * xV;
    } else {
    	color.red = 90;
    	color.blue = 60;
    	color.green = 90;
    }
    return (makeColor(color.red, color.green, color.blue));
}

/******************************************************************************
 ******************************************************************************
 * Funkce pro vyrenderovani sceny, tj. vykresleni modelu
 * Upravte tak, aby se model vykreslil animovane
 * (volani renderModel s aktualizovanym parametrem n)
 */

void renderStudentScene(S_Renderer *pRenderer, S_Model *pModel)
{
	/* test existence frame bufferu a modelu */
	IZG_ASSERT(pModel && pRenderer);

	/* nastavit projekcni matici */
	trProjectionPerspective(pRenderer->camera_dist, pRenderer->frame_w, pRenderer->frame_h);

	/* vycistit model matici */
	trLoadIdentity();

	/* nejprve nastavime posuv cele sceny od/ke kamere */
	trTranslate(0.0, 0.0, pRenderer->scene_move_z);

	/* nejprve nastavime posuv cele sceny v rovine XY */
	trTranslate(pRenderer->scene_move_x, pRenderer->scene_move_y, 0.0);

	/* natoceni cele sceny - jen ve dvou smerech - mys je jen 2D... :( */
	trRotateX(pRenderer->scene_rot_x);
	trRotateY(pRenderer->scene_rot_y);

	/* nastavime material */
	/*
	renMatAmbient(pRenderer, &MAT_RED_AMBIENT);
	renMatDiffuse(pRenderer, &MAT_RED_DIFFUSE);
	renMatSpecular(pRenderer, &MAT_RED_SPECULAR);
	//*/
	renMatAmbient(pRenderer, &MAT_WHITE_AMBIENT);
	renMatDiffuse(pRenderer, &MAT_WHITE_DIFFUSE);
	renMatSpecular(pRenderer, &MAT_WHITE_SPECULAR);

	/* a vykreslime nas model (ve vychozim stavu kreslime pouze snimek 0) */
	renderModel(pRenderer, pModel, myStudFrame);
}

/* Callback funkce volana pri tiknuti casovace
 * ticks - pocet milisekund od inicializace */
void onTimer( int ticks )
{
	/* uprava parametru pouzivaneho pro vyber klicoveho snimku
     * a pro interpolaci mezi snimky */
	myStudFrame = (float)ticks / 99.0;
}

/*****************************************************************************
 *****************************************************************************/
