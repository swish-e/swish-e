# This file was created automatically by SWIG.
# Don't modify this file, modify the SWIG interface instead.
# This file is compatible with both classic and new-style classes.

import _swish

def _swig_setattr(self,class_type,name,value):
    if (name == "this"):
        if isinstance(value, class_type):
            self.__dict__[name] = value.this
            if hasattr(value,"thisown"): self.__dict__["thisown"] = value.thisown
            del value.thisown
            return
    method = class_type.__swig_setmethods__.get(name,None)
    if method: return method(self,value)
    self.__dict__[name] = value

def _swig_getattr(self,class_type,name):
    method = class_type.__swig_getmethods__.get(name,None)
    if method: return method(self)
    raise AttributeError,name

import types
try:
    _object = types.ObjectType
    _newclass = 1
except AttributeError:
    class _object : pass
    _newclass = 0
del types


SEARCHSWISH_H = _swish.SEARCHSWISH_H
SWISH_NUMBER = _swish.SWISH_NUMBER
SWISH_STRING = _swish.SWISH_STRING
SWISH_LIST = _swish.SWISH_LIST
SWISH_BOOL = _swish.SWISH_BOOL
SWISH_WORD_HASH = _swish.SWISH_WORD_HASH
SWISH_OTHER_DATA = _swish.SWISH_OTHER_DATA
SWISH_HEADER_ERROR = _swish.SWISH_HEADER_ERROR
class SWISH_HEADER_VALUE(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, SWISH_HEADER_VALUE, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, SWISH_HEADER_VALUE, name)
    def __repr__(self):
        return "<C SWISH_HEADER_VALUE instance at %s>" % (self.this,)
    __swig_setmethods__["string"] = _swish.SWISH_HEADER_VALUE_string_set
    __swig_getmethods__["string"] = _swish.SWISH_HEADER_VALUE_string_get
    if _newclass:string = property(_swish.SWISH_HEADER_VALUE_string_get, _swish.SWISH_HEADER_VALUE_string_set)
    __swig_setmethods__["string_list"] = _swish.SWISH_HEADER_VALUE_string_list_set
    __swig_getmethods__["string_list"] = _swish.SWISH_HEADER_VALUE_string_list_get
    if _newclass:string_list = property(_swish.SWISH_HEADER_VALUE_string_list_get, _swish.SWISH_HEADER_VALUE_string_list_set)
    __swig_setmethods__["number"] = _swish.SWISH_HEADER_VALUE_number_set
    __swig_getmethods__["number"] = _swish.SWISH_HEADER_VALUE_number_get
    if _newclass:number = property(_swish.SWISH_HEADER_VALUE_number_get, _swish.SWISH_HEADER_VALUE_number_set)
    __swig_setmethods__["boolean"] = _swish.SWISH_HEADER_VALUE_boolean_set
    __swig_getmethods__["boolean"] = _swish.SWISH_HEADER_VALUE_boolean_get
    if _newclass:boolean = property(_swish.SWISH_HEADER_VALUE_boolean_get, _swish.SWISH_HEADER_VALUE_boolean_set)
    def __init__(self, *args):
        _swig_setattr(self, SWISH_HEADER_VALUE, 'this', _swish.new_SWISH_HEADER_VALUE(*args))
        _swig_setattr(self, SWISH_HEADER_VALUE, 'thisown', 1)
    def __del__(self, destroy=_swish.delete_SWISH_HEADER_VALUE):
        try:
            if self.thisown: destroy(self)
        except: pass

class SWISH_HEADER_VALUEPtr(SWISH_HEADER_VALUE):
    def __init__(self, this):
        _swig_setattr(self, SWISH_HEADER_VALUE, 'this', this)
        if not hasattr(self,"thisown"): _swig_setattr(self, SWISH_HEADER_VALUE, 'thisown', 0)
        _swig_setattr(self, SWISH_HEADER_VALUE,self.__class__,SWISH_HEADER_VALUE)
_swish.SWISH_HEADER_VALUE_swigregister(SWISH_HEADER_VALUEPtr)


SwishHeaderNames = _swish.SwishHeaderNames

SwishIndexNames = _swish.SwishIndexNames

SwishHeaderValue = _swish.SwishHeaderValue

SwishResultIndexValue = _swish.SwishResultIndexValue
IN_FILE_BIT = _swish.IN_FILE_BIT
IN_TITLE_BIT = _swish.IN_TITLE_BIT
IN_HEAD_BIT = _swish.IN_HEAD_BIT
IN_BODY_BIT = _swish.IN_BODY_BIT
IN_COMMENTS_BIT = _swish.IN_COMMENTS_BIT
IN_HEADER_BIT = _swish.IN_HEADER_BIT
IN_EMPHASIZED_BIT = _swish.IN_EMPHASIZED_BIT
IN_META_BIT = _swish.IN_META_BIT
IN_FILE = _swish.IN_FILE
IN_TITLE = _swish.IN_TITLE
IN_HEAD = _swish.IN_HEAD
IN_BODY = _swish.IN_BODY
IN_COMMENTS = _swish.IN_COMMENTS
IN_HEADER = _swish.IN_HEADER
IN_EMPHASIZED = _swish.IN_EMPHASIZED
IN_META = _swish.IN_META
IN_ALL = _swish.IN_ALL

SwishInit = _swish.SwishInit

SwishQuery = _swish.SwishQuery

New_Search_Object = _swish.New_Search_Object

SwishSetRefPtr = _swish.SwishSetRefPtr

SwishSearch_parent = _swish.SwishSearch_parent

SwishResults_parent = _swish.SwishResults_parent

SwishResult_parent = _swish.SwishResult_parent

ResultsSetRefPtr = _swish.ResultsSetRefPtr

SwishSetStructure = _swish.SwishSetStructure

SwishPhraseDelimiter = _swish.SwishPhraseDelimiter

SwishSetSort = _swish.SwishSetSort

SwishSetQuery = _swish.SwishSetQuery

SwishSetSearchLimit = _swish.SwishSetSearchLimit

SwishResetSearchLimit = _swish.SwishResetSearchLimit

SwishExecute = _swish.SwishExecute

SwishHits = _swish.SwishHits

SwishParsedWords = _swish.SwishParsedWords

SwishRemovedStopwords = _swish.SwishRemovedStopwords

SwishSeekResult = _swish.SwishSeekResult

SwishNextResult = _swish.SwishNextResult

SwishResultPropertyStr = _swish.SwishResultPropertyStr

SwishResultPropertyULong = _swish.SwishResultPropertyULong

SW_ResultToSW_HANDLE = _swish.SW_ResultToSW_HANDLE

SW_ResultsToSW_HANDLE = _swish.SW_ResultsToSW_HANDLE

Free_Search_Object = _swish.Free_Search_Object

Free_Results_Object = _swish.Free_Results_Object

SwishClose = _swish.SwishClose

SwishError = _swish.SwishError

SwishCriticalError = _swish.SwishCriticalError

SwishAbortLastError = _swish.SwishAbortLastError

SwishErrorString = _swish.SwishErrorString

SwishLastErrorMsg = _swish.SwishLastErrorMsg

set_error_handle = _swish.set_error_handle

SwishErrorsToStderr = _swish.SwishErrorsToStderr

SwishWordsByLetter = _swish.SwishWordsByLetter

SwishStemWord = _swish.SwishStemWord

SwishFuzzyWord = _swish.SwishFuzzyWord

SwishFuzzyWordList = _swish.SwishFuzzyWordList

SwishFuzzyWordCount = _swish.SwishFuzzyWordCount

SwishFuzzyWordError = _swish.SwishFuzzyWordError

SwishFuzzyWordFree = _swish.SwishFuzzyWordFree

SwishFuzzyMode = _swish.SwishFuzzyMode
PROP_UNDEFINED = _swish.PROP_UNDEFINED
PROP_UNKNOWN = _swish.PROP_UNKNOWN
PROP_STRING = _swish.PROP_STRING
PROP_INTEGER = _swish.PROP_INTEGER
PROP_FLOAT = _swish.PROP_FLOAT
PROP_DATE = _swish.PROP_DATE
PROP_ULONG = _swish.PROP_ULONG
class u_PropValue1(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, u_PropValue1, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, u_PropValue1, name)
    def __repr__(self):
        return "<C u_PropValue1 instance at %s>" % (self.this,)
    __swig_setmethods__["v_str"] = _swish.u_PropValue1_v_str_set
    __swig_getmethods__["v_str"] = _swish.u_PropValue1_v_str_get
    if _newclass:v_str = property(_swish.u_PropValue1_v_str_get, _swish.u_PropValue1_v_str_set)
    __swig_setmethods__["v_int"] = _swish.u_PropValue1_v_int_set
    __swig_getmethods__["v_int"] = _swish.u_PropValue1_v_int_get
    if _newclass:v_int = property(_swish.u_PropValue1_v_int_get, _swish.u_PropValue1_v_int_set)
    __swig_setmethods__["v_date"] = _swish.u_PropValue1_v_date_set
    __swig_getmethods__["v_date"] = _swish.u_PropValue1_v_date_get
    if _newclass:v_date = property(_swish.u_PropValue1_v_date_get, _swish.u_PropValue1_v_date_set)
    __swig_setmethods__["v_float"] = _swish.u_PropValue1_v_float_set
    __swig_getmethods__["v_float"] = _swish.u_PropValue1_v_float_get
    if _newclass:v_float = property(_swish.u_PropValue1_v_float_get, _swish.u_PropValue1_v_float_set)
    __swig_setmethods__["v_ulong"] = _swish.u_PropValue1_v_ulong_set
    __swig_getmethods__["v_ulong"] = _swish.u_PropValue1_v_ulong_get
    if _newclass:v_ulong = property(_swish.u_PropValue1_v_ulong_get, _swish.u_PropValue1_v_ulong_set)
    def __init__(self, *args):
        _swig_setattr(self, u_PropValue1, 'this', _swish.new_u_PropValue1(*args))
        _swig_setattr(self, u_PropValue1, 'thisown', 1)
    def __del__(self, destroy=_swish.delete_u_PropValue1):
        try:
            if self.thisown: destroy(self)
        except: pass

class u_PropValue1Ptr(u_PropValue1):
    def __init__(self, this):
        _swig_setattr(self, u_PropValue1, 'this', this)
        if not hasattr(self,"thisown"): _swig_setattr(self, u_PropValue1, 'thisown', 0)
        _swig_setattr(self, u_PropValue1,self.__class__,u_PropValue1)
_swish.u_PropValue1_swigregister(u_PropValue1Ptr)

class PropValue(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, PropValue, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, PropValue, name)
    def __repr__(self):
        return "<C PropValue instance at %s>" % (self.this,)
    __swig_setmethods__["datatype"] = _swish.PropValue_datatype_set
    __swig_getmethods__["datatype"] = _swish.PropValue_datatype_get
    if _newclass:datatype = property(_swish.PropValue_datatype_get, _swish.PropValue_datatype_set)
    __swig_setmethods__["value"] = _swish.PropValue_value_set
    __swig_getmethods__["value"] = _swish.PropValue_value_get
    if _newclass:value = property(_swish.PropValue_value_get, _swish.PropValue_value_set)
    __swig_setmethods__["destroy"] = _swish.PropValue_destroy_set
    __swig_getmethods__["destroy"] = _swish.PropValue_destroy_get
    if _newclass:destroy = property(_swish.PropValue_destroy_get, _swish.PropValue_destroy_set)
    def __init__(self, *args):
        _swig_setattr(self, PropValue, 'this', _swish.new_PropValue(*args))
        _swig_setattr(self, PropValue, 'thisown', 1)
    def __del__(self, destroy=_swish.delete_PropValue):
        try:
            if self.thisown: destroy(self)
        except: pass

class PropValuePtr(PropValue):
    def __init__(self, this):
        _swig_setattr(self, PropValue, 'this', this)
        if not hasattr(self,"thisown"): _swig_setattr(self, PropValue, 'thisown', 0)
        _swig_setattr(self, PropValue,self.__class__,PropValue)
_swish.PropValue_swigregister(PropValuePtr)


getResultPropValue = _swish.getResultPropValue

freeResultPropValue = _swish.freeResultPropValue

