#include <stdio.h>
#include <stdlib.h>

#include "vzImage.h"

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

static struct point_coord* find_points(long threshold, vzImage* image, long* found)
{
	struct point_coord* coords = (struct point_coord*)malloc(0);
	int c = 0,i,j;
	unsigned long p;

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
			}
		};

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
			long points_found;
			struct point_coord* points = find_points(threshold, image, &points_found);

			if(points_found)
			{
				/* sort items by value */
				sort_points(points, points_found);


				/* allocate space for points */
				points_x = (double*)realloc(points_x, sizeof(double) * (points_count + 1));
				points_y = (double*)realloc(points_y, sizeof(double) * (points_count + 1));

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

				/* putput in image coordinates */
				fprintf(stderr, "\t[%7.3lf, %7.3lf]\n", points_x[points_count], points_y[points_count]);

				/* translate */
				points_x[points_count] = points_x[points_count] - (double)trans_x;
				points_y[points_count] = (double)image->height - points_y[points_count] - (double)trans_y;


				/* inc */
				points_count++;
			};

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
		};

		/* output */
		for(i = 0; i<points_count; i++)
		{
			fprintf(stdout, "%7.3lf\t%7.3lf\n", points_x[i], points_y[i]);
		};
		
	};

	return 0;
};
