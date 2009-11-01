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
	2007-11-16: 
		*Visual Studio 2005 migration.

    2005-06-08: Code cleanup

*/


#ifndef XERCES_H
#define XERCES_H

#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/parsers/AbstractDOMParser.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/dom/DOMElement.hpp>
#include <xercesc/dom/DOMNode.hpp>
#include <xercesc/dom/DOMNodeList.hpp>
#include <xercesc/dom/DOMNamedNodeMap.hpp>

// xerces library linking
#ifdef _DEBUG
	#pragma comment(lib, "xerces-c_2D.lib") 
#else
	#pragma comment(lib, "xerces-c_2.lib") 
#endif

#define DOMDocumentX xercesc_2_8::DOMDocument
#define DOMNodeX xercesc_2_8::DOMNode
#define XercesDOMParserX xercesc_2_8::XercesDOMParser
#define DOMElementX xercesc_2_8::DOMElement
#define DOMNodeListX xercesc_2_8::DOMNodeList
#define XMLStringX xercesc_2_8::XMLString
#define XMLPlatformUtilsX xercesc_2_8::XMLPlatformUtils
#define DOMNamedNodeMapX xercesc_2_8::DOMNamedNodeMap

#endif
