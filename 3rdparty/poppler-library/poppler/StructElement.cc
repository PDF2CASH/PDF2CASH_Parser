//========================================================================
//
// StructElement.cc
//
// This file is licensed under the GPLv2 or later
//
// Copyright 2013, 2014 Igalia S.L.
// Copyright 2014 Luigi Scarso <luigi.scarso@gmail.com>
// Copyright 2014, 2017, 2018 Albert Astals Cid <aacid@kde.org>
// Copyright 2015 Dmytro Morgun <lztoad@gmail.com>
// Copyright 2018 Adrian Johnson <ajohnson@redneon.com>
// Copyright 2018 Klarälvdalens Datakonsult AB, a KDAB Group company, <info@kdab.com>. Work sponsored by the LiMux project of the city of Munich
//
//========================================================================

#include "StructElement.h"
#include "StructTreeRoot.h"
#include "GlobalParams.h"
#include "UnicodeMap.h"
#include "PDFDoc.h"
#include "Dict.h"

#include <assert.h>

class GfxState;


static GBool isPlacementName(Object *value)
{
  return value->isName("Block")
      || value->isName("Inline")
      || value->isName("Before")
      || value->isName("Start")
      || value->isName("End");
}

static GBool isWritingModeName(Object *value)
{
  return value->isName("LrTb")
      || value->isName("RlTb")
      || value->isName("TbRl");
}

static GBool isBorderStyleName(Object *value)
{
  return value->isName("None")
      || value->isName("Hidden")
      || value->isName("Dotted")
      || value->isName("Dashed")
      || value->isName("Solid")
      || value->isName("Double")
      || value->isName("Groove")
      || value->isName("Ridge")
      || value->isName("Inset")
      || value->isName("Outset");
}

static GBool isTextAlignName(Object *value)
{
  return value->isName("Start")
      || value->isName("End")
      || value->isName("Center")
      || value->isName("Justify");
}

static GBool isBlockAlignName(Object *value)
{
  return value->isName("Before")
      || value->isName("Middle")
      || value->isName("After")
      || value->isName("Justify");
}

static GBool isInlineAlignName(Object *value)
{
  return value->isName("Start")
      || value->isName("End")
      || value->isName("Center");
}

static GBool isNumber(Object *value)
{
  return value->isNum();
}

static GBool isLineHeight(Object *value)
{
  return value->isName("Normal")
      || value->isName("Auto")
      || isNumber(value);
}

static GBool isTextDecorationName(Object *value)
{
  return value->isName("None")
      || value->isName("Underline")
      || value->isName("Overline")
      || value->isName("LineThrough");
}

static GBool isRubyAlignName(Object *value)
{
  return value->isName("Start")
      || value->isName("End")
      || value->isName("Center")
      || value->isName("Justify")
      || value->isName("Distribute");
}

static GBool isRubyPositionName(Object *value)
{
  return value->isName("Before")
      || value->isName("After")
      || value->isName("Warichu")
      || value->isName("Inline");
}

static GBool isGlyphOrientationName(Object *value)
{
  return value->isName("Auto")
      || value->isName("90")
      || value->isName("180")
      || value->isName("270")
      || value->isName("360")
      || value->isName("-90")
      || value->isName("-180");
}

static GBool isListNumberingName(Object *value)
{
  return value->isName("None")
      || value->isName("Disc")
      || value->isName("Circle")
      || value->isName("Square")
      || value->isName("Decimal")
      || value->isName("UpperRoman")
      || value->isName("LowerRoman")
      || value->isName("UpperAlpha")
      || value->isName("LowerAlpha");
}

static GBool isFieldRoleName(Object *value)
{
  return value->isName("rb")
      || value->isName("cb")
      || value->isName("pb")
      || value->isName("tv");
}

static GBool isFieldCheckedName(Object *value)
{
  return value->isName("on")
      || value->isName("off")
      || value->isName("neutral");
}

static GBool isTableScopeName(Object *value)
{
  return value->isName("Row")
      || value->isName("Column")
      || value->isName("Both");
}

static GBool isRGBColor(Object *value)
{
  if (!(value->isArray() && value->arrayGetLength() == 3))
    return gFalse;

  GBool okay = gTrue;
  for (int i = 0; i < 3; i++) {
    Object obj = value->arrayGet(i);
    if (!obj.isNum()) {
      okay = gFalse;
      break;
    }
    if (obj.getNum() < 0.0 || obj.getNum() > 1.0) {
      okay = gFalse;
      break;
    }
  }

  return okay;
}

static GBool isNatural(Object *value)
{
  return (value->isInt()   && value->getInt()   > 0)
      || (value->isInt64() && value->getInt64() > 0);
}

static GBool isPositive(Object *value)
{
  return value->isNum() && value->getNum() >= 0.0;
}

static GBool isNumberOrAuto(Object *value)
{
  return isNumber(value) || value->isName("Auto");
}

static GBool isTextString(Object *value)
{
  // XXX: Shall isName() also be checked?
  return value->isString();
}


#define ARRAY_CHECKER(name, checkItem, length, allowSingle, allowNulls) \
    static GBool name(Object *value) {                                  \
      if (!value->isArray())                                            \
        return allowSingle ? checkItem(value) : gFalse;                 \
                                                                        \
      if (length && value->arrayGetLength() != length)                  \
        return gFalse;                                                  \
                                                                        \
      GBool okay = gTrue;                                               \
      for (int i = 0; i < value->arrayGetLength(); i++) {               \
        Object obj = value->arrayGet(i);                                \
        if ((!allowNulls && obj.isNull()) || !checkItem(&obj)) {        \
          okay = gFalse;                                                \
          break;                                                        \
        }                                                               \
      }                                                                 \
      return okay;                                                      \
    }

ARRAY_CHECKER(isRGBColorOrOptionalArray4, isRGBColor,        4, gTrue,  gTrue )
ARRAY_CHECKER(isPositiveOrOptionalArray4, isPositive,        4, gTrue,  gTrue )
ARRAY_CHECKER(isPositiveOrArray4,         isPositive,        4, gTrue,  gFalse)
ARRAY_CHECKER(isBorderStyle,              isBorderStyleName, 4, gTrue,  gTrue )
ARRAY_CHECKER(isNumberArray4,             isNumber,          4, gFalse, gFalse)
ARRAY_CHECKER(isNumberOrArrayN,           isNumber,          0, gTrue,  gFalse)
ARRAY_CHECKER(isTableHeaders,             isTextString,      0, gFalse, gFalse)


// Type of functions used to do type-checking on attribute values
typedef GBool (*AttributeCheckFunc)(Object*);

// Maps attributes to their names and whether the attribute can be inherited.
struct AttributeMapEntry {
  Attribute::Type    type;
  const char        *name;
  const Object      *defval;
  GBool              inherit;
  AttributeCheckFunc check;
};

struct AttributeDefaults {
  AttributeDefaults() {}; // needed to support old clang

  Object Inline  = Object(objName, "Inline");
  Object LrTb = Object(objName, "LrTb");
  Object Normal = Object(objName, "Normal");
  Object Distribute = Object(objName, "Distribute");
  Object off = Object(objName, "off");
  Object Zero = Object(0.0);
  Object Auto = Object(objName, "Auto");
  Object Start = Object(objName, "Start");
  Object None = Object(objName, "None");
  Object Before = Object(objName, "Before");
  Object Nat1 = Object(1);
};

static const AttributeDefaults attributeDefaults;


#define ATTR_LIST_END \
  { Attribute::Unknown, nullptr, nullptr, gFalse, nullptr }

#define ATTR_WITH_DEFAULT(name, inherit, check, defval) \
  { Attribute::name,           \
    #name,                     \
    &attributeDefaults.defval, \
    inherit,                   \
    check }

#define ATTR(name, inherit, check) \
  { Attribute::name,           \
    #name,                     \
    nullptr,                   \
    inherit,                   \
    check }

static const AttributeMapEntry attributeMapCommonShared[] =
{
  ATTR_WITH_DEFAULT(Placement,       gFalse, isPlacementName, Inline),
  ATTR_WITH_DEFAULT(WritingMode,     gTrue,  isWritingModeName, LrTb),
  ATTR             (BackgroundColor, gFalse, isRGBColor),
  ATTR             (BorderColor,     gTrue,  isRGBColorOrOptionalArray4),
  ATTR_WITH_DEFAULT(BorderStyle,     gFalse, isBorderStyle, None),
  ATTR             (BorderThickness, gTrue,  isPositiveOrOptionalArray4),
  ATTR_WITH_DEFAULT(Padding,         gFalse, isPositiveOrArray4, Zero),
  ATTR             (Color,           gTrue,  isRGBColor),
  ATTR_LIST_END
};

static const AttributeMapEntry attributeMapCommonBlock[] =
{
  ATTR_WITH_DEFAULT(SpaceBefore, gFalse, isPositive, Zero),
  ATTR_WITH_DEFAULT(SpaceAfter,  gFalse, isPositive, Zero),
  ATTR_WITH_DEFAULT(StartIndent, gTrue,  isNumber,   Zero),
  ATTR_WITH_DEFAULT(EndIndent,   gTrue,  isNumber,   Zero),
  ATTR_WITH_DEFAULT(TextIndent,  gTrue,  isNumber,   Zero),
  ATTR_WITH_DEFAULT(TextAlign,   gTrue,  isTextAlignName, Start),
  ATTR             (BBox,        gFalse, isNumberArray4),
  ATTR_WITH_DEFAULT(Width,       gFalse, isNumberOrAuto, Auto),
  ATTR_WITH_DEFAULT(Height,      gFalse, isNumberOrAuto, Auto),
  ATTR_WITH_DEFAULT(BlockAlign,  gTrue,  isBlockAlignName, Before),
  ATTR_WITH_DEFAULT(InlineAlign, gTrue,  isInlineAlignName, Start),
  ATTR_LIST_END
};

static const AttributeMapEntry attributeMapCommonInline[] =
{
  ATTR_WITH_DEFAULT(BaselineShift,            gFalse, isNumber, Zero),
  ATTR_WITH_DEFAULT(LineHeight,               gTrue,  isLineHeight, Normal),
  ATTR             (TextDecorationColor,      gTrue,  isRGBColor),
  ATTR             (TextDecorationThickness,  gTrue,  isPositive),
  ATTR_WITH_DEFAULT(TextDecorationType,       gFalse, isTextDecorationName, None),
  ATTR_WITH_DEFAULT(GlyphOrientationVertical, gTrue,  isGlyphOrientationName, Auto),
  ATTR_LIST_END
};

static const AttributeMapEntry attributeMapCommonRubyText[] =
{
  ATTR_WITH_DEFAULT(RubyPosition, gTrue, isRubyPositionName, Before),
  ATTR_WITH_DEFAULT(RubyAlign,    gTrue, isRubyAlignName, Distribute),
  ATTR_LIST_END
};

static const AttributeMapEntry attributeMapCommonColumns[] =
{
  ATTR_WITH_DEFAULT(ColumnCount,  gFalse, isNatural, Nat1),
  ATTR             (ColumnGap,    gFalse, isNumberOrArrayN),
  ATTR             (ColumnWidths, gFalse, isNumberOrArrayN),
  ATTR_LIST_END
};

static const AttributeMapEntry attributeMapCommonList[] = {
  ATTR_WITH_DEFAULT(ListNumbering, gTrue, isListNumberingName, None),
  ATTR_LIST_END
};

static const AttributeMapEntry attributeMapCommonPrintField[] =
{
  ATTR             (Role,    gFalse, isFieldRoleName),
  ATTR_WITH_DEFAULT(checked, gFalse, isFieldCheckedName, off),
  ATTR             (Desc,    gFalse, isTextString),
  ATTR_LIST_END
};

static const AttributeMapEntry attributeMapCommonTable[] =
{
  ATTR(Headers, gFalse, isTableHeaders),
  ATTR(Scope,   gFalse, isTableScopeName),
  ATTR(Summary, gFalse, isTextString),
  ATTR_LIST_END
};

static const AttributeMapEntry attributeMapCommonTableCell[] =
{
  ATTR_WITH_DEFAULT(RowSpan,      gFalse, isNatural, Nat1),
  ATTR_WITH_DEFAULT(ColSpan,      gFalse, isNatural, Nat1),
  ATTR_WITH_DEFAULT(TBorderStyle, gTrue,  isBorderStyle, None),
  ATTR_WITH_DEFAULT(TPadding,     gTrue,  isPositiveOrArray4, Zero),
  ATTR_LIST_END
};

#undef ATTR_WITH_DEFAULT
#undef ATTR


static const AttributeMapEntry *attributeMapAll[] = {
  attributeMapCommonShared,
  attributeMapCommonBlock,
  attributeMapCommonInline,
  attributeMapCommonRubyText,
  attributeMapCommonColumns,
  attributeMapCommonList,
  attributeMapCommonPrintField,
  attributeMapCommonTable,
  attributeMapCommonTableCell,
  nullptr,
};

static const AttributeMapEntry *attributeMapShared[] = {
  attributeMapCommonShared,
  nullptr,
};

static const AttributeMapEntry *attributeMapBlock[] = {
  attributeMapCommonShared,
  attributeMapCommonBlock,
  nullptr,
};

static const AttributeMapEntry *attributeMapInline[] = {
  attributeMapCommonShared,
  attributeMapCommonInline,
  nullptr,
};

static const AttributeMapEntry *attributeMapTableCell[] = {
  attributeMapCommonShared,
  attributeMapCommonBlock,
  attributeMapCommonTable,
  attributeMapCommonTableCell,
  nullptr,
};

static const AttributeMapEntry *attributeMapRubyText[] = {
  attributeMapCommonShared,
  attributeMapCommonInline,
  attributeMapCommonRubyText,
  nullptr,
};

static const AttributeMapEntry *attributeMapColumns[] = {
  attributeMapCommonShared,
  attributeMapCommonInline,
  attributeMapCommonColumns,
  nullptr,
};

static const AttributeMapEntry *attributeMapList[] = {
  attributeMapCommonShared,
  attributeMapCommonList,
  nullptr,
};

static const AttributeMapEntry *attributeMapTable[] = {
  attributeMapCommonShared,
  attributeMapCommonBlock,
  attributeMapCommonTable,
  nullptr,
};

static const AttributeMapEntry *attributeMapIllustration[] = {
  // XXX: Illustrations may have some attributes from the "shared", "inline",
  //      the "block" sets. This is a loose specification; making it better
  //      means duplicating entries from the sets. This seems good enough...
  attributeMapCommonShared,
  attributeMapCommonBlock,
  attributeMapCommonInline,
  nullptr,
};

// Table mapping owners of attributes to their names.
static const struct OwnerMapEntry {
  Attribute::Owner owner;
  const char      *name;
} ownerMap[] = {
  // XXX: Those are sorted in the owner priority resolution order. If the
  //      same attribute is defined with two owners, the order in the table
  //      can be used to know which one has more priority.
  { Attribute::XML_1_00,       "XML-1.00"       },
  { Attribute::HTML_3_20,      "HTML-3.20"      },
  { Attribute::HTML_4_01,      "HTML-4.01"      },
  { Attribute::OEB_1_00,       "OEB-1.00"       },
  { Attribute::RTF_1_05,       "RTF-1.05"       },
  { Attribute::CSS_1_00,       "CSS-1.00"       },
  { Attribute::CSS_2_00,       "CSS-2.00"       },
  { Attribute::Layout,         "Layout"         },
  { Attribute::PrintField,     "PrintField"     },
  { Attribute::Table,          "Table"          },
  { Attribute::List,           "List"           },
  { Attribute::UserProperties, "UserProperties" },
};


static GBool ownerHasMorePriority(Attribute::Owner a, Attribute::Owner b)
{
  unsigned aIndex, bIndex;

  for (unsigned i = aIndex = bIndex = 0; i < sizeof(ownerMap) / sizeof(ownerMap[0]); i++) {
    if (ownerMap[i].owner == a)
      aIndex = i;
    if (ownerMap[i].owner == b)
      bIndex = i;
  }

  return aIndex < bIndex;
}


// Maps element types to their names and also serves as lookup table
// for additional element type attributes.

enum ElementType {
  elementTypeUndefined,
  elementTypeGrouping,
  elementTypeInline,
  elementTypeBlock,
};

static const struct TypeMapEntry {
  StructElement::Type       type;
  const char               *name;
  ElementType               elementType;
  const AttributeMapEntry **attributes;
} typeMap[] = {
  { StructElement::Document,   "Document",   elementTypeGrouping,  attributeMapShared       },
  { StructElement::Part,       "Part",       elementTypeGrouping,  attributeMapShared       },
  { StructElement::Art,        "Art",        elementTypeGrouping,  attributeMapColumns      },
  { StructElement::Sect,       "Sect",       elementTypeGrouping,  attributeMapColumns      },
  { StructElement::Div,        "Div",        elementTypeGrouping,  attributeMapColumns      },
  { StructElement::BlockQuote, "BlockQuote", elementTypeGrouping,  attributeMapInline       },
  { StructElement::Caption,    "Caption",    elementTypeGrouping,  attributeMapInline       },
  { StructElement::NonStruct,  "NonStruct",  elementTypeGrouping,  attributeMapInline       },
  { StructElement::Index,      "Index",      elementTypeGrouping,  attributeMapInline       },
  { StructElement::Private,    "Private",    elementTypeGrouping,  attributeMapInline       },
  { StructElement::Span,       "Span",       elementTypeInline,    attributeMapInline       },
  { StructElement::Quote,      "Quote",      elementTypeInline,    attributeMapInline       },
  { StructElement::Note,       "Note",       elementTypeInline,    attributeMapInline       },
  { StructElement::Reference,  "Reference",  elementTypeInline,    attributeMapInline       },
  { StructElement::BibEntry,   "BibEntry",   elementTypeInline,    attributeMapInline       },
  { StructElement::Code,       "Code",       elementTypeInline,    attributeMapInline       },
  { StructElement::Link,       "Link",       elementTypeInline,    attributeMapInline       },
  { StructElement::Annot,      "Annot",      elementTypeInline,    attributeMapInline       },
  { StructElement::Ruby,       "Ruby",       elementTypeInline,    attributeMapRubyText     },
  { StructElement::RB,         "RB",         elementTypeUndefined, attributeMapRubyText     },
  { StructElement::RT,         "RT",         elementTypeUndefined, attributeMapRubyText     },
  { StructElement::RP,         "RP",         elementTypeUndefined, attributeMapShared       },
  { StructElement::Warichu,    "Warichu",    elementTypeInline,    attributeMapRubyText     },
  { StructElement::WT,         "WT",         elementTypeUndefined, attributeMapShared       },
  { StructElement::WP,         "WP",         elementTypeUndefined, attributeMapShared       },
  { StructElement::P,          "P",          elementTypeBlock,     attributeMapBlock        },
  { StructElement::H,          "H",          elementTypeBlock,     attributeMapBlock        },
  { StructElement::H1,         "H1",         elementTypeBlock,     attributeMapBlock        },
  { StructElement::H2,         "H2",         elementTypeBlock,     attributeMapBlock        },
  { StructElement::H3,         "H3",         elementTypeBlock,     attributeMapBlock        },
  { StructElement::H4,         "H4",         elementTypeBlock,     attributeMapBlock        },
  { StructElement::H5,         "H5",         elementTypeBlock,     attributeMapBlock        },
  { StructElement::H6,         "H6",         elementTypeBlock,     attributeMapBlock        },
  { StructElement::L,          "L",          elementTypeBlock,     attributeMapList         },
  { StructElement::LI,         "LI",         elementTypeBlock,     attributeMapBlock        },
  { StructElement::Lbl,        "Lbl",        elementTypeBlock,     attributeMapBlock        },
  { StructElement::LBody,      "LBody",      elementTypeBlock,     attributeMapBlock        },
  { StructElement::Table,      "Table",      elementTypeBlock,     attributeMapTable        },
  { StructElement::TR,         "TR",         elementTypeUndefined, attributeMapShared       },
  { StructElement::TH,         "TH",         elementTypeUndefined, attributeMapTableCell    },
  { StructElement::TD,         "TD",         elementTypeUndefined, attributeMapTableCell    },
  { StructElement::THead,      "THead",      elementTypeUndefined, attributeMapShared       },
  { StructElement::TFoot,      "TFoot",      elementTypeUndefined, attributeMapShared       },
  { StructElement::TBody,      "TBody",      elementTypeUndefined, attributeMapShared       },
  { StructElement::Figure,     "Figure",     elementTypeUndefined, attributeMapIllustration },
  { StructElement::Formula,    "Formula",    elementTypeUndefined, attributeMapIllustration },
  { StructElement::Form,       "Form",       elementTypeUndefined, attributeMapIllustration },
  { StructElement::TOC,        "TOC",        elementTypeGrouping,  attributeMapShared       },
  { StructElement::TOCI,       "TOCI",       elementTypeGrouping,  attributeMapShared       },
};


//------------------------------------------------------------------------
// Helpers for the attribute and structure type tables
//------------------------------------------------------------------------

static inline const AttributeMapEntry *
getAttributeMapEntry(const AttributeMapEntry **entryList, Attribute::Type type)
{
  assert(entryList);
  while (*entryList) {
    const AttributeMapEntry *entry = *entryList;
    while (entry->type != Attribute::Unknown) {
      assert(entry->name);
      if (type == entry->type)
        return entry;
      entry++;
    }
    entryList++;
  }
  return nullptr;
}

static inline const AttributeMapEntry *
getAttributeMapEntry(const AttributeMapEntry **entryList, const char *name)
{
  assert(entryList);
  while (*entryList) {
    const AttributeMapEntry *entry = *entryList;
    while (entry->type != Attribute::Unknown) {
      assert(entry->name);
      if (strcmp(name, entry->name) == 0)
        return entry;
      entry++;
    }
    entryList++;
  }
  return nullptr;
}

static inline const OwnerMapEntry *getOwnerMapEntry(Attribute::Owner owner)
{
  for (unsigned i = 0; i < sizeof(ownerMap) / sizeof(ownerMap[0]); i++) {
    if (owner == ownerMap[i].owner)
      return &ownerMap[i];
  }
  return nullptr;
}

static inline const OwnerMapEntry *getOwnerMapEntry(const char *name)
{
  for (unsigned i = 0; i < sizeof(ownerMap) / sizeof(ownerMap[0]); i++) {
    if (strcmp(name, ownerMap[i].name) == 0)
      return &ownerMap[i];
  }
  return nullptr;
}

static const char *ownerToName(Attribute::Owner owner)
{
  const OwnerMapEntry *entry = getOwnerMapEntry(owner);
  return entry ? entry->name : "UnknownOwner";
}

static Attribute::Owner nameToOwner(const char *name)
{
  const OwnerMapEntry *entry = getOwnerMapEntry(name);
  return entry ? entry->owner : Attribute::UnknownOwner;
}

static inline const TypeMapEntry *getTypeMapEntry(StructElement::Type type)
{
  for (unsigned i = 0; i < sizeof(typeMap) / sizeof(typeMap[0]); i++) {
    if (type == typeMap[i].type)
      return &typeMap[i];
  }
  return nullptr;
}

static inline const TypeMapEntry *getTypeMapEntry(const char *name)
{
  for (unsigned i = 0; i < sizeof(typeMap) / sizeof(typeMap[0]); i++) {
    if (strcmp(name, typeMap[i].name) == 0)
      return &typeMap[i];
  }
  return nullptr;
}

static const char *typeToName(StructElement::Type type)
{
  if (type == StructElement::MCID)
    return "MarkedContent";
  if (type == StructElement::OBJR)
    return "ObjectReference";

  const TypeMapEntry *entry = getTypeMapEntry(type);
  return entry ? entry->name : "Unknown";
}

static StructElement::Type nameToType(const char *name)
{
  const TypeMapEntry *entry = getTypeMapEntry(name);
  return entry ? entry->type : StructElement::Unknown;
}


//------------------------------------------------------------------------
// Attribute
//------------------------------------------------------------------------

Attribute::Attribute(const char *nameA, int nameLenA, Object *valueA):
  type(UserProperty),
  owner(UserProperties),
  revision(0),
  name(nameA, nameLenA),
  value(),
  hidden(gFalse),
  formatted(nullptr)
{
  assert(valueA);
  value = valueA->copy();
}

Attribute::Attribute(Type typeA, Object *valueA):
  type(typeA),
  owner(UserProperties), // TODO: Determine corresponding owner from Type
  revision(0),
  name(),
  value(),
  hidden(gFalse),
  formatted(nullptr)
{
  assert(valueA);

  value = valueA->copy();

  if (!checkType())
    type = Unknown;
}

Attribute::~Attribute()
{
  delete formatted;
}

const char *Attribute::getTypeName() const
{
  if (type == UserProperty)
    return name.getCString();

  const AttributeMapEntry *entry = getAttributeMapEntry(attributeMapAll, type);
  if (entry)
    return entry->name;

  return "Unknown";
}

const char *Attribute::getOwnerName() const
{
  return ownerToName(owner);
}

Object *Attribute::getDefaultValue(Attribute::Type type)
{
  const AttributeMapEntry *entry = getAttributeMapEntry(attributeMapAll, type);
  return entry ? const_cast<Object*>(entry->defval) : nullptr;
}

void Attribute::setFormattedValue(const char *formattedA)
{
  if (formattedA) {
    if (formatted)
      formatted->Set(formattedA);
    else
      formatted = new GooString(formattedA);
  } else {
    delete formatted;
    formatted = nullptr;
  }
}

GBool Attribute::checkType(StructElement *element)
{
  // If an element is passed, tighther type-checking can be done.
  if (!element)
    return gTrue;

  const TypeMapEntry *elementTypeEntry = getTypeMapEntry(element->getType());
  if (elementTypeEntry && elementTypeEntry->attributes) {
    const AttributeMapEntry *entry = getAttributeMapEntry(elementTypeEntry->attributes, type);
    if (entry) {
      if (entry->check && !((*entry->check)(&value))) {
        return gFalse;
      }
    } else {
      // No entry: the attribute is not valid for the containing element.
      return gFalse;
    }
  }

  return gTrue;
}

Attribute::Type Attribute::getTypeForName(const char *name, StructElement *element)
{
  const AttributeMapEntry **attributes = attributeMapAll;
  if (element) {
    const TypeMapEntry *elementTypeEntry = getTypeMapEntry(element->getType());
    if (elementTypeEntry && elementTypeEntry->attributes) {
      attributes = elementTypeEntry->attributes;
    }
  }

  const AttributeMapEntry *entry = getAttributeMapEntry(attributes, name);
  return entry ? entry->type : Unknown;
}

Attribute *Attribute::parseUserProperty(Dict *property)
{
  Object obj, value;
  const char *name = nullptr;
  int nameLen = GooString::CALC_STRING_LEN;

  obj = property->lookup("N");
  if (obj.isString()) {
    const GooString *s = obj.getString();
    name = s->getCString();
    nameLen = s->getLength();
  } else if (obj.isName())
    name = obj.getName();
  else {
    error(errSyntaxError, -1, "N object is wrong type ({0:s})", obj.getTypeName());
    return nullptr;
  }

  value = property->lookup("V");
  if (value.isNull()) {
    error(errSyntaxError, -1, "V object is wrong type ({0:s})", value.getTypeName());
    return nullptr;
  }

  Attribute *attribute = new Attribute(name, nameLen, &value);
  obj = property->lookup("F");
  if (obj.isString()) {
    attribute->setFormattedValue(obj.getString()->getCString());
  } else if (!obj.isNull()) {
    error(errSyntaxWarning, -1, "F object is wrong type ({0:s})", obj.getTypeName());
  }

  obj = property->lookup("H");
  if (obj.isBool()) {
    attribute->setHidden(obj.getBool());
  } else if (!obj.isNull()) {
    error(errSyntaxWarning, -1, "H object is wrong type ({0:s})", obj.getTypeName());
  }

  return attribute;
}


//------------------------------------------------------------------------
// StructElement
//------------------------------------------------------------------------

StructElement::StructData::StructData():
  altText(nullptr),
  actualText(nullptr),
  id(nullptr),
  title(nullptr),
  expandedAbbr(nullptr),
  language(nullptr),
  revision(0)
{
}

StructElement::StructData::~StructData()
{
  delete altText;
  delete actualText;
  delete id;
  delete title;
  delete language;
  for (ElemPtrArray::iterator i = elements.begin(); i != elements.end(); ++i) delete *i;
  for (AttrPtrArray::iterator i = attributes.begin(); i != attributes.end(); ++i) delete *i;
}


StructElement::StructElement(Dict *element,
                             StructTreeRoot *treeRootA,
                             StructElement *parentA,
                             std::set<int> &seen):
  type(Unknown),
  treeRoot(treeRootA),
  parent(parentA),
  s(new StructData())
{
  assert(treeRoot);
  assert(element);

  parse(element);
  parseChildren(element, seen);
}

StructElement::StructElement(int mcid, StructTreeRoot *treeRootA, StructElement *parentA):
  type(MCID),
  treeRoot(treeRootA),
  parent(parentA),
  c(new ContentData(mcid))
{
  assert(treeRoot);
  assert(parent);
}

StructElement::StructElement(const Ref& ref, StructTreeRoot *treeRootA, StructElement *parentA):
  type(OBJR),
  treeRoot(treeRootA),
  parent(parentA),
  c(new ContentData(ref))
{
  assert(treeRoot);
  assert(parent);
}

StructElement::~StructElement()
{
  if (isContent())
    delete c;
  else
    delete s;
}

GBool StructElement::isBlock() const
{
  const TypeMapEntry *entry = getTypeMapEntry(type);
  return entry ? (entry->elementType == elementTypeBlock) : gFalse;
}

GBool StructElement::isInline() const
{
  const TypeMapEntry *entry = getTypeMapEntry(type);
  return entry ? (entry->elementType == elementTypeInline) : gFalse;
}

GBool StructElement::isGrouping() const
{
  const TypeMapEntry *entry = getTypeMapEntry(type);
  return entry ? (entry->elementType == elementTypeGrouping) : gFalse;
}

GBool StructElement::hasPageRef() const
{
  return pageRef.isRef() || (parent && parent->hasPageRef());
}

bool StructElement::getPageRef(Ref& ref) const
{
  if (pageRef.isRef()) {
    ref = pageRef.getRef();
    return gTrue;
  }

  if (parent)
    return parent->getPageRef(ref);

  return gFalse;
}

const char *StructElement::getTypeName() const
{
  return typeToName(type);
}

const Attribute *StructElement::findAttribute(Attribute::Type attributeType, GBool inherit,
                                              Attribute::Owner attributeOwner) const
{
  if (isContent())
    return parent->findAttribute(attributeType, inherit, attributeOwner);

  if (attributeType == Attribute::Unknown || attributeType == Attribute::UserProperty)
    return nullptr;

  const Attribute *result = nullptr;

  if (attributeOwner == Attribute::UnknownOwner) {
    // Search for the attribute, no matter who the owner is
    for (unsigned i = 0; i < getNumAttributes(); i++) {
      const Attribute *attr = getAttribute(i);
      if (attributeType == attr->getType()) {
        if (!result || ownerHasMorePriority(attr->getOwner(), result->getOwner()))
          result = attr;
      }
    }
  } else {
    // Search for the attribute, with a specific owner
    for (unsigned i = 0; i < getNumAttributes(); i++) {
      const Attribute *attr = getAttribute(i);
      if (attributeType == attr->getType() && attributeOwner == attr->getOwner()) {
        result = attr;
        break;
      }
    }
  }

  if (result)
    return result;

  if (inherit && parent) {
    const AttributeMapEntry *entry = getAttributeMapEntry(attributeMapAll, attributeType);
    assert(entry);
    // TODO: Take into account special inheritance cases, for example:
    //       inline elements which have been changed to be block using
    //       "/Placement/Block" have slightly different rules.
    if (entry->inherit)
      return parent->findAttribute(attributeType, inherit, attributeOwner);
  }

  return nullptr;
}

GooString* StructElement::appendSubTreeText(GooString *string, GBool recursive) const
{
  if (isContent() && !isObjectRef()) {
    MarkedContentOutputDev mcdev(getMCID());
    const TextSpanArray& spans(getTextSpansInternal(mcdev));

    if (!string)
      string = new GooString();

    for (TextSpanArray::const_iterator i = spans.begin(); i != spans.end(); ++i)
      string->append(i->getText());

    return string;
  }

  if (!recursive)
    return nullptr;

  // Do a depth-first traversal, to get elements in logical order
  if (!string)
    string = new GooString();

  for (unsigned i = 0; i < getNumChildren(); i++)
    getChild(i)->appendSubTreeText(string, recursive);

  return string;
}

const TextSpanArray& StructElement::getTextSpansInternal(MarkedContentOutputDev& mcdev) const
{
  assert(isContent());

  int startPage = 0, endPage = 0;

  Ref ref;
  if (getPageRef(ref)) {
    startPage = endPage = treeRoot->getDoc()->findPage(ref.num, ref.gen);
  }

  if (!(startPage && endPage)) {
    startPage = 1;
    endPage = treeRoot->getDoc()->getNumPages();
  }

  treeRoot->getDoc()->displayPages(&mcdev, startPage, endPage, 72.0, 72.0, 0, gTrue, gFalse, gFalse);
  return mcdev.getTextSpans();
}

static StructElement::Type roleMapResolve(Dict *roleMap, const char *name, const char *curName)
{
  // Circular reference
  if (curName && !strcmp(name, curName))
    return StructElement::Unknown;

  Object resolved = roleMap->lookup(curName ? curName : name);
  if (resolved.isName()) {
    StructElement::Type type = nameToType(resolved.getName());
    return type == StructElement::Unknown
      ? roleMapResolve(roleMap, name, resolved.getName())
      : type;
  }

  if (!resolved.isNull())
    error(errSyntaxWarning, -1, "RoleMap entry is wrong type ({0:s})", resolved.getTypeName());
  return StructElement::Unknown;
}

void StructElement::parse(Dict *element)
{
  Object obj;

  // Type is optional, but if present must be StructElem
  obj = element->lookup("Type");
  if (!obj.isNull() && !obj.isName("StructElem")) {
    error(errSyntaxError, -1, "Type of StructElem object is wrong");
    return;
  }

  // Parent object reference (required).
  s->parentRef = element->lookupNF("P");
  if (!s->parentRef.isRef()) {
    error(errSyntaxError, -1, "P object is wrong type ({0:s})", obj.getTypeName());
    return;
  }

  // Check whether the S-type is valid for the top level
  // element and create a node of the appropriate type.
  obj = element->lookup("S");
  if (!obj.isName()) {
    error(errSyntaxError, -1, "S object is wrong type ({0:s})", obj.getTypeName());
    return;
  }

  // Type name may not be standard, resolve through RoleMap first.
  if (treeRoot->getRoleMap()) {
    type = roleMapResolve(treeRoot->getRoleMap(), obj.getName(), nullptr);
  }

  // Resolving through RoleMap may leave type as Unknown, e.g. for types
  // which are not present in it, yet they are standard element types.
  if (type == Unknown)
    type = nameToType(obj.getName());

  // At this point either the type name must have been resolved.
  if (type == Unknown) {
    error(errSyntaxError, -1, "StructElem object is wrong type ({0:s})", obj.getName());
    return;
  }

  // Object ID (optional), to be looked at the IDTree in the tree root.
  obj = element->lookup("ID");
  if (obj.isString()) {
    s->id = obj.takeString();
  }

  // Page reference (optional) in which at least one of the child items
  // is to be rendered in. Note: each element stores only the /Pg value
  // contained by it, and StructElement::getPageRef() may look in parent
  // elements to find the page where an element belongs.
  pageRef = element->lookupNF("Pg");

  // Revision number (optional).
  obj = element->lookup("R");
  if (obj.isInt()) {
    s->revision = obj.getInt();
  }

  // Element title (optional).
  obj = element->lookup("T");
  if (obj.isString()) {
    s->title = obj.takeString();
  }

  // Language (optional).
  obj = element->lookup("Lang");
  if (obj.isString()) {
    s->language = obj.takeString();
  }

  // Alternative text (optional).
  obj = element->lookup("Alt");
  if (obj.isString()) {
    s->altText = obj.takeString();
  }

  // Expanded form of an abbreviation (optional).
  obj = element->lookup("E");
  if (obj.isString()) {
    s->expandedAbbr = obj.takeString();
  }

  // Actual text (optional).
  obj = element->lookup("ActualText");
  if (obj.isString()) {
    s->actualText = obj.takeString();
  }

  // Attributes directly attached to the element (optional).
  obj = element->lookup("A");
  if (obj.isDict()) {
    parseAttributes(obj.getDict());
  } else if (obj.isArray()) {
    unsigned attrIndex = getNumAttributes();
    for (int i = 0; i < obj.arrayGetLength(); i++) {
      Object iobj = obj.arrayGet(i);
      if (iobj.isDict()) {
        attrIndex = getNumAttributes();
        parseAttributes(iobj.getDict());
      } else if (iobj.isInt()) {
        const int revision = iobj.getInt();
        // Set revision numbers for the elements previously created.
        for (unsigned j = attrIndex; j < getNumAttributes(); j++)
          getAttribute(j)->setRevision(revision);
      } else {
        error(errSyntaxWarning, -1, "A item is wrong type ({0:s})", iobj.getTypeName());
      }
    }
  } else if (!obj.isNull()) {
    error(errSyntaxWarning, -1, "A is wrong type ({0:s})", obj.getTypeName());
  }

  // Attributes referenced indirectly through the ClassMap (optional).
  if (treeRoot->getClassMap()) {
    Object classes = element->lookup("C");
    if (classes.isName()) {
      Object attr = treeRoot->getClassMap()->lookup(classes.getName());
      if (attr.isDict()) {
        parseAttributes(attr.getDict(), gTrue);
      } else if (attr.isArray()) {
        for (int i = 0; i < attr.arrayGetLength(); i++) {
          unsigned attrIndex = getNumAttributes();
          Object iobj = attr.arrayGet(i);
          if (iobj.isDict()) {
            attrIndex = getNumAttributes();
            parseAttributes(iobj.getDict(), gTrue);
          } else if (iobj.isInt()) {
            // Set revision numbers for the elements previously created.
            const int revision = iobj.getInt();
            for (unsigned j = attrIndex; j < getNumAttributes(); j++)
              getAttribute(j)->setRevision(revision);
          } else {
            error(errSyntaxWarning, -1, "C item is wrong type ({0:s})", iobj.getTypeName());
          }
        }
      } else if (!attr.isNull()) {
        error(errSyntaxWarning, -1, "C object is wrong type ({0:s})", classes.getTypeName());
      }
    }
  }
}

StructElement *StructElement::parseChild(Object *ref,
                                         Object *childObj,
                                         std::set<int> &seen)
{
  assert(childObj);
  assert(ref);

  StructElement *child = nullptr;

  if (childObj->isInt()) {
    child = new StructElement(childObj->getInt(), treeRoot, this);
  } else if (childObj->isDict("MCR")) {
    /*
     * TODO: The optional Stm/StwOwn attributes are not handled, so all the
     *      page will be always scanned when calling StructElement::getText().
     */
    Object mcidObj = childObj->dictLookup("MCID");
    if (!mcidObj.isInt()) {
      error(errSyntaxError, -1, "MCID object is wrong type ({0:s})", mcidObj.getTypeName());
      return nullptr;
    }

    child = new StructElement(mcidObj.getInt(), treeRoot, this);

    Object pageRefObj = childObj->dictLookupNF("Pg");
    if (pageRefObj.isRef()) {
      child->pageRef = std::move(pageRefObj);
    }
  } else if (childObj->isDict("OBJR")) {
    Object refObj = childObj->dictLookupNF("Obj");
    if (refObj.isRef()) {

      child = new StructElement(refObj.getRef(), treeRoot, this);

      Object pageRefObj = childObj->dictLookupNF("Pg");
      if (pageRefObj.isRef()) {
        child->pageRef = std::move(pageRefObj);
      }
    } else {
      error(errSyntaxError, -1, "Obj object is wrong type ({0:s})", refObj.getTypeName());
    }
  } else if (childObj->isDict()) {
    if (!ref->isRef()) {
      error(errSyntaxError, -1,
            "Structure element dictionary is not an indirect reference ({0:s})",
            ref->getTypeName());
    } else if (seen.find(ref->getRefNum()) == seen.end()) {
      seen.insert(ref->getRefNum());
      child = new StructElement(childObj->getDict(), treeRoot, this, seen);
    } else {
      error(errSyntaxWarning, -1,
            "Loop detected in structure tree, skipping subtree at object {0:d}:{1:d}",
            ref->getRefNum(), ref->getRefGen());
    }
  } else {
    error(errSyntaxWarning, -1, "K has a child of wrong type ({0:s})", childObj->getTypeName());
  }

  if (child) {
    if (child->isOk()) {
      appendChild(child);
      if (ref->isRef())
        treeRoot->parentTreeAdd(ref->getRef(), child);
    } else {
      delete child;
      child = nullptr;
    }
  }

  return child;
}

void StructElement::parseChildren(Dict *element, std::set<int> &seen)
{
  Object kids = element->lookup("K");
  if (kids.isArray()) {
    for (int i = 0; i < kids.arrayGetLength(); i++) {
      Object obj = kids.arrayGet(i);
      Object ref = kids.arrayGetNF(i);
      parseChild(&ref, &obj, seen);
    }
  } else if (kids.isDict() || kids.isInt()) {
    Object ref = element->lookupNF("K");
    parseChild(&ref, &kids, seen);
  }
}

void StructElement::parseAttributes(Dict *attributes, GBool keepExisting)
{
  Object owner = attributes->lookup("O");
  if (owner.isName("UserProperties")) {
    // In this case /P is an array of UserProperty dictionaries
    Object userProperties = attributes->lookup("P");
    if (userProperties.isArray()) {
      for (int i = 0; i < userProperties.arrayGetLength(); i++) {
        Object property = userProperties.arrayGet(i);
        if (property.isDict()) {
          Attribute *attribute = Attribute::parseUserProperty(property.getDict());
          if (attribute && attribute->isOk()) {
            appendAttribute(attribute);
          } else {
            error(errSyntaxWarning, -1, "Item in P is invalid");
            delete attribute;
          }
        } else {
          error(errSyntaxWarning, -1, "Item in P is wrong type ({0:s})", property.getTypeName());
        }
      }
    }
  } else if (owner.isName()) {
    // In this case /P contains standard attributes.
    // Check first if the owner is a valid standard one.
    Attribute::Owner ownerValue = nameToOwner(owner.getName());
    if (ownerValue != Attribute::UnknownOwner) {
      // Iterate over the entries of the "attributes" dictionary.
      // The /O entry (owner) is skipped.
      for (int i = 0; i < attributes->getLength(); i++) {
        const char *key = attributes->getKey(i);
        if (strcmp(key, "O") != 0) {
          Attribute::Type type = Attribute::getTypeForName(key, this);

          // Check if the attribute is already defined.
          if (keepExisting) {
            GBool exists = gFalse;
            for (unsigned j = 0; j < getNumAttributes(); j++) {
              if (getAttribute(j)->getType() == type) {
                exists = gTrue;
                break;
              }
            }
            if (exists)
              continue;
          }

          if (type != Attribute::Unknown) {
            Object value = attributes->getVal(i);
            GBool typeCheckOk = gTrue;
            Attribute *attribute = new Attribute(type, &value);

            if (attribute->isOk() && (typeCheckOk = attribute->checkType(this))) {
              appendAttribute(attribute);
            } else {
              // It is not needed to free "value", the Attribute instance
              // owns the contents, so deleting "attribute" is enough.
              if (!typeCheckOk) {
                error(errSyntaxWarning, -1, "Attribute {0:s} value is of wrong type ({1:s})",
                      attribute->getTypeName(), attribute->getValue()->getTypeName());
              }
              delete attribute;
            }
          } else {
            error(errSyntaxWarning, -1, "Wrong Attribute '{0:s}' in element {1:s}", key, getTypeName());
          }
        }
      }
    } else {
      error(errSyntaxWarning, -1, "O object is invalid value ({0:s})", owner.getName());
    }
  } else if (!owner.isNull()) {
    error(errSyntaxWarning, -1, "O is wrong type ({0:s})", owner.getTypeName());
  }
}
