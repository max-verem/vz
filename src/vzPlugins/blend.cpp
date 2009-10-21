/*
    ViZualizator
    (Real-Time TV graphics production system)

    Copyright (C) 2009 Maksym Veremeyenko.
    This file is part of ViZualizator (Real-Time TV graphics production system).
    Contributed by Maksym Veremeyenko, verem@m1stereo.tv, 2009.

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

*/
static char* _plugin_description = 
"Turn (or override) blending parameters defined by BlendFuncSeparateEXT\n"
"or BlendEquationSeparateEXT\n"
;

static char* _plugin_notes = 
"For description and meaning values of theese functions refer to:\n"
"    BlendFuncSeparateEXT:\n"
"    http://oss.sgi.com/projects/ogl-sample/registry/EXT/blend_func_separate.txt\n"
"\n"
"    BlendEquationSeparateEXT:\n"
"    http://www.opengl.org/registry/specs/EXT/blend_equation_separate.txt\n"
"\n"
"Text parameters supplied as text, i.e\n"
"[...]\n"
"      <param name=\"s_sfactorRGB\" value=\"ONE_MINUS_DST_ALPHA\"/>\n"
"[...]\n"
"\n"
;

#include "../vz/plugin-devel.h"
#include "../vz/plugin.h"
//#include "../vz/vzGlExt.h"

// declare name and version of plugin
DEFINE_PLUGIN_INFO("blend");

// internal structure of plugin
typedef struct
{
    long l_BlendFuncSeparateEXT;        /* use function l_BlendFuncSeparateEXT */
    char* s_sfactorRGB;
    char* s_dfactorRGB;
    char* s_sfactorAlpha;
    char* s_dfactorAlpha;

    long l_BlendEquationSeparateEXT;    /* use function BlendEquationSeparateEXT */
    char* s_modeRGB;
    char* s_modeAlpha;

	// save state variables
    GLint _BLEND_DST_RGB_EXT;
    GLint _BLEND_SRC_RGB_EXT;
    GLint _BLEND_DST_ALPHA_EXT;
    GLint _BLEND_SRC_ALPHA_EXT;
    GLint _BLEND_EQUATION_RGB_EXT;
    GLint _BLEND_EQUATION_ALPHA_EXT;
} vzPluginData;

// default value of structore
vzPluginData default_value = 
{
	0,
    NULL,
    NULL,
    NULL,
    NULL,

    0,
    NULL,
    NULL,

    0.0f,
    0.0f,
    0.0f,
    0.0f,

    0,
	0
};

PLUGIN_EXPORT vzPluginParameter parameters[] = 
{
	{"l_BlendFuncSeparateEXT",
        "Flag to enable function use",
        PLUGIN_PARAMETER_OFFSET(default_value, l_BlendFuncSeparateEXT)},

    {"s_sfactorRGB",
        "source RGB factor, see BlendFuncSeparateEXT manual",
        PLUGIN_PARAMETER_OFFSET(default_value, s_sfactorRGB)},

	{"s_dfactorRGB",
        "destination RGB factor, see BlendFuncSeparateEXT manual",
        PLUGIN_PARAMETER_OFFSET(default_value, s_dfactorRGB)},

	{"s_sfactorAlpha",
        "source Alpha factor, see BlendFuncSeparateEXT manual",
        PLUGIN_PARAMETER_OFFSET(default_value, s_sfactorAlpha)},

	{"s_dfactorAlpha",
        "destination Alpha factor, see BlendFuncSeparateEXT manual",
        PLUGIN_PARAMETER_OFFSET(default_value, s_dfactorAlpha)},

	{"l_BlendEquationSeparateEXT",
        "Flag to enable function use",
        PLUGIN_PARAMETER_OFFSET(default_value, l_BlendEquationSeparateEXT)},

	{"s_modeRGB",
        "modeRGB, see BlendEquationSeparateEXT manual",
        PLUGIN_PARAMETER_OFFSET(default_value, s_modeRGB)},

	{"s_modeAlpha",
        "modeAlpha, see BlendEquationSeparateEXT manual",
        PLUGIN_PARAMETER_OFFSET(default_value, s_modeAlpha)},

    {NULL,NULL,0}
};


PLUGIN_EXPORT void* constructor(void* scene, void* parent_container)
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

PLUGIN_EXPORT void prerender(void* data, vzRenderSession* session)
{
    /* save to stack values of blending function and equantion */
//    if(_DATA->l_BlendFuncSeparateEXT || _DATA->l_BlendEquationSeparateEXT)
//        glPushAttrib(GL_BLEND);

    if(_DATA->l_BlendFuncSeparateEXT)
    {
        /* save old values */
        glGetIntegerv(GL_BLEND_SRC_RGB_EXT,   &_DATA->_BLEND_SRC_RGB_EXT);
        glGetIntegerv(GL_BLEND_DST_RGB_EXT,   &_DATA->_BLEND_DST_RGB_EXT);
        glGetIntegerv(GL_BLEND_SRC_ALPHA_EXT, &_DATA->_BLEND_SRC_ALPHA_EXT);
        glGetIntegerv(GL_BLEND_DST_ALPHA_EXT, &_DATA->_BLEND_DST_ALPHA_EXT);

        /* convert attr attrs */
        GLenum sfactorRGB   = vzGlExtEnumLookup(_DATA->s_sfactorRGB);
        GLenum dfactorRGB   = vzGlExtEnumLookup(_DATA->s_dfactorRGB);
        GLenum sfactorAlpha = vzGlExtEnumLookup(_DATA->s_sfactorAlpha);
        GLenum dfactorAlpha = vzGlExtEnumLookup(_DATA->s_dfactorAlpha);

        /* check if all attrs defined */
        if
        (
            GL_UNKNOWN_ATTR != sfactorRGB   &&
            GL_UNKNOWN_ATTR != dfactorRGB   && 
            GL_UNKNOWN_ATTR != sfactorAlpha &&
            GL_UNKNOWN_ATTR != dfactorAlpha
        )
            glBlendFuncSeparateEXT(sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha);
    };

    if(_DATA->l_BlendEquationSeparateEXT)
    {
        /* convert attr attrs */
        GLenum modeRGB = vzGlExtEnumLookup(_DATA->s_modeRGB);
        GLenum modeAlpha = vzGlExtEnumLookup(_DATA->s_modeAlpha);

        /* check if all attrs defined */
        if
        (
            GL_UNKNOWN_ATTR == modeRGB      &&
            GL_UNKNOWN_ATTR == modeAlpha
        )
            glBlendEquationSeparateEXT(modeRGB, modeAlpha);
    };
};

PLUGIN_EXPORT void postrender(void* data,vzRenderSession* session)
{
    /* save to stack values of blending function and equantion */
//    if(_DATA->l_BlendFuncSeparateEXT || _DATA->l_BlendEquationSeparateEXT)
//        glPopAttrib();

    /* restore */
    if(_DATA->l_BlendFuncSeparateEXT)
        glBlendFuncSeparateEXT
        (
            _DATA->_BLEND_SRC_RGB_EXT, _DATA->_BLEND_DST_RGB_EXT,
            _DATA->_BLEND_SRC_ALPHA_EXT, _DATA->_BLEND_DST_ALPHA_EXT
        );
};

PLUGIN_EXPORT void render(void* data,vzRenderSession* session)
{

};

PLUGIN_EXPORT void notify(void* data, char* param_name)
{

};
