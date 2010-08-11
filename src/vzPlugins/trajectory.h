#ifndef TRAJECTORY_H
#define TRAJECTORY_H

#include <stdio.h>

static char* find_ch(char* tokens, long ch)
{
	for(; *tokens; tokens++)
		if(*tokens == ch) return tokens;
	return 0;
};

static void trim_ch(char* str, char* tokens)
{
	char* c;
	for(; (*str) ; str++)
		if(NULL != find_ch(tokens, *str))
			for(c = str; *c; c++)
				*c = *(c+1);
};

static int split_str(char* str, char* splitter, char*** s)
{
	int c = 0;
	char *t1, *t2;

    /* check if string defined */
    if(!str || !splitter)
    {
        *s = NULL;
        return 0;
    };

	/* allocate mem for struct */
	*s = (char**)malloc(0);

	for(t2 = str; 1 ; )
	{
		/* find non terminal character - skip */
		for(t1 = t2; (*t1)&&(NULL != find_ch(splitter, *t1)) ; t1++);

		/* check if and found */
		if(0 == (*t1)) break;

		/* find next ternimals */
		for(t2 = t1; (*t2)&&(NULL == find_ch(splitter, *t2)) ; t2++);

		/* check if token found */
		if(t1 != t2)
		{
			/* reallocate space for array */
			*s = (char**)realloc(*s, sizeof(char**) * (c + 1));

			/* allocate space for string part */
			(*s)[c] = (char*)malloc(1 + (t2 - t1));

			/* copy string */
			memcpy((*s)[c], t1, (t2 - t1));
			(*s)[c][t2 - t1] = 0;

			/* increment counter */
			c++;

			/* shift */
			if(*t2) t2++;
		};
	};

	/* setup last node in chain */
	*s = (char**)realloc(*s, sizeof(char**) * (c + 1));
	(*s)[c] = NULL;

	return c;
};

static void split_str_free(char*** s)
{
	char** strs = *s;
    if(strs)
    {
        for(; *strs; strs++) free(*strs);
        free(*s);
    };
	*s = NULL;
};

static char** parse_map(char* map_string, long* len)
{
	long c1,c2,i;
	char **p0, **p1;

	char** map_data = (char**)malloc(0);
	long map_len = 0;

	static char* splitters_records = ";,/|";
	static char* splitters_pairs = ":=-><";

	/* split into params */
	for(c1 = split_str(map_string, splitters_records, &p0),i=0; i<c1; i++)
	{
		/* split in pairs */
		if(2 == (c2 = split_str(p0[i], splitters_pairs, &p1)))
		{
			/* trim pairs */
			trim_ch(p1[0], " ");
			trim_ch(p1[1], " ");

			/* save to map table */

			/* allocate space for slot */
			map_data = (char**)realloc(map_data, sizeof(char*)*2*(map_len + 1));

			/* copy data */
			map_data[2*map_len + 0] = (char*)atol(p1[0]);
			map_data[2*map_len + 1] = (char*)malloc(strlen(p1[1]) + 1);
			strcpy(map_data[2*map_len + 1], p1[1]);

			/* increment counter */
			map_len++;
		};
		split_str_free(&p1);
	};
	split_str_free(&p0);


	/* setup terminator */
	map_data = (char**)realloc(map_data, sizeof(char*)*2*(map_len + 1));
	map_data[2*map_len + 0] = 0;
	map_data[2*map_len + 1] = 0;
	

	/* return data */
	*len = map_len;
	return map_data;
};

static void free_map(char** map)
{
	long i;
	for(i = 0; map[i + 1] ; i += 2) free(map[i + 1]);
	free(map);
};

static void free_table(char*** t)
{
	long i,j,c;

	for(i = 0; t[i] ; i++)
	{
		for(c = (int)t[i][0], j = 0; j<c ; j++)
			free(t[i][j + 1]);
		free(t[i]);
	};
	free(t);
};

static char*** parse_table(char* filename, long *count)
{
	char buf[1024];
	char*** data = (char***)malloc(0);
	long len = 0,c = 0, i;
	char** p;
	FILE* f;

	/* open file */
	if(NULL != (f = fopen(filename, "rt")))
	{
		while(!(feof(f)))
		{
			if(NULL != fgets(buf, sizeof(buf), f))
			{
				/* clear unused chars */
				trim_ch(buf, "\n\r");

				/* split into parts */
				if(c = split_str(buf, "\t;:/", &p))
				{
					/* reallocate datas */
					data = (char***)realloc(data, sizeof(char**)*(len + 1));

					/* allocate space for row */
					data[len] = (char**) malloc(sizeof(char*)*(c + 1));

					/* setup cols count */
					data[len][0] = (char*)c;

					/* copy str pointer and trim chars */
					for(i = 0; i<c ; i++)
					{
						/* cleanup spaces */
						trim_ch(p[i], " ");

						/* assign */
						data[len][i + 1] = p[i];
					};

					/* free container, but not data */
					free(p);

					/* increment length */
					len++;
				}
				else
					split_str_free(&p);
			};

		};
		fclose(f);
	}

	/* setup terminator */
	data = (char***)realloc(data, sizeof(char**)*(len + 1));
	data[len] = 0;

	/* return datas */
	*count = len;
	return data;
};

static char* lookup_table_value(char* param_name, int index, char** map, char*** table, long table_len)
{
	long i,k = -1;

	/* lookup param index */
	for(i = 0; (NULL != map[i + 1]) && ((-1) == k) ; i+=2)
		if(0 == strcmp(param_name, map[i + 1]))
			k = (int)map[i];

	/* check if param found */
	if((-1) == k) return NULL;

	/* check index value */
	if( (index < 0) || (index >= table_len) ) return NULL;

	/* check if row has enough wide */
	if( ((int)table[index][0]) <= k) return NULL;

	/* return value */
	return table[index][k + 1];
};

#endif /* TRAJECTORY_H */
