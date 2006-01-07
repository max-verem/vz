/*
    ViZualizator
    (Real-Time TV graphics production system)

    Copyright (C) 2005 Maksym Veremeyenko.
    This file is part of ViZualizator (Real-Time TV graphics production system).
    Contributed by Maksym Veremeyenko, verem@m1stereo.tv, 2005.

    ViZualizator is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    ViZualizator is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with ViZualizator; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

ChangeLog:
    2005-06-08: Code cleanup


*/

char* get_glerror()
{
	switch(glGetError())
	{
		case GL_NO_ERROR: return "GL_NO_ERROR";
		case GL_INVALID_ENUM: return "GL_INVALID_ENUM: An unacceptable value is specified for an enumerated argument. The offending function is ignored, having no side effect other than to set the error flag.";
		case GL_INVALID_VALUE: return "GL_INVALID_VALUE: A numeric argument is out of range. The offending function is ignored, having no side effect other than to set the error flag.";
		case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION: The specified operation is not allowed in the current state. The offending function is ignored, having no side effect other than to set the error flag.";
		case GL_STACK_OVERFLOW: return "GL_STACK_OVERFLOW: This function would cause a stack overflow. The offending function is ignored, having no side effect other than to set the error flag.";
		case GL_STACK_UNDERFLOW: return "GL_STACK_UNDERFLOW: This function would cause a stack underflow. The offending function is ignored, having no side effect other than to set the error flag.";
		case GL_OUT_OF_MEMORY: return "GL_OUT_OF_MEMORY: There is not enough memory left to execute the function. The state of OpenGL is undefined, except for the state of the error flags, after this error is recorded.";
		default:
			return "UNKNOWN";
	};
};

void center_vector(long L_center,float f_width, float f_height, float& co_X, float& co_Y)
{
	int i = 0;
	static const float v[18] = 
	{
		0.0f,	-1.0f,
		-0.5f,	-1.0f,
		-1.0f,	-1.0f,

		0.0f,	-0.5f,
		-0.5f,	-0.5f,
		-1.0f,	-0.5f,

		0.0f,	0.0f,
		-0.5f,	0.0f,
		-1.0f,	0.0f
	};

	switch(L_center)
	{
		// left-top
		case GEOM_CENTER_LT:i=0;break;
		// center-top
		case GEOM_CENTER_CT:i=1;;break;
		// right-top
		case GEOM_CENTER_RT:i=2;;break;

		// left-middle
		case GEOM_CENTER_LM:i=3;break;
		// center-middle
		case GEOM_CENTER_CM:i=4;break;
		// right-middle
		case GEOM_CENTER_RM:i=5;break;

		// left-bottom
		case GEOM_CENTER_LB:i=6;break;
		// center-bottom
		case GEOM_CENTER_CB:i=7;break;
		// right-bottom
		case GEOM_CENTER_RB:i=8;break;
	};

	co_X += f_width * v[2*i];
	co_Y += f_height * v[2*i + 1];
};

inline unsigned long POT(unsigned long v)
{
	for(unsigned long i=1;(i!=0x80000000)&&(i<v);i<<=1);
	return i;
};