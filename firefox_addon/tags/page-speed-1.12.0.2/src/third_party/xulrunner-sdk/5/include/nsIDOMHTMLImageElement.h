/*
 * DO NOT EDIT.  THIS FILE IS GENERATED FROM /builds/slave/aurora-lnx64-xr/build/dom/interfaces/html/nsIDOMHTMLImageElement.idl
 */

#ifndef __gen_nsIDOMHTMLImageElement_h__
#define __gen_nsIDOMHTMLImageElement_h__


#ifndef __gen_nsIDOMHTMLElement_h__
#include "nsIDOMHTMLElement.h"
#endif

/* For IDL files that don't want to include root IDL files. */
#ifndef NS_NO_VTABLE
#define NS_NO_VTABLE
#endif

/* starting interface:    nsIDOMHTMLImageElement */
#define NS_IDOMHTMLIMAGEELEMENT_IID_STR "c4ef8a40-dd56-4b95-a007-630a0ac04341"

#define NS_IDOMHTMLIMAGEELEMENT_IID \
  {0xc4ef8a40, 0xdd56, 0x4b95, \
    { 0xa0, 0x07, 0x63, 0x0a, 0x0a, 0xc0, 0x43, 0x41 }}

/**
 * The nsIDOMHTMLImageElement interface is the interface to a [X]HTML
 * img element.
 *
 * This interface is trying to follow the DOM Level 2 HTML specification:
 * http://www.w3.org/TR/DOM-Level-2-HTML/
 *
 * with changes from the work-in-progress WHATWG HTML specification:
 * http://www.whatwg.org/specs/web-apps/current-work/
 */
class NS_NO_VTABLE NS_SCRIPTABLE nsIDOMHTMLImageElement : public nsIDOMHTMLElement {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDOMHTMLIMAGEELEMENT_IID)

  /* attribute DOMString alt; */
  NS_SCRIPTABLE NS_IMETHOD GetAlt(nsAString & aAlt) = 0;
  NS_SCRIPTABLE NS_IMETHOD SetAlt(const nsAString & aAlt) = 0;

  /* attribute DOMString src; */
  NS_SCRIPTABLE NS_IMETHOD GetSrc(nsAString & aSrc) = 0;
  NS_SCRIPTABLE NS_IMETHOD SetSrc(const nsAString & aSrc) = 0;

  /* attribute DOMString useMap; */
  NS_SCRIPTABLE NS_IMETHOD GetUseMap(nsAString & aUseMap) = 0;
  NS_SCRIPTABLE NS_IMETHOD SetUseMap(const nsAString & aUseMap) = 0;

  /* attribute boolean isMap; */
  NS_SCRIPTABLE NS_IMETHOD GetIsMap(PRBool *aIsMap) = 0;
  NS_SCRIPTABLE NS_IMETHOD SetIsMap(PRBool aIsMap) = 0;

  /* attribute long width; */
  NS_SCRIPTABLE NS_IMETHOD GetWidth(PRInt32 *aWidth) = 0;
  NS_SCRIPTABLE NS_IMETHOD SetWidth(PRInt32 aWidth) = 0;

  /* attribute long height; */
  NS_SCRIPTABLE NS_IMETHOD GetHeight(PRInt32 *aHeight) = 0;
  NS_SCRIPTABLE NS_IMETHOD SetHeight(PRInt32 aHeight) = 0;

  /* readonly attribute long naturalWidth; */
  NS_SCRIPTABLE NS_IMETHOD GetNaturalWidth(PRInt32 *aNaturalWidth) = 0;

  /* readonly attribute long naturalHeight; */
  NS_SCRIPTABLE NS_IMETHOD GetNaturalHeight(PRInt32 *aNaturalHeight) = 0;

  /* readonly attribute boolean complete; */
  NS_SCRIPTABLE NS_IMETHOD GetComplete(PRBool *aComplete) = 0;

  /* attribute DOMString name; */
  NS_SCRIPTABLE NS_IMETHOD GetName(nsAString & aName) = 0;
  NS_SCRIPTABLE NS_IMETHOD SetName(const nsAString & aName) = 0;

  /* attribute DOMString align; */
  NS_SCRIPTABLE NS_IMETHOD GetAlign(nsAString & aAlign) = 0;
  NS_SCRIPTABLE NS_IMETHOD SetAlign(const nsAString & aAlign) = 0;

  /* attribute DOMString border; */
  NS_SCRIPTABLE NS_IMETHOD GetBorder(nsAString & aBorder) = 0;
  NS_SCRIPTABLE NS_IMETHOD SetBorder(const nsAString & aBorder) = 0;

  /* attribute long hspace; */
  NS_SCRIPTABLE NS_IMETHOD GetHspace(PRInt32 *aHspace) = 0;
  NS_SCRIPTABLE NS_IMETHOD SetHspace(PRInt32 aHspace) = 0;

  /* attribute DOMString longDesc; */
  NS_SCRIPTABLE NS_IMETHOD GetLongDesc(nsAString & aLongDesc) = 0;
  NS_SCRIPTABLE NS_IMETHOD SetLongDesc(const nsAString & aLongDesc) = 0;

  /* attribute long vspace; */
  NS_SCRIPTABLE NS_IMETHOD GetVspace(PRInt32 *aVspace) = 0;
  NS_SCRIPTABLE NS_IMETHOD SetVspace(PRInt32 aVspace) = 0;

  /* attribute DOMString lowsrc; */
  NS_SCRIPTABLE NS_IMETHOD GetLowsrc(nsAString & aLowsrc) = 0;
  NS_SCRIPTABLE NS_IMETHOD SetLowsrc(const nsAString & aLowsrc) = 0;

  /* readonly attribute long x; */
  NS_SCRIPTABLE NS_IMETHOD GetX(PRInt32 *aX) = 0;

  /* readonly attribute long y; */
  NS_SCRIPTABLE NS_IMETHOD GetY(PRInt32 *aY) = 0;

};

  NS_DEFINE_STATIC_IID_ACCESSOR(nsIDOMHTMLImageElement, NS_IDOMHTMLIMAGEELEMENT_IID)

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_NSIDOMHTMLIMAGEELEMENT \
  NS_SCRIPTABLE NS_IMETHOD GetAlt(nsAString & aAlt); \
  NS_SCRIPTABLE NS_IMETHOD SetAlt(const nsAString & aAlt); \
  NS_SCRIPTABLE NS_IMETHOD GetSrc(nsAString & aSrc); \
  NS_SCRIPTABLE NS_IMETHOD SetSrc(const nsAString & aSrc); \
  NS_SCRIPTABLE NS_IMETHOD GetUseMap(nsAString & aUseMap); \
  NS_SCRIPTABLE NS_IMETHOD SetUseMap(const nsAString & aUseMap); \
  NS_SCRIPTABLE NS_IMETHOD GetIsMap(PRBool *aIsMap); \
  NS_SCRIPTABLE NS_IMETHOD SetIsMap(PRBool aIsMap); \
  NS_SCRIPTABLE NS_IMETHOD GetWidth(PRInt32 *aWidth); \
  NS_SCRIPTABLE NS_IMETHOD SetWidth(PRInt32 aWidth); \
  NS_SCRIPTABLE NS_IMETHOD GetHeight(PRInt32 *aHeight); \
  NS_SCRIPTABLE NS_IMETHOD SetHeight(PRInt32 aHeight); \
  NS_SCRIPTABLE NS_IMETHOD GetNaturalWidth(PRInt32 *aNaturalWidth); \
  NS_SCRIPTABLE NS_IMETHOD GetNaturalHeight(PRInt32 *aNaturalHeight); \
  NS_SCRIPTABLE NS_IMETHOD GetComplete(PRBool *aComplete); \
  NS_SCRIPTABLE NS_IMETHOD GetName(nsAString & aName); \
  NS_SCRIPTABLE NS_IMETHOD SetName(const nsAString & aName); \
  NS_SCRIPTABLE NS_IMETHOD GetAlign(nsAString & aAlign); \
  NS_SCRIPTABLE NS_IMETHOD SetAlign(const nsAString & aAlign); \
  NS_SCRIPTABLE NS_IMETHOD GetBorder(nsAString & aBorder); \
  NS_SCRIPTABLE NS_IMETHOD SetBorder(const nsAString & aBorder); \
  NS_SCRIPTABLE NS_IMETHOD GetHspace(PRInt32 *aHspace); \
  NS_SCRIPTABLE NS_IMETHOD SetHspace(PRInt32 aHspace); \
  NS_SCRIPTABLE NS_IMETHOD GetLongDesc(nsAString & aLongDesc); \
  NS_SCRIPTABLE NS_IMETHOD SetLongDesc(const nsAString & aLongDesc); \
  NS_SCRIPTABLE NS_IMETHOD GetVspace(PRInt32 *aVspace); \
  NS_SCRIPTABLE NS_IMETHOD SetVspace(PRInt32 aVspace); \
  NS_SCRIPTABLE NS_IMETHOD GetLowsrc(nsAString & aLowsrc); \
  NS_SCRIPTABLE NS_IMETHOD SetLowsrc(const nsAString & aLowsrc); \
  NS_SCRIPTABLE NS_IMETHOD GetX(PRInt32 *aX); \
  NS_SCRIPTABLE NS_IMETHOD GetY(PRInt32 *aY); 

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_NSIDOMHTMLIMAGEELEMENT(_to) \
  NS_SCRIPTABLE NS_IMETHOD GetAlt(nsAString & aAlt) { return _to GetAlt(aAlt); } \
  NS_SCRIPTABLE NS_IMETHOD SetAlt(const nsAString & aAlt) { return _to SetAlt(aAlt); } \
  NS_SCRIPTABLE NS_IMETHOD GetSrc(nsAString & aSrc) { return _to GetSrc(aSrc); } \
  NS_SCRIPTABLE NS_IMETHOD SetSrc(const nsAString & aSrc) { return _to SetSrc(aSrc); } \
  NS_SCRIPTABLE NS_IMETHOD GetUseMap(nsAString & aUseMap) { return _to GetUseMap(aUseMap); } \
  NS_SCRIPTABLE NS_IMETHOD SetUseMap(const nsAString & aUseMap) { return _to SetUseMap(aUseMap); } \
  NS_SCRIPTABLE NS_IMETHOD GetIsMap(PRBool *aIsMap) { return _to GetIsMap(aIsMap); } \
  NS_SCRIPTABLE NS_IMETHOD SetIsMap(PRBool aIsMap) { return _to SetIsMap(aIsMap); } \
  NS_SCRIPTABLE NS_IMETHOD GetWidth(PRInt32 *aWidth) { return _to GetWidth(aWidth); } \
  NS_SCRIPTABLE NS_IMETHOD SetWidth(PRInt32 aWidth) { return _to SetWidth(aWidth); } \
  NS_SCRIPTABLE NS_IMETHOD GetHeight(PRInt32 *aHeight) { return _to GetHeight(aHeight); } \
  NS_SCRIPTABLE NS_IMETHOD SetHeight(PRInt32 aHeight) { return _to SetHeight(aHeight); } \
  NS_SCRIPTABLE NS_IMETHOD GetNaturalWidth(PRInt32 *aNaturalWidth) { return _to GetNaturalWidth(aNaturalWidth); } \
  NS_SCRIPTABLE NS_IMETHOD GetNaturalHeight(PRInt32 *aNaturalHeight) { return _to GetNaturalHeight(aNaturalHeight); } \
  NS_SCRIPTABLE NS_IMETHOD GetComplete(PRBool *aComplete) { return _to GetComplete(aComplete); } \
  NS_SCRIPTABLE NS_IMETHOD GetName(nsAString & aName) { return _to GetName(aName); } \
  NS_SCRIPTABLE NS_IMETHOD SetName(const nsAString & aName) { return _to SetName(aName); } \
  NS_SCRIPTABLE NS_IMETHOD GetAlign(nsAString & aAlign) { return _to GetAlign(aAlign); } \
  NS_SCRIPTABLE NS_IMETHOD SetAlign(const nsAString & aAlign) { return _to SetAlign(aAlign); } \
  NS_SCRIPTABLE NS_IMETHOD GetBorder(nsAString & aBorder) { return _to GetBorder(aBorder); } \
  NS_SCRIPTABLE NS_IMETHOD SetBorder(const nsAString & aBorder) { return _to SetBorder(aBorder); } \
  NS_SCRIPTABLE NS_IMETHOD GetHspace(PRInt32 *aHspace) { return _to GetHspace(aHspace); } \
  NS_SCRIPTABLE NS_IMETHOD SetHspace(PRInt32 aHspace) { return _to SetHspace(aHspace); } \
  NS_SCRIPTABLE NS_IMETHOD GetLongDesc(nsAString & aLongDesc) { return _to GetLongDesc(aLongDesc); } \
  NS_SCRIPTABLE NS_IMETHOD SetLongDesc(const nsAString & aLongDesc) { return _to SetLongDesc(aLongDesc); } \
  NS_SCRIPTABLE NS_IMETHOD GetVspace(PRInt32 *aVspace) { return _to GetVspace(aVspace); } \
  NS_SCRIPTABLE NS_IMETHOD SetVspace(PRInt32 aVspace) { return _to SetVspace(aVspace); } \
  NS_SCRIPTABLE NS_IMETHOD GetLowsrc(nsAString & aLowsrc) { return _to GetLowsrc(aLowsrc); } \
  NS_SCRIPTABLE NS_IMETHOD SetLowsrc(const nsAString & aLowsrc) { return _to SetLowsrc(aLowsrc); } \
  NS_SCRIPTABLE NS_IMETHOD GetX(PRInt32 *aX) { return _to GetX(aX); } \
  NS_SCRIPTABLE NS_IMETHOD GetY(PRInt32 *aY) { return _to GetY(aY); } 

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_NSIDOMHTMLIMAGEELEMENT(_to) \
  NS_SCRIPTABLE NS_IMETHOD GetAlt(nsAString & aAlt) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetAlt(aAlt); } \
  NS_SCRIPTABLE NS_IMETHOD SetAlt(const nsAString & aAlt) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetAlt(aAlt); } \
  NS_SCRIPTABLE NS_IMETHOD GetSrc(nsAString & aSrc) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetSrc(aSrc); } \
  NS_SCRIPTABLE NS_IMETHOD SetSrc(const nsAString & aSrc) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetSrc(aSrc); } \
  NS_SCRIPTABLE NS_IMETHOD GetUseMap(nsAString & aUseMap) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetUseMap(aUseMap); } \
  NS_SCRIPTABLE NS_IMETHOD SetUseMap(const nsAString & aUseMap) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetUseMap(aUseMap); } \
  NS_SCRIPTABLE NS_IMETHOD GetIsMap(PRBool *aIsMap) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetIsMap(aIsMap); } \
  NS_SCRIPTABLE NS_IMETHOD SetIsMap(PRBool aIsMap) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetIsMap(aIsMap); } \
  NS_SCRIPTABLE NS_IMETHOD GetWidth(PRInt32 *aWidth) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetWidth(aWidth); } \
  NS_SCRIPTABLE NS_IMETHOD SetWidth(PRInt32 aWidth) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetWidth(aWidth); } \
  NS_SCRIPTABLE NS_IMETHOD GetHeight(PRInt32 *aHeight) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetHeight(aHeight); } \
  NS_SCRIPTABLE NS_IMETHOD SetHeight(PRInt32 aHeight) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetHeight(aHeight); } \
  NS_SCRIPTABLE NS_IMETHOD GetNaturalWidth(PRInt32 *aNaturalWidth) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetNaturalWidth(aNaturalWidth); } \
  NS_SCRIPTABLE NS_IMETHOD GetNaturalHeight(PRInt32 *aNaturalHeight) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetNaturalHeight(aNaturalHeight); } \
  NS_SCRIPTABLE NS_IMETHOD GetComplete(PRBool *aComplete) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetComplete(aComplete); } \
  NS_SCRIPTABLE NS_IMETHOD GetName(nsAString & aName) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetName(aName); } \
  NS_SCRIPTABLE NS_IMETHOD SetName(const nsAString & aName) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetName(aName); } \
  NS_SCRIPTABLE NS_IMETHOD GetAlign(nsAString & aAlign) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetAlign(aAlign); } \
  NS_SCRIPTABLE NS_IMETHOD SetAlign(const nsAString & aAlign) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetAlign(aAlign); } \
  NS_SCRIPTABLE NS_IMETHOD GetBorder(nsAString & aBorder) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetBorder(aBorder); } \
  NS_SCRIPTABLE NS_IMETHOD SetBorder(const nsAString & aBorder) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetBorder(aBorder); } \
  NS_SCRIPTABLE NS_IMETHOD GetHspace(PRInt32 *aHspace) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetHspace(aHspace); } \
  NS_SCRIPTABLE NS_IMETHOD SetHspace(PRInt32 aHspace) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetHspace(aHspace); } \
  NS_SCRIPTABLE NS_IMETHOD GetLongDesc(nsAString & aLongDesc) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetLongDesc(aLongDesc); } \
  NS_SCRIPTABLE NS_IMETHOD SetLongDesc(const nsAString & aLongDesc) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetLongDesc(aLongDesc); } \
  NS_SCRIPTABLE NS_IMETHOD GetVspace(PRInt32 *aVspace) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetVspace(aVspace); } \
  NS_SCRIPTABLE NS_IMETHOD SetVspace(PRInt32 aVspace) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetVspace(aVspace); } \
  NS_SCRIPTABLE NS_IMETHOD GetLowsrc(nsAString & aLowsrc) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetLowsrc(aLowsrc); } \
  NS_SCRIPTABLE NS_IMETHOD SetLowsrc(const nsAString & aLowsrc) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetLowsrc(aLowsrc); } \
  NS_SCRIPTABLE NS_IMETHOD GetX(PRInt32 *aX) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetX(aX); } \
  NS_SCRIPTABLE NS_IMETHOD GetY(PRInt32 *aY) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetY(aY); } 

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class nsDOMHTMLImageElement : public nsIDOMHTMLImageElement
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMHTMLIMAGEELEMENT

  nsDOMHTMLImageElement();

private:
  ~nsDOMHTMLImageElement();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(nsDOMHTMLImageElement, nsIDOMHTMLImageElement)

nsDOMHTMLImageElement::nsDOMHTMLImageElement()
{
  /* member initializers and constructor code */
}

nsDOMHTMLImageElement::~nsDOMHTMLImageElement()
{
  /* destructor code */
}

/* attribute DOMString alt; */
NS_IMETHODIMP nsDOMHTMLImageElement::GetAlt(nsAString & aAlt)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsDOMHTMLImageElement::SetAlt(const nsAString & aAlt)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute DOMString src; */
NS_IMETHODIMP nsDOMHTMLImageElement::GetSrc(nsAString & aSrc)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsDOMHTMLImageElement::SetSrc(const nsAString & aSrc)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute DOMString useMap; */
NS_IMETHODIMP nsDOMHTMLImageElement::GetUseMap(nsAString & aUseMap)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsDOMHTMLImageElement::SetUseMap(const nsAString & aUseMap)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute boolean isMap; */
NS_IMETHODIMP nsDOMHTMLImageElement::GetIsMap(PRBool *aIsMap)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsDOMHTMLImageElement::SetIsMap(PRBool aIsMap)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute long width; */
NS_IMETHODIMP nsDOMHTMLImageElement::GetWidth(PRInt32 *aWidth)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsDOMHTMLImageElement::SetWidth(PRInt32 aWidth)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute long height; */
NS_IMETHODIMP nsDOMHTMLImageElement::GetHeight(PRInt32 *aHeight)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsDOMHTMLImageElement::SetHeight(PRInt32 aHeight)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute long naturalWidth; */
NS_IMETHODIMP nsDOMHTMLImageElement::GetNaturalWidth(PRInt32 *aNaturalWidth)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute long naturalHeight; */
NS_IMETHODIMP nsDOMHTMLImageElement::GetNaturalHeight(PRInt32 *aNaturalHeight)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute boolean complete; */
NS_IMETHODIMP nsDOMHTMLImageElement::GetComplete(PRBool *aComplete)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute DOMString name; */
NS_IMETHODIMP nsDOMHTMLImageElement::GetName(nsAString & aName)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsDOMHTMLImageElement::SetName(const nsAString & aName)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute DOMString align; */
NS_IMETHODIMP nsDOMHTMLImageElement::GetAlign(nsAString & aAlign)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsDOMHTMLImageElement::SetAlign(const nsAString & aAlign)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute DOMString border; */
NS_IMETHODIMP nsDOMHTMLImageElement::GetBorder(nsAString & aBorder)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsDOMHTMLImageElement::SetBorder(const nsAString & aBorder)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute long hspace; */
NS_IMETHODIMP nsDOMHTMLImageElement::GetHspace(PRInt32 *aHspace)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsDOMHTMLImageElement::SetHspace(PRInt32 aHspace)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute DOMString longDesc; */
NS_IMETHODIMP nsDOMHTMLImageElement::GetLongDesc(nsAString & aLongDesc)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsDOMHTMLImageElement::SetLongDesc(const nsAString & aLongDesc)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute long vspace; */
NS_IMETHODIMP nsDOMHTMLImageElement::GetVspace(PRInt32 *aVspace)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsDOMHTMLImageElement::SetVspace(PRInt32 aVspace)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute DOMString lowsrc; */
NS_IMETHODIMP nsDOMHTMLImageElement::GetLowsrc(nsAString & aLowsrc)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsDOMHTMLImageElement::SetLowsrc(const nsAString & aLowsrc)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute long x; */
NS_IMETHODIMP nsDOMHTMLImageElement::GetX(PRInt32 *aX)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute long y; */
NS_IMETHODIMP nsDOMHTMLImageElement::GetY(PRInt32 *aY)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif


#endif /* __gen_nsIDOMHTMLImageElement_h__ */
