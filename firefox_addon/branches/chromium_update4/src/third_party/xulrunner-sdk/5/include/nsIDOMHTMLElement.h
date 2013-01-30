/*
 * DO NOT EDIT.  THIS FILE IS GENERATED FROM /builds/slave/aurora-lnx64-xr/build/dom/interfaces/html/nsIDOMHTMLElement.idl
 */

#ifndef __gen_nsIDOMHTMLElement_h__
#define __gen_nsIDOMHTMLElement_h__


#ifndef __gen_nsIDOMElement_h__
#include "nsIDOMElement.h"
#endif

/* For IDL files that don't want to include root IDL files. */
#ifndef NS_NO_VTABLE
#define NS_NO_VTABLE
#endif

/* starting interface:    nsIDOMHTMLElement */
#define NS_IDOMHTMLELEMENT_IID_STR "164c7ebd-2245-42d2-a96f-2bf2d01c1697"

#define NS_IDOMHTMLELEMENT_IID \
  {0x164c7ebd, 0x2245, 0x42d2, \
    { 0xa9, 0x6f, 0x2b, 0xf2, 0xd0, 0x1c, 0x16, 0x97 }}

class NS_NO_VTABLE NS_SCRIPTABLE nsIDOMHTMLElement : public nsIDOMElement {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDOMHTMLELEMENT_IID)

  /**
 * The nsIDOMHTMLElement interface is the primary [X]HTML element
 * interface. It represents a single [X]HTML element in the document
 * tree.
 *
 * This interface is trying to follow the DOM Level 2 HTML specification:
 * http://www.w3.org/TR/DOM-Level-2-HTML/
 *
 * with changes from the work-in-progress WHATWG HTML specification:
 * http://www.whatwg.org/specs/web-apps/current-work/
 */
  /* attribute DOMString id; */
  NS_SCRIPTABLE NS_IMETHOD GetId(nsAString & aId) = 0;
  NS_SCRIPTABLE NS_IMETHOD SetId(const nsAString & aId) = 0;

  /* attribute DOMString title; */
  NS_SCRIPTABLE NS_IMETHOD GetTitle(nsAString & aTitle) = 0;
  NS_SCRIPTABLE NS_IMETHOD SetTitle(const nsAString & aTitle) = 0;

  /* attribute DOMString lang; */
  NS_SCRIPTABLE NS_IMETHOD GetLang(nsAString & aLang) = 0;
  NS_SCRIPTABLE NS_IMETHOD SetLang(const nsAString & aLang) = 0;

  /* attribute DOMString dir; */
  NS_SCRIPTABLE NS_IMETHOD GetDir(nsAString & aDir) = 0;
  NS_SCRIPTABLE NS_IMETHOD SetDir(const nsAString & aDir) = 0;

  /* attribute DOMString className; */
  NS_SCRIPTABLE NS_IMETHOD GetClassName(nsAString & aClassName) = 0;
  NS_SCRIPTABLE NS_IMETHOD SetClassName(const nsAString & aClassName) = 0;

  /* attribute DOMString accessKey; */
  NS_SCRIPTABLE NS_IMETHOD GetAccessKey(nsAString & aAccessKey) = 0;
  NS_SCRIPTABLE NS_IMETHOD SetAccessKey(const nsAString & aAccessKey) = 0;

  /* void blur (); */
  NS_SCRIPTABLE NS_IMETHOD Blur(void) = 0;

  /* void focus (); */
  NS_SCRIPTABLE NS_IMETHOD Focus(void) = 0;

  /* void click (); */
  NS_SCRIPTABLE NS_IMETHOD Click(void) = 0;

};

  NS_DEFINE_STATIC_IID_ACCESSOR(nsIDOMHTMLElement, NS_IDOMHTMLELEMENT_IID)

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_NSIDOMHTMLELEMENT \
  NS_SCRIPTABLE NS_IMETHOD GetId(nsAString & aId); \
  NS_SCRIPTABLE NS_IMETHOD SetId(const nsAString & aId); \
  NS_SCRIPTABLE NS_IMETHOD GetTitle(nsAString & aTitle); \
  NS_SCRIPTABLE NS_IMETHOD SetTitle(const nsAString & aTitle); \
  NS_SCRIPTABLE NS_IMETHOD GetLang(nsAString & aLang); \
  NS_SCRIPTABLE NS_IMETHOD SetLang(const nsAString & aLang); \
  NS_SCRIPTABLE NS_IMETHOD GetDir(nsAString & aDir); \
  NS_SCRIPTABLE NS_IMETHOD SetDir(const nsAString & aDir); \
  NS_SCRIPTABLE NS_IMETHOD GetClassName(nsAString & aClassName); \
  NS_SCRIPTABLE NS_IMETHOD SetClassName(const nsAString & aClassName); \
  NS_SCRIPTABLE NS_IMETHOD GetAccessKey(nsAString & aAccessKey); \
  NS_SCRIPTABLE NS_IMETHOD SetAccessKey(const nsAString & aAccessKey); \
  NS_SCRIPTABLE NS_IMETHOD Blur(void); \
  NS_SCRIPTABLE NS_IMETHOD Focus(void); \
  NS_SCRIPTABLE NS_IMETHOD Click(void); 

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_NSIDOMHTMLELEMENT(_to) \
  NS_SCRIPTABLE NS_IMETHOD GetId(nsAString & aId) { return _to GetId(aId); } \
  NS_SCRIPTABLE NS_IMETHOD SetId(const nsAString & aId) { return _to SetId(aId); } \
  NS_SCRIPTABLE NS_IMETHOD GetTitle(nsAString & aTitle) { return _to GetTitle(aTitle); } \
  NS_SCRIPTABLE NS_IMETHOD SetTitle(const nsAString & aTitle) { return _to SetTitle(aTitle); } \
  NS_SCRIPTABLE NS_IMETHOD GetLang(nsAString & aLang) { return _to GetLang(aLang); } \
  NS_SCRIPTABLE NS_IMETHOD SetLang(const nsAString & aLang) { return _to SetLang(aLang); } \
  NS_SCRIPTABLE NS_IMETHOD GetDir(nsAString & aDir) { return _to GetDir(aDir); } \
  NS_SCRIPTABLE NS_IMETHOD SetDir(const nsAString & aDir) { return _to SetDir(aDir); } \
  NS_SCRIPTABLE NS_IMETHOD GetClassName(nsAString & aClassName) { return _to GetClassName(aClassName); } \
  NS_SCRIPTABLE NS_IMETHOD SetClassName(const nsAString & aClassName) { return _to SetClassName(aClassName); } \
  NS_SCRIPTABLE NS_IMETHOD GetAccessKey(nsAString & aAccessKey) { return _to GetAccessKey(aAccessKey); } \
  NS_SCRIPTABLE NS_IMETHOD SetAccessKey(const nsAString & aAccessKey) { return _to SetAccessKey(aAccessKey); } \
  NS_SCRIPTABLE NS_IMETHOD Blur(void) { return _to Blur(); } \
  NS_SCRIPTABLE NS_IMETHOD Focus(void) { return _to Focus(); } \
  NS_SCRIPTABLE NS_IMETHOD Click(void) { return _to Click(); } 

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_NSIDOMHTMLELEMENT(_to) \
  NS_SCRIPTABLE NS_IMETHOD GetId(nsAString & aId) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetId(aId); } \
  NS_SCRIPTABLE NS_IMETHOD SetId(const nsAString & aId) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetId(aId); } \
  NS_SCRIPTABLE NS_IMETHOD GetTitle(nsAString & aTitle) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetTitle(aTitle); } \
  NS_SCRIPTABLE NS_IMETHOD SetTitle(const nsAString & aTitle) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetTitle(aTitle); } \
  NS_SCRIPTABLE NS_IMETHOD GetLang(nsAString & aLang) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetLang(aLang); } \
  NS_SCRIPTABLE NS_IMETHOD SetLang(const nsAString & aLang) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetLang(aLang); } \
  NS_SCRIPTABLE NS_IMETHOD GetDir(nsAString & aDir) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetDir(aDir); } \
  NS_SCRIPTABLE NS_IMETHOD SetDir(const nsAString & aDir) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetDir(aDir); } \
  NS_SCRIPTABLE NS_IMETHOD GetClassName(nsAString & aClassName) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetClassName(aClassName); } \
  NS_SCRIPTABLE NS_IMETHOD SetClassName(const nsAString & aClassName) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetClassName(aClassName); } \
  NS_SCRIPTABLE NS_IMETHOD GetAccessKey(nsAString & aAccessKey) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetAccessKey(aAccessKey); } \
  NS_SCRIPTABLE NS_IMETHOD SetAccessKey(const nsAString & aAccessKey) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetAccessKey(aAccessKey); } \
  NS_SCRIPTABLE NS_IMETHOD Blur(void) { return !_to ? NS_ERROR_NULL_POINTER : _to->Blur(); } \
  NS_SCRIPTABLE NS_IMETHOD Focus(void) { return !_to ? NS_ERROR_NULL_POINTER : _to->Focus(); } \
  NS_SCRIPTABLE NS_IMETHOD Click(void) { return !_to ? NS_ERROR_NULL_POINTER : _to->Click(); } 

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class nsDOMHTMLElement : public nsIDOMHTMLElement
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMHTMLELEMENT

  nsDOMHTMLElement();

private:
  ~nsDOMHTMLElement();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(nsDOMHTMLElement, nsIDOMHTMLElement)

nsDOMHTMLElement::nsDOMHTMLElement()
{
  /* member initializers and constructor code */
}

nsDOMHTMLElement::~nsDOMHTMLElement()
{
  /* destructor code */
}

/* attribute DOMString id; */
NS_IMETHODIMP nsDOMHTMLElement::GetId(nsAString & aId)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsDOMHTMLElement::SetId(const nsAString & aId)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute DOMString title; */
NS_IMETHODIMP nsDOMHTMLElement::GetTitle(nsAString & aTitle)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsDOMHTMLElement::SetTitle(const nsAString & aTitle)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute DOMString lang; */
NS_IMETHODIMP nsDOMHTMLElement::GetLang(nsAString & aLang)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsDOMHTMLElement::SetLang(const nsAString & aLang)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute DOMString dir; */
NS_IMETHODIMP nsDOMHTMLElement::GetDir(nsAString & aDir)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsDOMHTMLElement::SetDir(const nsAString & aDir)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute DOMString className; */
NS_IMETHODIMP nsDOMHTMLElement::GetClassName(nsAString & aClassName)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsDOMHTMLElement::SetClassName(const nsAString & aClassName)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute DOMString accessKey; */
NS_IMETHODIMP nsDOMHTMLElement::GetAccessKey(nsAString & aAccessKey)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsDOMHTMLElement::SetAccessKey(const nsAString & aAccessKey)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void blur (); */
NS_IMETHODIMP nsDOMHTMLElement::Blur()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void focus (); */
NS_IMETHODIMP nsDOMHTMLElement::Focus()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void click (); */
NS_IMETHODIMP nsDOMHTMLElement::Click()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif


#endif /* __gen_nsIDOMHTMLElement_h__ */
