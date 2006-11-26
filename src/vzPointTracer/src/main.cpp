#include <stdio.h>
#include <stdlib.h>

#include "vzImage.h"

//#define METHOD1
#define METHOD2

struct point_coord
{
	long x;
	long y;
	long v;
};

static void sort_points(struct point_coord* points, long points_count)
{
	int i,j, m;
	struct point_coord p;

	for(i = 0; i<points_count; i++)
	{
		for(m = i, j = i ; j<points_count; j++)
			if(points[j].v > points[m].v)
				m = j;

		if(m != i)
		{
			p = points[i];
			points[i] = points[m];
			points[m] = p;
		};
	};
};

static struct point_coord* find_points(long threshold, vzImage* image, long* found, 
	long* m_left, long* m_right, long *m_top, long *m_bottom)
{
	struct point_coord* coords = (struct point_coord*)malloc(0);
	int c = 0,i,j;
	unsigned long p;

	/* setup base (unreal) values of range vars */
	*m_left = 10000; *m_right = -10000; *m_top = 10000; *m_bottom = -10000;

	/* scan image */
	for(j = 0; j<image->height; j++)
		for(i = 0; i<image->width; i++)
		{
			/* find point */
			p = ((unsigned long*)image->surface)[j * image->width + i];

			/* check threshold */
			if ( (p & 0xFF) > ((unsigned long)threshold))
			{
				/* reallocate buf */
				coords = (struct point_coord*)realloc(coords,  sizeof(struct point_coord) * (c + 1));

				/* assign */
				coords[c].v = (p & 0xFF) - threshold;
				coords[c].x = i;
				coords[c].y = j;

				/* increment values */
				c++;

				/* fix ranges */
				if(j>*m_bottom) *m_bottom = j;
				if(j<*m_top) *m_top = j;
				if(i>*m_right) *m_right = i;
				if(i<*m_left) *m_left = i;
			}
		};

	/* output points */
	for(j = 0; j<c; j++)
		fprintf(stderr, "\t*[%d,%d] == %d\n", coords[j].x, coords[j].y, coords[j].v); 

	/* return results */
	*found = c;
	return coords;
};

static void usage()
{
	fprintf
	(
		stderr,
		"Usage:\n"
		"    vzPointTracer.exe <threshold> <translateX> <translateY> <start num> <stop num> <path template>\n"
		"Where:\n"
		"    <threshold>     - lower level of black point (0..255).\n"
		"    <translateX>    - translation coordane X ( returns x - translateX).\n"
		"    <translateY>    - translation coordane Y ( returns y - translateY).\n"
		"    <start num>     - start number of image file (first image).\n"
		"    <stop num>      - stop number of image file (last number).\n"
		"    <path template> - sprintf-like path to 32bit TGA files, e.g. 'c:/temp/seq/comp%%.5d.tga'\n"
//		,
	);

}

int main(int argc, char** argv)
{
//long points_count

/*

0 0 0 0 75 X:\tmp\2CH_DVE\Sfera\Sfera_%.4d.tga
0 0 0 1 5 c:/temp/seq/comp%.4d.tga

*/


	/* check args */
	if(argc != 7)
	{
		fprintf
		(
			stderr, 
			"ERROR! Not enough arguments.\n"
		);
		usage();
		exit(-1);
	};

	/* assign args */
	long
		threshold = atol(argv[1]),
		trans_x = atol(argv[2]),
		trans_y = atol(argv[3]),
		start_frame = atol(argv[4]),
		stop_frame = atol(argv[5]),
		i;
	char *path_fmt = argv[6], *e;
	char path_buf[1024];
	vzImage* image;

	double* points_x = (double*)malloc(0);
	double* points_y = (double*)malloc(0);
	long points_count = 0;

	/* enumerate all images */
	for(i = start_frame; i<=stop_frame; i++)
	{
		/* prepare file name */
		sprintf(path_buf, path_fmt, i);

		/* notify */
		fprintf(stderr, "%-5d : %s ", i - start_frame, path_buf);

		/* try to open file */
		if(NULL == (image = vzImageLoadTGA(path_buf, &e)))
		{
			/* notify */
			fprintf(stderr, "ERROR (%s)\n", e);
		}
		else
		{
			/* notify */
			fprintf(stderr, "OK (%d,%d)\n", image->width, image->height);


			/* do some job here */

			
			/* find points */
			long points_found, m_left, m_right, m_top, m_bottom;

			struct point_coord* points = find_points(threshold, image, &points_found, &m_left, &m_right, &m_top, &m_bottom);

			/* allocate space for points */
			points_x = (double*)realloc(points_x, sizeof(double) * (points_count + 1));
			points_y = (double*)realloc(points_y, sizeof(double) * (points_count + 1));

			if(points_found)
			{

#ifdef METHOD1
				/* sort items by value */
				sort_points(points, points_found);

				/* find points count in plato */
				int p = 1,j;
				for(j = 1;(points[j].v == points[0].v)&&(j<points_found); j++,p++);

				/* find midl arith */
				for(points_x[points_count] = 0.0, points_y[points_count] = 0.0, j = 0; j<p; j++)
				{
					points_x[points_count] += (double)points[j].x;
					points_y[points_count] += (double)points[j].y;
				};
				points_x[points_count] /= (double)p;
				points_y[points_count] /= (double)p;

#endif /* METHOD1 */


#ifdef METHOD2
				long k, j, m_prev, m_curr;
				double m = 0.0, m2 = 0.0;

				/* find mass of figure */
				for(j = 0; j< points_found; j++) m += (double)points[j].v;
				m2 = m / 2.0;

				/* find mass by row */
				for(j = m_left, m_prev = 0; j <= m_right; j++)
				{
					/* find mass in this row */
					for(k = 0, m_curr = 0; k < points_found; k++) 
						if(points[k].x == j)
							m_curr += points[k].v;

					/* check if found center */
					if
					(
						( ((double)(m_prev + m_curr)) >= m2 )
						&&
						( ((double)(m_prev )) <= m2 )
					)
					{
						/* center found - calc offset*/
						points_x[points_count] = (double)j + (m2 - m_prev)/( (double)m_curr );
						break;
					};

					m_prev += m_curr;
				};


				/* find mass by col */
				for(j = m_top, m_prev = 0; j <= m_bottom; j++)
				{
					/* find mass in this row */
					for(k = 0, m_curr = 0; k < points_found; k++) 
						if(points[k].y == j)
							m_curr += points[k].v;

					/* check if found center */
					if
					(
						( ((double)(m_prev + m_curr)) >= m2 )
						&&
						( ((double)(m_prev )) <= m2 )
					)
					{
						/* center found - calc offset*/
						points_y[points_count] = (double)j + (m2 - m_prev)/( (double)m_curr );
						break;
					};

					m_prev += m_curr;
				};
#endif /* METHOD2 */


				/* putput in image coordinates */
				fprintf(stderr, "\t=[%7.3lf, %7.3lf]\n", points_x[points_count], points_y[points_count]);

				/* translate */
				points_x[points_count] = points_x[points_count] - (double)trans_x;
				points_y[points_count] = (double)image->height - points_y[points_count] - (double)trans_y;
			}
			else
			{
				fprintf(stderr, "NOT POINTS FOUND!!!!!\n");

				/* setup fake values */
				points_x[points_count] = 10000.0;
				points_y[points_count] = 10000.0;
			};


			/* inc */
			points_count++;

			free(points);

			/* free image object */
			vzImageFree(image);
		};
	};


	/* traslate and filter found points */
	if(points_count)
	{
		
		/* filter */
		for(i = 0; i<points_count; i++)
		{
			/* try to linear approximate not found point */
			if
			(
				( i )
				&&
				( (i + 1) < points_count )
				&&
				( 10000.0 == points_x[i])
				&&
				( 10000.0 == points_y[i])
			)
			{
				points_y[i] = (points_y[i-1] + points_y[i+1])/2.0;
				points_x[i] = (points_x[i-1] + points_x[i+1])/2.0;
			};
		};

		/* output */
		for(i = 0; i<points_count; i++)
		{
			fprintf(stdout, "%7.3lf\t%7.3lf\n", points_x[i], points_y[i]);
		};
		
	};

	return 0;
};
