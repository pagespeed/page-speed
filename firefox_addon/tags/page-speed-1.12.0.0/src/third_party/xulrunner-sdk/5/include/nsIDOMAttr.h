/*
 * DO NOT EDIT.  THIS FILE IS GENERATED FROM /builds/slave/aurora-lnx64-xr/build/dom/interfaces/core/nsIDOMAttr.idl
 */

#ifndef __gen_nsIDOMAttr_h__
#define __gen_nsIDOMAttr_h__


#ifndef __gen_nsIDOMNode_h__
#include "nsIDOMNode.h"
#endif

/* For IDL files that don't want to include root IDL files. */
#ifndef NS_NO_VTABLE
#define NS_NO_VTABLE
#endif

/* starting interface:    nsIDOMAttr */
#define NS_IDOMATTR_IID_STR "669a0f55-1e5d-4471-8de9-a6c6774354dd"

#define NS_IDOMATTR_IID \
  {0x669a0f55, 0x1e5d, 0x4471, \
    { 0x8d, 0xe9, 0xa6, 0xc6, 0x77, 0x43, 0x54, 0xdd }}

class NS_NO_VTABLE NS_SCRIPTABLE nsIDOMAttr : public nsIDOMNode {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDOMATTR_IID)

  /**
 * The nsIDOMAttr interface represents an attribute in an "Element" object. 
 * Typically the allowable values for the attribute are defined in a document 
 * type definition.
 * 
 * For more information on this interface please see 
 * http://www.w3.org/TR/DOM-Level-2-Core/
 */
  /* readonly attribute DOMString name; */
  NS_SCRIPTABLE NS_IMETHOD GetName(nsAString & aName) = 0;

  /* readonly attribute boolean specified; */
  NS_SCRIPTABLE NS_IMETHOD GetSpecified(PRBool *aSpecified) = 0;

  /* attribute DOMString value; */
  NS_SCRIPTABLE NS_IMETHOD GetValue(nsAString & aValue) = 0;
  NS_SCRIPTABLE NS_IMETHOD SetValue(const nsAString & aValue) = 0;

  /* readonly attribute nsIDOMElement ownerElement; */
  NS_SCRIPTABLE NS_IMETHOD GetOwnerElement(nsIDOMElement **aOwnerElement) = 0;

};

  NS_DEFINE_STATIC_IID_ACCESSOR(nsIDOMAttr, NS_IDOMATTR_IID)

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_NSIDOMATTR \
  NS_SCRIPTABLE NS_IMETHOD GetName(nsAString & aName); \
  NS_SCRIPTABLE NS_IMETHOD GetSpecified(PRBool *aSpecified); \
  NS_SCRIPTABLE NS_IMETHOD GetValue(nsAString & aValue); \
  NS_SCRIPTABLE NS_IMETHOD SetValue(const nsAString & aValue); \
  NS_SCRIPTABLE NS_IMETHOD GetOwnerElement(nsIDOMElement **aOwnerElement); 

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_NSIDOMATTR(_to) \
  NS_SCRIPTABLE NS_IMETHOD GetName(nsAString & aName) { return _to GetName(aName); } \
  NS_SCRIPTABLE NS_IMETHOD GetSpecified(PRBool *aSpecified) { return _to GetSpecified(aSpecified); } \
  NS_SCRIPTABLE NS_IMETHOD GetValue(nsAString & aValue) { return _to GetValue(aValue); } \
  NS_SCRIPTABLE NS_IMETHOD SetValue(const nsAString & aValue) { return _to SetValue(aValue); } \
  NS_SCRIPTABLE NS_IMETHOD GetOwnerElement(nsIDOMElement **aOwnerElement) { return _to GetOwnerElement(aOwnerElement); } 

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_NSIDOMATTR(_to) \
  NS_SCRIPTABLE NS_IMETHOD GetName(nsAString & aName) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetName(aName); } \
  NS_SCRIPTABLE NS_IMETHOD GetSpecified(PRBool *aSpecified) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetSpecified(aSpecified); } \
  NS_SCRIPTABLE NS_IMETHOD GetValue(nsAString & aValue) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetValue(aValue); } \
  NS_SCRIPTABLE NS_IMETHOD SetValue(const nsAString & aValue) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetValue(aValue); } \
  NS_SCRIPTABLE NS_IMETHOD GetOwnerElement(nsIDOMElement **aOwnerElement) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetOwnerElement(aOwnerElement); } 

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class nsDOMAttr : public nsIDOMAttr
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMATTR

  nsDOMAttr();

private:
  ~nsDOMAttr();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(nsDOMAttr, nsIDOMAttr)

nsDOMAttr::nsDOMAttr()
{
  /* member initializers and constructor code */
}

nsDOMAttr::~nsDOMAttr()
{
  /* destructor code */
}

/* readonly attribute DOMString name; */
NS_IMETHODIMP nsDOMAttr::GetName(nsAString & aName)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute boolean specified; */
NS_IMETHODIMP nsDOMAttr::GetSpecified(PRBool *aSpecified)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute DOMString value; */
NS_IMETHODIMP nsDOMAttr::GetValue(nsAString & aValue)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsDOMAttr::SetValue(const nsAString & aValue)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute nsIDOMElement ownerElement; */
NS_IMETHODIMP nsDOMAttr::GetOwnerElement(nsIDOMElement **aOwnerElement)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif


#endif /* __gen_nsIDOMAttr_h__ */
