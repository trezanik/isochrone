/**
 * @file        src/imgui/TConverter.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "imgui/definitions.h"

#include "imgui/ImNodeGraphPin.h"
#include "imgui/ImNodeGraphLink.h"
#include "imgui/event/ImGuiEvent.h"
#include "imgui/TConverter.h"

#include "core/util/string/STR_funcs.h"


namespace trezanik {
namespace imgui {


//-------------- PinSocketShape

const char  str_circle[] = "Circle";
const char  str_square[] = "Square";
const char  str_diamond[] = "Diamond";
const char  str_hexagon[] = "Hexagon";

template<>
trezanik::imgui::PinSocketShape
TConverter<trezanik::imgui::PinSocketShape>::FromString(
	const char* str
)
{
	bool  case_sensitive = false;

	if ( STR_compare(str, str_circle, case_sensitive) == 0 )
		return trezanik::imgui::PinSocketShape::Circle;
	if ( STR_compare(str, str_square, case_sensitive) == 0 )
		return trezanik::imgui::PinSocketShape::Square;
	if ( STR_compare(str, str_diamond, case_sensitive) == 0 )
		return trezanik::imgui::PinSocketShape::Diamond;
	if ( STR_compare(str, str_hexagon, case_sensitive) == 0 )
		return trezanik::imgui::PinSocketShape::Hexagon;

	return trezanik::imgui::PinSocketShape::Invalid;
}


template<>
trezanik::imgui::PinSocketShape
TConverter<trezanik::imgui::PinSocketShape>::FromString(
	const std::string& str
)
{
	return FromString(str.c_str());
}


template<>
trezanik::imgui::PinSocketShape
TConverter<trezanik::imgui::PinSocketShape>::FromUint8(
	const uint8_t uint8
)
{
	// the last element
	if ( uint8 > static_cast<uint8_t>(trezanik::imgui::PinSocketShape::Hexagon) )
		return trezanik::imgui::PinSocketShape::Invalid;

	return static_cast<trezanik::imgui::PinSocketShape>(uint8);
}


template<>
std::string
TConverter<trezanik::imgui::PinSocketShape>::ToString(
	trezanik::imgui::PinSocketShape value
)
{
	switch ( value )
	{
	case trezanik::imgui::PinSocketShape::Circle:   return str_circle;
	case trezanik::imgui::PinSocketShape::Square:   return str_square;
	case trezanik::imgui::PinSocketShape::Diamond:  return str_diamond;
	case trezanik::imgui::PinSocketShape::Hexagon:  return str_hexagon;
	default:
		break;
	}

	return text_invalid;
}


template<>
uint8_t
TConverter<trezanik::imgui::PinSocketShape>::ToUint8(
	trezanik::imgui::PinSocketShape value
)
{
	return static_cast<uint8_t>(value);
}


//-------------- PinStyleDisplay

const char  str_shape[] = "Shape";
const char  str_image[] = "Image";

template<>
trezanik::imgui::PinStyleDisplay
TConverter<trezanik::imgui::PinStyleDisplay>::FromString(
	const char* str
)
{
	bool  case_sensitive = false;

	if ( STR_compare(str, str_shape, case_sensitive) == 0 )
		return trezanik::imgui::PinStyleDisplay::Shape;
	if ( STR_compare(str, str_image, case_sensitive) == 0 )
		return trezanik::imgui::PinStyleDisplay::Image;

	return trezanik::imgui::PinStyleDisplay::Invalid;
}


template<>
trezanik::imgui::PinStyleDisplay
TConverter<trezanik::imgui::PinStyleDisplay>::FromString(
	const std::string& str
)
{
	return FromString(str.c_str());
}


template<>
trezanik::imgui::PinStyleDisplay
TConverter<trezanik::imgui::PinStyleDisplay>::FromUint8(
	const uint8_t uint8
)
{
	// the last element
	if ( uint8 > static_cast<uint8_t>(trezanik::imgui::PinStyleDisplay::Image) )
		return trezanik::imgui::PinStyleDisplay::Invalid;

	return static_cast<trezanik::imgui::PinStyleDisplay>(uint8);
}


template<>
std::string
TConverter<trezanik::imgui::PinStyleDisplay>::ToString(
	trezanik::imgui::PinStyleDisplay value
)
{
	switch ( value )
	{
	case trezanik::imgui::PinStyleDisplay::Shape:  return str_shape;
	case trezanik::imgui::PinStyleDisplay::Image:  return str_image;
	default:
		break;
	}

	return text_invalid;
}


template<>
uint8_t
TConverter<trezanik::imgui::PinStyleDisplay>::ToUint8(
	trezanik::imgui::PinStyleDisplay value
)
{
	return static_cast<uint8_t>(value);
}


//-------------- LinkMethod

const char  str_direct[]          = "direct";
const char  str_quadbezier[]      = "quadratic-bezier";
const char  str_cubicbezier[]     = "cubic-bezier";
const char  str_multilineauto[]   = "multiline-auto";
const char  str_multilinehybrid[] = "multiline-hybrid";

template<>
LinkMethod
TConverter<LinkMethod>::FromString(
	const char* str
)
{
	bool  case_sensitive = false;

	if ( STR_compare(str, str_direct, case_sensitive) == 0 )
		return LinkMethod::Direct;
	if ( STR_compare(str, str_quadbezier, case_sensitive) == 0 )
		return LinkMethod::QuadraticBezier;
	if ( STR_compare(str, str_cubicbezier, case_sensitive) == 0 )
		return LinkMethod::CubicBezier;
	if ( STR_compare(str, str_multilineauto, case_sensitive) == 0 )
		return LinkMethod::MultiLineAuto;
	if ( STR_compare(str, str_multilinehybrid, case_sensitive) == 0 )
		return LinkMethod::MultiLineHybrid;

	return LinkMethod::Invalid;
}


template<>
LinkMethod
TConverter<LinkMethod>::FromString(
	const std::string& str
)
{
	return FromString(str.c_str());
}


 
template<>
LinkMethod
TConverter<LinkMethod>::FromUint8(
	const uint8_t uint8
)
{
	// the last element
	if ( uint8 > static_cast<uint8_t>(LinkMethod::MultiLineHybrid) )
		return LinkMethod::Invalid;

	return static_cast<LinkMethod>(uint8);
}
 
 
template<>
std::string
TConverter<LinkMethod>::ToString(
	LinkMethod proto
)
{
	switch ( proto )
	{
	case LinkMethod::Direct:           return str_direct;
	case LinkMethod::QuadraticBezier:  return str_quadbezier;
	case LinkMethod::CubicBezier:      return str_cubicbezier;
	case LinkMethod::MultiLineAuto:    return str_multilineauto;
	case LinkMethod::MultiLineHybrid:  return str_multilinehybrid;
	default:
		break;
	}

	return text_invalid;
}
 
 
template<>
uint8_t
TConverter<LinkMethod>::ToUint8(
	LinkMethod method
)
{
	return static_cast<uint8_t>(method);
}


//-------------- NodeGraphUpdate

const char  str_node_position[] = "NodePosition";
const char  str_node_size[]     = "NodeSize";
const char  str_node_del[]      = "NodeDeleting";
const char  str_node_name[]     = "NodeName";
const char  str_node_data[]     = "NodeData";
const char  str_node_style[]    = "NodeStyle";
const char  str_node_select[]   = "NodeSelected";
const char  str_node_unselect[] = "NodeUnselected";
const char  str_pin_del[]       = "PinDeleted";
const char  str_pin_position[]  = "PinPosition";
const char  str_pin_style[]     = "PinStyle";
const char  str_link_assign[]   = "LinkAssigned";
const char  str_link_del[]      = "LinkDeleted";
const char  str_link_unassign[] = "LinkUnassigned";

template<>
trezanik::imgui::NodeGraphUpdate
TConverter<trezanik::imgui::NodeGraphUpdate>::FromString(
	const char* str
)
{
	bool  case_sensitive = false;

	if ( STR_compare(str, str_node_position, case_sensitive) == 0 )
		return trezanik::imgui::NodeGraphUpdate::NodePosition;
	if ( STR_compare(str, str_node_size, case_sensitive) == 0 )
		return trezanik::imgui::NodeGraphUpdate::NodeSize;
	if ( STR_compare(str, str_node_del, case_sensitive) == 0 )
		return trezanik::imgui::NodeGraphUpdate::NodeDeleting;
	if ( STR_compare(str, str_node_name, case_sensitive) == 0 )
		return trezanik::imgui::NodeGraphUpdate::NodeName;
	if ( STR_compare(str, str_node_data, case_sensitive) == 0 )
		return trezanik::imgui::NodeGraphUpdate::NodeData;
	if ( STR_compare(str, str_node_style, case_sensitive) == 0 )
		return trezanik::imgui::NodeGraphUpdate::NodeStyle;
	if ( STR_compare(str, str_node_select, case_sensitive) == 0 )
		return trezanik::imgui::NodeGraphUpdate::NodeSelected;
	if ( STR_compare(str, str_node_unselect, case_sensitive) == 0 )
		return trezanik::imgui::NodeGraphUpdate::NodeUnselected;
	if ( STR_compare(str, str_pin_del, case_sensitive) == 0 )
		return trezanik::imgui::NodeGraphUpdate::PinDeleted;
	if ( STR_compare(str, str_pin_position, case_sensitive) == 0 )
		return trezanik::imgui::NodeGraphUpdate::PinPosition;
	if ( STR_compare(str, str_pin_style, case_sensitive) == 0 )
		return trezanik::imgui::NodeGraphUpdate::PinStyle;
	if ( STR_compare(str, str_link_assign, case_sensitive) == 0 )
		return trezanik::imgui::NodeGraphUpdate::LinkAssigned;
	if ( STR_compare(str, str_link_del, case_sensitive) == 0 )
		return trezanik::imgui::NodeGraphUpdate::LinkDeleted;
	if ( STR_compare(str, str_link_unassign, case_sensitive) == 0 )
		return trezanik::imgui::NodeGraphUpdate::LinkUnassigned;

	return trezanik::imgui::NodeGraphUpdate::Invalid;
}


template<>
trezanik::imgui::NodeGraphUpdate
TConverter<trezanik::imgui::NodeGraphUpdate>::FromString(
	const std::string& str
)
{
	return FromString(str.c_str());
}


// not a uint8_t, no FromUint8()


template<>
std::string
TConverter<trezanik::imgui::NodeGraphUpdate>::ToString(
	trezanik::imgui::NodeGraphUpdate proto
)
{
	switch ( proto )
	{
	case trezanik::imgui::NodeGraphUpdate::NodePosition: return str_node_position;
	case trezanik::imgui::NodeGraphUpdate::NodeSize: return str_node_size;
	case trezanik::imgui::NodeGraphUpdate::NodeDeleting: return str_node_del;
	case trezanik::imgui::NodeGraphUpdate::NodeName: return str_node_name;
	case trezanik::imgui::NodeGraphUpdate::NodeData: return str_node_data;
	case trezanik::imgui::NodeGraphUpdate::NodeStyle: return str_node_style;
	case trezanik::imgui::NodeGraphUpdate::NodeSelected:  return str_node_select;
	case trezanik::imgui::NodeGraphUpdate::NodeUnselected: return str_node_unselect;
	case trezanik::imgui::NodeGraphUpdate::PinDeleted:  return str_pin_del;
	case trezanik::imgui::NodeGraphUpdate::PinPosition: return str_pin_position;
	case trezanik::imgui::NodeGraphUpdate::PinStyle: return str_pin_style;
	case trezanik::imgui::NodeGraphUpdate::LinkAssigned: return str_link_assign;
	case trezanik::imgui::NodeGraphUpdate::LinkDeleted: return str_link_del;
	case trezanik::imgui::NodeGraphUpdate::LinkUnassigned: return str_link_unassign;
	default:
		break;
	}

	return text_invalid;
}


// not a uint8_t, no ToUint8()


} // namespace imgui
} // namespace trezanik
