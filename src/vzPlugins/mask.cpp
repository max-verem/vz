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
	2005-07-07:
		*Fixed issue about incorrect mask in frame-based mode

	2005-06-24:
		*Successfully tested with 'demo1b.xml' !!!
		*Starting draft version.

*/

#include "../vz/plugin-devel.h"
#include "../vz/plugin.h"

// declare name and version of plugin
PLUGIN_EXPORT vzPluginInfo info =
{
	"mask",
	1.0,
	"rc8"
};

// internal structure of plugin
typedef struct
{
	long l_id;			// level is mask number from 2..8
	long L_operation;	// parameter defines that is a mask or target

	// save state variables
	GLint _GL_STENCIL_WRITEMASK;
	GLint _GL_STENCIL_REF;
	GLint _GL_STENCIL_VALUE_MASK;
	GLint _GL_STENCIL_FUNC;
	GLint _GL_COLOR_WRITEMASK[4];
} vzPluginData;

// default value of structore
vzPluginData default_value = 
{
	2,
	0,

	0,
	0,
	0,
	0
};

PLUGIN_EXPORT vzPluginParameter parameters[] = 
{
	{"l_id", "Mask identificator (value should be between 2..8)", PLUGIN_PARAMETER_OFFSET(default_value,l_id)},
	{"L_operation", "Operation definition: '_TRG' - targeting or '_SRC' -  mask source drawing", PLUGIN_PARAMETER_OFFSET(default_value,L_operation)},
	{NULL,NULL,0}
};


PLUGIN_EXPORT void* constructor(void)
{
	// init memmory for structure
	vzPluginData* data = (vzPluginData*)malloc(sizeof(vzPluginData));

	// copy default value
	*data = default_value;

	// return pointer
	return data;
};

PLUGIN_EXPORT void destructor(void* data)
{
	// free data
	free(data);
};

PLUGIN_EXPORT void prerender(void* data,vzRenderSession* session)
{
	// define operation type
	if (_DATA->L_operation == FOURCC_TO_LONG('_','T','R','G'))
	{
		// target operation

		// 1. save previous stencil ref value and mask
		glGetIntegerv(GL_STENCIL_REF,&_DATA->_GL_STENCIL_REF);
		glGetIntegerv(GL_STENCIL_VALUE_MASK,&_DATA->_GL_STENCIL_VALUE_MASK);
		glGetIntegerv(GL_STENCIL_FUNC,&_DATA->_GL_STENCIL_FUNC);
		
		// 2. modify stencil value to write specific bit to stencil buffer
		glStencilFunc(GL_EQUAL,_DATA->_GL_STENCIL_REF | (1<<_DATA->l_id), _DATA->_GL_STENCIL_VALUE_MASK | (1<<_DATA->l_id));
	};

	if (_DATA->L_operation == FOURCC_TO_LONG('_','S','R','C'))
	{
		// drawing mask - we draw stencil bits
		
		// 1. save previous stencil mask
		glGetIntegerv(GL_STENCIL_WRITEMASK,&_DATA->_GL_STENCIL_WRITEMASK);

		// 2. enable bit for writing
		glStencilMask(_DATA->_GL_STENCIL_WRITEMASK | (1<<_DATA->l_id) );

		// 3. save previous stencil ref value and mask
		glGetIntegerv(GL_STENCIL_REF,&_DATA->_GL_STENCIL_REF);
		glGetIntegerv(GL_STENCIL_VALUE_MASK,&_DATA->_GL_STENCIL_VALUE_MASK);
		glGetIntegerv(GL_STENCIL_FUNC,&_DATA->_GL_STENCIL_FUNC);

		// 4. modify stencil value to write specific bit to stencil buffer
		glStencilFunc(GL_EQUAL,_DATA->_GL_STENCIL_REF | (1<<_DATA->l_id), _DATA->_GL_STENCIL_VALUE_MASK | (1<<_DATA->l_id));

		// 5. save previous colour masks values
		glGetIntegerv(GL_COLOR_WRITEMASK,_DATA->_GL_COLOR_WRITEMASK);

		// 6. disable writing to colour buffer
		glColorMask(0,0,0,0);

		// further drawing will only affect i-th rising of stencil buffer!
	};

};

PLUGIN_EXPORT void postrender(void* data,vzRenderSession* session)
{
	// define operation type
	if (_DATA->L_operation == FOURCC_TO_LONG('_','T','R','G'))
	{
		// target operation

		// 1. restore stencil ref value and mask
		glStencilFunc(_DATA->_GL_STENCIL_FUNC,_DATA->_GL_STENCIL_REF , _DATA->_GL_STENCIL_VALUE_MASK);

	};

	if (_DATA->L_operation == FOURCC_TO_LONG('_','S','R','C'))
	{
		// source mask drawing
		// restore original values

		// 1. restore write mask for stencil buffer
		glStencilMask(_DATA->_GL_STENCIL_WRITEMASK);

		// 2. restore stencil ref value and mask
		glStencilFunc(_DATA->_GL_STENCIL_FUNC,_DATA->_GL_STENCIL_REF , _DATA->_GL_STENCIL_VALUE_MASK);

		// 6. restore colour buffer 
		glColorMask(_DATA->_GL_COLOR_WRITEMASK[0],_DATA->_GL_COLOR_WRITEMASK[1],_DATA->_GL_COLOR_WRITEMASK[2],_DATA->_GL_COLOR_WRITEMASK[3]);

	};
};

PLUGIN_EXPORT void render(void* data,vzRenderSession* session)
{

};

PLUGIN_EXPORT void notify(void* data)
{

};
