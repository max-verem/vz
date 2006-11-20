inline void calc_ft_vec_part
(
	float a_x, float a_y, 
	float b_x, float b_y, 
	float k,
	float* c_x, float* c_y
)
{
	*c_x = a_x + (b_x - a_x)*k;
	*c_y = a_y + (b_y - a_y)*k;
};

inline void calc_ft_vec_prolong
(
	float x1,float y1,	/* vector from */
	float x2,float y2,	/* vector to */
	long l,				/* origin vector length */
	long L,				/* resulting vector length */
	float* X, float* Y	/* resulting coordinate */
)
{
	double k;

	/* check condition */
	if(0 == l)
	{
		*X = x1; *Y = y1;
		return;
	};

	k = (float)L/(float)l;

	*X = (float)(k*(x2 - x1) + x1);
	*Y = (float)(k*(y2 - y1) + y1);
};

inline void calc_ft_vec_cross
(
	float a1_x, float a1_y, 
	float a2_x, float a2_y, 
	float b1_x, float b1_y, 
	float b2_x, float b2_y, 
	
	float* c_x, float* c_y
)
{
	/* matrix coefs */
	double
		a1 = a2_x - a1_x,
		b1 = b1_x - b2_x,
		c1 = b1_x - a1_x,

		a2 = a2_y - a1_y,
		b2 = b1_y - b2_y,
		c2 = b1_y - a1_y;

	/* matrix determinant */
	double
		d = a1*b2 - b1*a2;

	/* check if vectors are parallel */
	if(0.0 == d) return;

	/* calc k.. */
	double ka = (b2*c1 - b1*c2)/d;
	double kb = (c2*a1 - c1*a2)/d;

	/* calc cross coordinates */
	*c_x = (float)( (a2_x - a1_x)*ka + a1_x );  
	*c_y = (float)( (a2_y - a1_y)*ka + a1_y );  
};



inline void calc_free_transform
(
	/* input params */
	long width, long height,
	long base_width, long base_height,
	float x1, float y1,
	float x2, float y2,
	float x3, float y3,
	float x4, float y4,

	/* output params */
	float* X1, float* Y1,
	float* X2, float* Y2,
	float* X3, float* Y3,
	float* X4, float* Y4
)
{
	float 
		X1a, Y1a, X2a, Y2a, X3a, Y3a, X4a, Y4a,
		X1b, Y1b, X2b, Y2b, X3b, Y3b, X4b, Y4b;

	long h = base_height + (height - base_height)/2;
	long w = base_width + (width - base_width)/2;

	/* points A */
	calc_ft_vec_prolong(x1,y1, x2,y2, base_height, h, &X2a, &Y2a);
	calc_ft_vec_prolong(x2,y2, x1,y1, base_height, h, &X1a, &Y1a);
	calc_ft_vec_prolong(x4,y4, x3,y3, base_height, h, &X3a, &Y3a);
	calc_ft_vec_prolong(x3,y3, x4,y4, base_height, h, &X4a, &Y4a);

	/* points B */
	calc_ft_vec_prolong(x4,y4, x1,y1, base_width, w, &X1b, &Y1b);
	calc_ft_vec_prolong(x1,y1, x4,y4, base_width, w, &X4b, &Y4b);
	calc_ft_vec_prolong(x3,y3, x2,y2, base_width, w, &X2b, &Y2b);
	calc_ft_vec_prolong(x2,y2, x3,y3, base_width, w, &X3b, &Y3b);
	

	calc_ft_vec_cross(X1a,Y1a, X4a,Y4a, X1b,Y1b, X2b,Y2b, X1,Y1);
	calc_ft_vec_cross(X2a,Y2a, X3a,Y3a, X2b,Y2b, X1b,Y1b, X2,Y2);
	calc_ft_vec_cross(X3a,Y3a, X2a,Y2a, X3b,Y3b, X4b,Y4b, X3,Y3);
	calc_ft_vec_cross(X4a,Y4a, X1a,Y1a, X4b,Y4b, X3b,Y3b, X4,Y4);
};
