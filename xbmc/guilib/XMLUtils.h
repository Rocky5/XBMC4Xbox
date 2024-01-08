#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "utils/StdString.h"
#include "../xbmc/xbox/PlatformDefs.h"

// forward
class TiXmlDocument;
class TiXmlNode;

class XMLUtils
{
public:
  static bool HasUTF8Declaration(const CStdString &strXML);

  static bool GetHex(const TiXmlNode* pRootNode, const char* strTag, uint32_t& dwHexValue);
  static bool GetUInt(const TiXmlNode* pRootNode, const char* strTag, uint32_t& dwUIntValue);
  static bool GetLong(const TiXmlNode* pRootNode, const char* strTag, long& lLongValue);
  static bool GetFloat(const TiXmlNode* pRootNode, const char* strTag, float& value);
  static bool GetDouble(const TiXmlNode* pRootNode, const char* strTag, double &value);
  static bool GetInt(const TiXmlNode* pRootNode, const char* strTag, int& iIntValue);
  static bool GetBoolean(const TiXmlNode* pRootNode, const char* strTag, bool& bBoolValue);
  static bool GetString(const TiXmlNode* pRootNode, const char* strTag, CStdString& strStringValue);
  static bool GetAdditiveString(const TiXmlNode* pRootNode, const char* strTag, const CStdString& strSeparator, CStdString& strStringValue);
  static bool GetEncoding(const TiXmlDocument* pDoc, CStdString& strEncoding);
  static bool GetPath(const TiXmlNode* pRootNode, const char* strTag, CStdString& strStringValue);
  static bool GetFloat(const TiXmlNode* pRootNode, const char* strTag, float& value, const float min, const float max);
  static bool GetInt(const TiXmlNode* pRootNode, const char* strTag, int& iIntValue, const int min, const int max);

  static void SetString(TiXmlNode* pRootNode, const char *strTag, const CStdString& strValue);
  static void SetAdditiveString(TiXmlNode* pRootNode, const char *strTag, const CStdString& strSeparator, const CStdString& strValue);
  static void SetInt(TiXmlNode* pRootNode, const char *strTag, int value);
  static void SetFloat(TiXmlNode* pRootNode, const char *strTag, float value);
  static void SetBoolean(TiXmlNode* pRootNode, const char *strTag, bool value);
  static void SetHex(TiXmlNode* pRootNode, const char *strTag, uint32_t value);
  static void SetPath(TiXmlNode* pRootNode, const char *strTag, const CStdString& strValue);
};

