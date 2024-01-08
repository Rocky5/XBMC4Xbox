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

#include "RssFeed.h"
#include "settings/Settings.h"
#include "utils/URIUtils.h"
#include "FileSystem/CurlFile.h"
#include "tinyXML/tinyxml.h"
#include "utils/log.h"

using namespace std;

CRssFeed::CRssFeed() 
{
  
}

CRssFeed::~CRssFeed() 
{

}

bool CRssFeed::Init(const CStdString& strURL)
{
  m_strURL = strURL;

  CLog::Log(LOGINFO, "Initializing feed: %s", m_strURL.c_str());
  return true;
}

time_t CRssFeed::ParseDate(const CStdString & strDate)
{
  struct tm pubDate = {0};
  // TODO: Handle time zone
#ifndef _WIN32 // TODO: Implement this on win32 (no strptime)
  strptime(strDate.c_str(), "%a, %d %b %Y %H:%M:%S", &pubDate);
#endif
  // Check the difference between the time of last check and time of the item
  return mktime(&pubDate);
}

bool CRssFeed::ReadFeed()
{
  // Remove all previous items
  EnterCriticalSection(m_ItemVectorLock);
  items.Clear();
  LeaveCriticalSection(m_ItemVectorLock);

  CStdString strXML;
  XFILE::CCurlFile http;
  if (!http.Get(m_strURL, strXML))
    return false;

  TiXmlDocument xmlDoc;
  xmlDoc.Parse(strXML.c_str());
  if (xmlDoc.Error())
    CLog::Log(LOGERROR,"error parsing xml doc from <%s>. error: <%d>", m_strURL.c_str(), xmlDoc.ErrorId());
  
  TiXmlElement* rssXmlNode = xmlDoc.RootElement();
  
  if (!rssXmlNode)
    return false;

  CStdString strMediaThumbnail ;

  TiXmlHandle docHandle( &xmlDoc );
  TiXmlElement* channelXmlNode = docHandle.FirstChild( "rss" ).FirstChild( "channel" ).Element();  
  if (channelXmlNode)
  {
    TiXmlElement* aNode = channelXmlNode->FirstChildElement("title");
    if (aNode && !aNode->NoChildren())
      m_strTitle = aNode->FirstChild()->Value();

    aNode = channelXmlNode->FirstChildElement("itunes:summary");
    if (aNode && !aNode->NoChildren())
      m_strDescription = aNode->FirstChild()->Value();
    
    if (m_strDescription.IsEmpty())
    {
      aNode = channelXmlNode->FirstChildElement("description");
      if (aNode && !aNode->NoChildren())
        m_strDescription = aNode->FirstChild()->Value();
    }
    
    // Get channel thumbnail
    TiXmlHandle chanHandle( channelXmlNode );
    aNode = chanHandle.FirstChild("image").FirstChild("url").Element();
    if (aNode && !aNode->NoChildren())
      strMediaThumbnail = aNode->FirstChild()->Value();
    
    if (strMediaThumbnail.IsEmpty() || !IsPathToThumbnail(strMediaThumbnail))
    {
      aNode = chanHandle.FirstChild("itunes:image").Element();
      if (aNode && !aNode->NoChildren())
        strMediaThumbnail = aNode->FirstChild()->Value();      
    }
  }
  else
    return false;

  TiXmlElement* child = NULL;
  TiXmlElement* item_child = NULL;
  for (child = channelXmlNode->FirstChildElement("item"); child; child = child->NextSiblingElement())
  {
    // Create new item, 
    CFileItemPtr item(new CFileItem());

    for (item_child = child->FirstChildElement(); item_child; item_child = item_child->NextSiblingElement())
    {
      if (strcmp(item_child->Value(), "title") == 0)
	  {
        if (item_child->GetText())
          item->SetLabel(item_child->GetText());
      }
      else if (strcmp(item_child->Value(), "pubDate") == 0)
	  {
        CDateTime pubDate(ParseDate(item_child->GetText()));
        item->m_dateTime = pubDate;
      }
      else if (strcmp(item_child->Value(), "link") == 0)
	  {
        if (item_child->GetText())
		{
          string strLink = item_child->GetText();

          string strPrefix = strLink.substr(0, strLink.find_first_of(":"));
          if (strPrefix == "rss")
		  {
            // If this is an rss item, we treat it as another level in the directory
            item->m_bIsFolder = true;
            item->SetPath(strLink);
          }
          else if (item->GetPath() == "" && IsPathToMedia(strLink))
            item->SetPath(strLink);
        }
      }
      else if(strcmp(item_child->Value(), "enclosure") == 0)
	  {
        const char * url = item_child->Attribute("url");
        if (url && item->GetPath().IsEmpty() && IsPathToMedia(url))
          item->SetPath(url);
        const char * content_type = item_child->Attribute("type");
        if (content_type)
		{
          item->SetMimeType(content_type);
          CStdString strContentType(content_type);
          if (url && item->GetPath().IsEmpty() &&  
            (strContentType.Left(6).Equals("video/") ||
             strContentType.Left(6).Equals("audio/") 
            ))
            item->SetPath(url);
        }
        const char * len = item_child->Attribute("length");
        if (len)  
          item->m_dwSize = _atoi64(len);
      }
      else if(strcmp(item_child->Value(), "media:content") == 0)
	  {
        const char * url = item_child->Attribute("url");
        if (url && item->GetPath() == "" && IsPathToMedia(url))
          item->SetPath(url);
        
        const char * content_type = item_child->Attribute("type");
        if (content_type)
		{
          item->SetMimeType(content_type);
          CStdString strContentType(content_type);
          if (url && item->GetPath().IsEmpty() &&  
            (strContentType.Left(6).Equals("video/") ||
             strContentType.Left(6).Equals("audio/") 
            ))
            item->SetPath(url);
        }

        // Go over all child nodes of the media content and get the thumbnail
        TiXmlElement* media_content_child = item_child->FirstChildElement("media:thumbnail");
        if (media_content_child && media_content_child->Value() && strcmp(media_content_child->Value(), "media:thumbnail") == 0)
        {
          const char * url = media_content_child->Attribute("url");
          if (url && IsPathToThumbnail(url)) 
            item->SetThumbnailImage(url);
        }
      }
      else if(strcmp(item_child->Value(), "guid") == 0)
	  {
        if (item->GetPath().IsEmpty() && IsPathToMedia(item_child->Value()))
		{
          if(item_child->GetText())
            item->SetPath(item_child->GetText());
        }
      }
      else if(strcmp(item_child->Value(), "media:thumbnail") == 0 && IsPathToThumbnail(item_child->GetText())) 
        item->SetThumbnailImage(item_child->GetText());
      else if(strcmp(item_child->Value(), "media:thumbnail") == 0)
	  {
        const char * url = item_child->Attribute("url");
        if(url && IsPathToThumbnail(url))
          item->SetThumbnailImage(url);
      }
      else if(strcmp(item_child->Value(), "itunes:image") == 0 && IsPathToThumbnail(item_child->GetText())) 
        item->SetThumbnailImage(item_child->GetText());
      else if(strcmp(item_child->Value(), "itunes:image") == 0)
	  {
        const char * url = item_child->Attribute("href");
        if(url && IsPathToThumbnail(url))
          item->SetThumbnailImage(url);
      }
      else if(strcmp(item_child->Value(), "description") == 0)
	  {
        if(item_child->GetText())
		{
          CStdString description = item_child->GetText();
          while (description.Find("<") != -1)
          {
            int start = description.Find("<");
            int end = description.Find(">");
            if (end > start)
              description.Delete(start, end-start+1);
            else
              description.Delete(start, description.GetLength() - start);
          }
          description.Trim();
          item->SetProperty("description", description);
          item->SetLabel2(description);
        }
      }
      else if(strcmp(item_child->Value(), "itunes:summary") == 0)
	  {
        if(item_child->GetText())
		{
          CStdString description = item_child->GetText();
          item->SetProperty("description", description);
          item->SetLabel2(description);
        }
      }
      else if(strcmp(item_child->Value(), "itunes:subtitle") == 0)
	  {
        if(item_child->GetText())
		{
          CStdString description = item_child->GetText();
          item->SetProperty("description", description);
          item->SetLabel2(description);
        }
      }
      else if(strcmp(item_child->Value(), "itunes:author") == 0)
	  {
        if(item_child->GetText())
          item->SetProperty("author", item_child->GetText());
      }
      else if(strcmp(item_child->Value(), "itunes:duration") == 0)
	  {
        if(item_child->GetText())
          item->SetProperty("duration", item_child->GetText());
      }
      else if(strcmp(item_child->Value(), "itunes:keywords") == 0)
	  {
        if(item_child->GetText())
          item->SetProperty("keywords", item_child->GetText());
      }
    } // for item

    item->SetProperty("isrss", "1");
    item->SetProperty("chanthumb",strMediaThumbnail);

    if (!item->GetPath().IsEmpty())
    {
      EnterCriticalSection(m_ItemVectorLock);
      items.Add(item);
      LeaveCriticalSection(m_ItemVectorLock);
    }
  }

  return true;
}

bool CRssFeed::IsPathToMedia(const CStdString& strPath )
{
  CStdString extension;
  URIUtils::GetExtension(strPath, extension);

  if (extension.IsEmpty())
    return false;

  extension.ToLower();

  if (g_settings.m_videoExtensions.Find(extension) != -1)
    return true;

  if (g_settings.m_musicExtensions.Find(extension) != -1)
    return true;

  if (g_settings.m_pictureExtensions.Find(extension) != -1)
    return true;

  return false;
}

bool CRssFeed::IsPathToThumbnail(const CStdString& strPath )
{
  // Currently just check if this is an image, maybe we will add some
  // other checks later
  CStdString extension;
  URIUtils::GetExtension(strPath, extension);

  if (extension.IsEmpty())
    return false;

  extension.ToLower();

  if (g_settings.m_pictureExtensions.Find(extension) != -1)
    return true;

  return false;
}

