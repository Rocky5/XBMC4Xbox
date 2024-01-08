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

#include "video/VideoInfoTag.h"
#include "XMLUtils.h"
#include "LocalizeStrings.h"
#include "settings/GUISettings.h"
#include "settings/AdvancedSettings.h"
#include "utils/log.h"
#include "utils/Variant.h"
#include "utils/CharsetConverter.h"
#include "pictures/Picture.h"

#include <sstream>

using namespace std;

void CVideoInfoTag::Reset()
{
  m_strDirector = "";
  m_strWritingCredits = "";
  m_strGenre = "";
  m_strCountry = "";
  m_strTagLine = "";
  m_strPlotOutline = "";
  m_strPlot = "";
  m_strPictureURL.Clear();
  m_strTitle = "";
  m_strOriginalTitle = "";
  m_strSortTitle = "";
  m_strVotes = "";
  m_cast.clear();
  m_strSet = "";
  m_strFile = "";
  m_strPath = "";
  m_strIMDBNumber = "";
  m_strMPAARating = "";
  m_strPremiered= "";
  m_strStatus= "";
  m_strProductionCode= "";
  m_strFirstAired= "";
  m_strStudio = "";
  m_strAlbum = "";
  m_strArtist = "";
  m_strTrailer = "";
  m_iTop250 = 0;
  m_iYear = 0;
  m_iSeason = -1;
  m_iEpisode = -1;
  m_iSpecialSortSeason = -1;
  m_iSpecialSortEpisode = -1;
  m_fRating = 0.0f;
  m_iDbId = -1;
  m_iFileId = -1;
  m_iBookmarkId = -1;
  m_iTrack = -1;
  m_fanart.m_xml = "";
  m_strRuntime = "";
  m_lastPlayed = "";
  m_strShowLink = "";
  m_streamDetails.Reset();
  m_playCount = 0;
  m_fEpBookmark = 0;
  m_resumePoint.Reset();
  m_resumePoint.type = CBookmark::RESUME;
}

bool CVideoInfoTag::Save(TiXmlNode *node, const CStdString &tag, bool savePathInfo)
{
  if (!node) return false;

  // we start with a <tag> tag
  TiXmlElement movieElement(tag.c_str());
  TiXmlNode *movie = node->InsertEndChild(movieElement);

  if (!movie) return false;

  XMLUtils::SetString(movie, "title", m_strTitle);
  if (!m_strOriginalTitle.IsEmpty())
    XMLUtils::SetString(movie, "originaltitle", m_strOriginalTitle);
  if (!m_strSortTitle.IsEmpty())
    XMLUtils::SetString(movie, "sorttitle", m_strSortTitle);
  XMLUtils::SetFloat(movie, "rating", m_fRating);
  XMLUtils::SetFloat(movie, "epbookmark", m_fEpBookmark);
  XMLUtils::SetInt(movie, "year", m_iYear);
  XMLUtils::SetInt(movie, "top250", m_iTop250);
  if (tag == "episodedetails" || tag == "tvshow")
  {
    XMLUtils::SetInt(movie, "season", m_iSeason);
    XMLUtils::SetInt(movie, "episode", m_iEpisode);
    XMLUtils::SetInt(movie, "displayseason",m_iSpecialSortSeason);
    XMLUtils::SetInt(movie, "displayepisode",m_iSpecialSortEpisode);
  }
  if (tag == "musicvideo")
  {
    XMLUtils::SetInt(movie, "track", m_iTrack);
    XMLUtils::SetString(movie, "album", m_strAlbum);
  }
  XMLUtils::SetString(movie, "votes", m_strVotes);
  XMLUtils::SetString(movie, "outline", m_strPlotOutline);
  XMLUtils::SetString(movie, "plot", m_strPlot);
  XMLUtils::SetString(movie, "tagline", m_strTagLine);
  XMLUtils::SetString(movie, "runtime", m_strRuntime);
  if (!m_strPictureURL.m_xml.empty())
  {
    TiXmlDocument doc;
    doc.Parse(m_strPictureURL.m_xml); 
    const TiXmlNode* thumb = doc.FirstChild("thumb");
    while (thumb)
    {
      movie->InsertEndChild(*thumb);
      thumb = thumb->NextSibling("thumb");
    }
  }
  if (m_fanart.m_xml.size())
  {
    TiXmlDocument doc;
    doc.Parse(m_fanart.m_xml);
    movie->InsertEndChild(*doc.RootElement());
  }
  XMLUtils::SetString(movie, "mpaa", m_strMPAARating);
  XMLUtils::SetInt(movie, "playcount", m_playCount);
  XMLUtils::SetString(movie, "lastplayed", m_lastPlayed);
  if (savePathInfo)
  {
    XMLUtils::SetString(movie, "file", m_strFile);
    XMLUtils::SetString(movie, "path", m_strPath);
    XMLUtils::SetString(movie, "filenameandpath", m_strFileNameAndPath);
  }
  if (!m_strEpisodeGuide.IsEmpty())
    XMLUtils::SetString(movie, "episodeguide", m_strEpisodeGuide);

  XMLUtils::SetString(movie, "id", m_strIMDBNumber);
  XMLUtils::SetAdditiveString(movie, "genre",
                          g_advancedSettings.m_videoItemSeparator, m_strGenre);
  XMLUtils::SetAdditiveString(movie, "country",
                          g_advancedSettings.m_videoItemSeparator, m_strCountry);
  XMLUtils::SetAdditiveString(movie, "set",
                          g_advancedSettings.m_videoItemSeparator, m_strSet);
  XMLUtils::SetAdditiveString(movie, "credits",
                          g_advancedSettings.m_videoItemSeparator, m_strWritingCredits);
  XMLUtils::SetAdditiveString(movie, "director",
                          g_advancedSettings.m_videoItemSeparator, m_strDirector);
  XMLUtils::SetString(movie, "premiered", m_strPremiered);
  XMLUtils::SetString(movie, "status", m_strStatus);
  XMLUtils::SetString(movie, "code", m_strProductionCode);
  XMLUtils::SetString(movie, "aired", m_strFirstAired);
  XMLUtils::SetAdditiveString(movie, "studio",
                          g_advancedSettings.m_videoItemSeparator, m_strStudio);
  XMLUtils::SetString(movie, "trailer", m_strTrailer);

  if (m_streamDetails.HasItems())
  {
    // it goes fileinfo/streamdetails/[video|audio|subtitle]
    TiXmlElement fileinfo("fileinfo");
    TiXmlElement streamdetails("streamdetails");
    for (int iStream=1; iStream<=m_streamDetails.GetVideoStreamCount(); iStream++)
    {
      TiXmlElement stream("video");
      XMLUtils::SetString(&stream, "codec", m_streamDetails.GetVideoCodec(iStream));
      XMLUtils::SetFloat(&stream, "aspect", m_streamDetails.GetVideoAspect(iStream));
      XMLUtils::SetInt(&stream, "width", m_streamDetails.GetVideoWidth(iStream));
      XMLUtils::SetInt(&stream, "height", m_streamDetails.GetVideoHeight(iStream));
      XMLUtils::SetInt(&stream, "durationinseconds", m_streamDetails.GetVideoDuration(iStream));
      streamdetails.InsertEndChild(stream);
    }
    for (int iStream=1; iStream<=m_streamDetails.GetAudioStreamCount(); iStream++)
    {
      TiXmlElement stream("audio");
      XMLUtils::SetString(&stream, "codec", m_streamDetails.GetAudioCodec(iStream));
      XMLUtils::SetString(&stream, "language", m_streamDetails.GetAudioLanguage(iStream));
      XMLUtils::SetInt(&stream, "channels", m_streamDetails.GetAudioChannels(iStream));
      streamdetails.InsertEndChild(stream);
    }
    for (int iStream=1; iStream<=m_streamDetails.GetSubtitleStreamCount(); iStream++)
    {
      TiXmlElement stream("subtitle");
      XMLUtils::SetString(&stream, "language", m_streamDetails.GetSubtitleLanguage(iStream));
      streamdetails.InsertEndChild(stream);
    }
    fileinfo.InsertEndChild(streamdetails);
    movie->InsertEndChild(fileinfo);
  }  /* if has stream details */

  // cast
  for (iCast it = m_cast.begin(); it != m_cast.end(); ++it)
  {
    // add a <actor> tag
    TiXmlElement cast("actor");
    TiXmlNode *node = movie->InsertEndChild(cast);
    TiXmlElement actor("name");
    TiXmlNode *actorNode = node->InsertEndChild(actor);
    TiXmlText name(it->strName);
    actorNode->InsertEndChild(name);
    TiXmlElement role("role");
    TiXmlNode *roleNode = node->InsertEndChild(role);
    TiXmlText character(it->strRole);
    roleNode->InsertEndChild(character);
    TiXmlElement thumb("thumb");
    TiXmlNode *thumbNode = node->InsertEndChild(thumb);
    TiXmlText th(it->thumbUrl.GetFirstThumb().m_url);
    thumbNode->InsertEndChild(th);
  }
  XMLUtils::SetAdditiveString(movie, "artist",
                         g_advancedSettings.m_videoItemSeparator, m_strArtist);
  XMLUtils::SetAdditiveString(movie, "showlink",
                         g_advancedSettings.m_videoItemSeparator, m_strShowLink);
 
  TiXmlElement resume("resume");
  XMLUtils::SetFloat(&resume, "position", (float)m_resumePoint.timeInSeconds);
  XMLUtils::SetFloat(&resume, "total", (float)m_resumePoint.totalTimeInSeconds);
  movie->InsertEndChild(resume);

  return true;
}

bool CVideoInfoTag::Load(const TiXmlElement *movie, bool chained /* = false */)
{
  if (!movie) return false;

  // reset our details if we aren't chained.
  if (!chained) Reset();

  if (CStdString("Title").Equals(movie->Value())) // mymovies.xml
    ParseMyMovies(movie);
  else
    ParseNative(movie);

  return true;
}

void CVideoInfoTag::Archive(CArchive& ar)
{
  if (ar.IsStoring())
  {
    ar << m_strDirector;
    ar << m_strWritingCredits;
    ar << m_strGenre;
    ar << m_strCountry;
    ar << m_strTagLine;
    ar << m_strPlotOutline;
    ar << m_strPlot;
    ar << m_strPictureURL.m_spoof;
    ar << m_strPictureURL.m_xml;
    ar << m_fanart.m_xml;
    ar << m_strTitle;
    ar << m_strSortTitle;
    ar << m_strVotes;
    ar << m_strStudio;
    ar << m_strTrailer;
    ar << (int)m_cast.size();
    for (unsigned int i=0;i<m_cast.size();++i)
    {
      ar << m_cast[i].strName;
      ar << m_cast[i].strRole;
      ar << m_cast[i].thumbUrl.m_xml;
    }
    
    ar << m_strSet;
    ar << m_strRuntime;
    ar << m_strFile;
    ar << m_strPath;
    ar << m_strIMDBNumber;
    ar << m_strMPAARating;
    ar << m_strFileNameAndPath;
    ar << m_strOriginalTitle;
    ar << m_strEpisodeGuide;
    ar << m_strPremiered;
    ar << m_strStatus;
    ar << m_strProductionCode;
    ar << m_strFirstAired;
    ar << m_strShowTitle;
    ar << m_strAlbum;
    ar << m_strArtist;
    ar << m_playCount;
    ar << m_lastPlayed;
    ar << m_iTop250;
    ar << m_iYear;
    ar << m_iSeason;
    ar << m_iEpisode;
    ar << m_fRating;
    ar << m_iDbId;
    ar << m_iFileId;
    ar << m_iSpecialSortSeason;
    ar << m_iSpecialSortEpisode;
    ar << m_iBookmarkId;
    ar << m_iTrack;
    ar << dynamic_cast<IArchivable&>(m_streamDetails);
    ar << m_strShowLink;
    ar << m_fEpBookmark;
    ar << m_resumePoint.timeInSeconds;
    ar << m_resumePoint.totalTimeInSeconds;
  }
  else
  {
    ar >> m_strDirector;
    ar >> m_strWritingCredits;
    ar >> m_strGenre;
    ar >> m_strCountry;
    ar >> m_strTagLine;
    ar >> m_strPlotOutline;
    ar >> m_strPlot;
    ar >> m_strPictureURL.m_spoof;
    ar >> m_strPictureURL.m_xml;
    m_strPictureURL.Parse();
    ar >> m_fanart.m_xml;
    m_fanart.Unpack();
    ar >> m_strTitle;
    ar >> m_strSortTitle;
    ar >> m_strVotes;
    ar >> m_strStudio;
    ar >> m_strTrailer;
    int iCastSize;
    ar >> iCastSize;
    for (int i=0;i<iCastSize;++i)
    {
      SActorInfo info;
      ar >> info.strName;
      ar >> info.strRole;
      CStdString strXml;
      ar >> strXml;
      info.thumbUrl.ParseString(strXml);
      m_cast.push_back(info);
    }
    
    ar >> m_strSet;
    ar >> m_strRuntime;
    ar >> m_strFile;
    ar >> m_strPath;
    ar >> m_strIMDBNumber;
    ar >> m_strMPAARating;
    ar >> m_strFileNameAndPath;
    ar >> m_strOriginalTitle;
    ar >> m_strEpisodeGuide;
    ar >> m_strPremiered;
    ar >> m_strStatus;
    ar >> m_strProductionCode;
    ar >> m_strFirstAired;
    ar >> m_strShowTitle;
    ar >> m_strAlbum;
    ar >> m_strArtist;
    ar >> m_playCount;
    ar >> m_lastPlayed;
    ar >> m_iTop250;
    ar >> m_iYear;
    ar >> m_iSeason;
    ar >> m_iEpisode;
    ar >> m_fRating;
    ar >> m_iDbId;
    ar >> m_iFileId;
    ar >> m_iSpecialSortSeason;
    ar >> m_iSpecialSortEpisode;
    ar >> m_iBookmarkId;
    ar >> m_iTrack;
    ar >> dynamic_cast<IArchivable&>(m_streamDetails);
    ar >> m_strShowLink;
    ar >> m_fEpBookmark;
    ar >> m_resumePoint.timeInSeconds;
    ar >> m_resumePoint.totalTimeInSeconds;
  }
}

void CVideoInfoTag::Serialize(CVariant& value)
{
  value["director"] = m_strDirector;
  value["writingcredits"] = m_strWritingCredits;
  value["genre"] = m_strGenre;
  value["country"] = m_strCountry;
  value["tagline"] = m_strTagLine;
  value["plotoutline"] = m_strPlotOutline;
  value["plot"] = m_strPlot;
  value["title"] = m_strTitle;
  value["votes"] = m_strVotes;
  value["studio"] = m_strStudio;
  value["trailer"] = m_strTrailer;
  for (unsigned int i = 0; i < m_cast.size(); ++i)
  {
    value["cast"][i]["name"] = m_cast[i].strName;
    value["cast"][i]["role"] = m_cast[i].strRole;
  }
  value["set"] = m_strSet;
  value["runtime"] = m_strRuntime;
  value["file"] = m_strFile;
  value["path"] = m_strPath;
  value["imdbnumber"] = m_strIMDBNumber;
  value["filenameandpath"] = m_strFileNameAndPath;
  value["originaltitle"] = m_strOriginalTitle;
  value["episodeguide"] = m_strEpisodeGuide;
  value["premiered"] = m_strPremiered;
  value["status"] = m_strStatus;
  value["productioncode"] = m_strProductionCode;
  value["firstaired"] = m_strFirstAired;
  value["showtitle"] = m_strShowTitle;
  value["album"] = m_strAlbum;
  value["artist"] = m_strArtist;
  value["playcount"] = m_playCount;
  value["lastPlayed"] = m_lastPlayed;
  value["top250"] = m_iTop250;
  value["year"] = m_iYear;
  value["season"] = m_iSeason;
  value["episode"] = m_iEpisode;
  value["rating"] = m_fRating;
  value["dbid"] = m_iDbId;
  value["fileid"] = m_iFileId;
  value["track"] = m_iTrack;
  value["showlink"] = m_strShowLink;
  m_streamDetails.Serialize(value["streamDetails"]);
  CVariant resume = CVariant(CVariant::VariantTypeObject);
  resume["position"] = (float)m_resumePoint.timeInSeconds;
  resume["total"] = (float)m_resumePoint.totalTimeInSeconds;
  value["resume"] = resume;
}

const CStdString CVideoInfoTag::GetCast(bool bIncludeRole /*= false*/) const
{
  CStdString strLabel;
  for (iCast it = m_cast.begin(); it != m_cast.end(); ++it)
  {
    CStdString character;
    if (it->strRole.IsEmpty() || !bIncludeRole)
      character.Format("%s\n", it->strName.c_str());
    else
      character.Format("%s %s %s\n", it->strName.c_str(), g_localizeStrings.Get(20347).c_str(), it->strRole.c_str());
    strLabel += character;
  }
  return strLabel.TrimRight("\n");
}

void CVideoInfoTag::ParseNative(const TiXmlElement* movie)
{
  XMLUtils::GetString(movie, "title", m_strTitle);
  XMLUtils::GetString(movie, "originaltitle", m_strOriginalTitle);
  XMLUtils::GetString(movie, "sorttitle", m_strSortTitle);
  XMLUtils::GetFloat(movie, "rating", m_fRating);
  XMLUtils::GetFloat(movie, "epbookmark", m_fEpBookmark);
  int max_value = 10;
  const TiXmlElement* rElement = movie->FirstChildElement("rating");
  if (rElement && (rElement->QueryIntAttribute("max", &max_value) == TIXML_SUCCESS) && max_value>=1)
  {    
    m_fRating = m_fRating / max_value * 10; // Normalise the Movie Rating to between 1 and 10
  }
  XMLUtils::GetInt(movie, "year", m_iYear);
  XMLUtils::GetInt(movie, "top250", m_iTop250);
  XMLUtils::GetInt(movie, "season", m_iSeason);
  XMLUtils::GetInt(movie, "episode", m_iEpisode);
  XMLUtils::GetInt(movie, "track", m_iTrack);
  XMLUtils::GetInt(movie, "displayseason", m_iSpecialSortSeason);
  XMLUtils::GetInt(movie, "displayepisode", m_iSpecialSortEpisode);
  int after=0;
  XMLUtils::GetInt(movie, "displayafterseason",after);
  if (after > 0)
  {
    m_iSpecialSortSeason = after;
    m_iSpecialSortEpisode = 0x1000; // should be more than any realistic episode number
  }
  XMLUtils::GetString(movie, "votes", m_strVotes);
  XMLUtils::GetString(movie, "outline", m_strPlotOutline);
  XMLUtils::GetString(movie, "plot", m_strPlot);
  XMLUtils::GetString(movie, "tagline", m_strTagLine);
  XMLUtils::GetString(movie, "runtime", m_strRuntime);
  XMLUtils::GetString(movie, "mpaa", m_strMPAARating);
  XMLUtils::GetInt(movie, "playcount", m_playCount);
  XMLUtils::GetString(movie, "lastplayed", m_lastPlayed);
  XMLUtils::GetString(movie, "file", m_strFile);
  XMLUtils::GetString(movie, "path", m_strPath);
  XMLUtils::GetString(movie, "id", m_strIMDBNumber);
  XMLUtils::GetString(movie, "filenameandpath", m_strFileNameAndPath);
  XMLUtils::GetString(movie, "premiered", m_strPremiered);
  XMLUtils::GetString(movie, "status", m_strStatus);
  XMLUtils::GetString(movie, "code", m_strProductionCode);
  XMLUtils::GetString(movie, "aired", m_strFirstAired);
  XMLUtils::GetString(movie, "album", m_strAlbum);
  XMLUtils::GetString(movie, "trailer", m_strTrailer);

  const TiXmlElement* thumb = movie->FirstChildElement("thumb");
  while (thumb)
  {
    m_strPictureURL.ParseElement(thumb);
    thumb = thumb->NextSiblingElement("thumb");
  }

  XMLUtils::GetAdditiveString(movie,"genre",g_advancedSettings.m_videoItemSeparator,m_strGenre);
  XMLUtils::GetAdditiveString(movie,"country",g_advancedSettings.m_videoItemSeparator,m_strCountry);
  XMLUtils::GetAdditiveString(movie,"credits",g_advancedSettings.m_videoItemSeparator,m_strWritingCredits);
  XMLUtils::GetAdditiveString(movie,"director",g_advancedSettings.m_videoItemSeparator,m_strDirector);
  XMLUtils::GetAdditiveString(movie,"showlink",g_advancedSettings.m_videoItemSeparator,m_strShowLink);

  // cast
  const TiXmlElement* node = movie->FirstChildElement("actor");
  while (node)
  {
    const TiXmlNode *actor = node->FirstChild("name");
    if (actor && actor->FirstChild())
    {
      SActorInfo info;
      info.strName = actor->FirstChild()->Value();
      const TiXmlNode *roleNode = node->FirstChild("role");
      if (roleNode && roleNode->FirstChild())
        info.strRole = roleNode->FirstChild()->Value();
      const TiXmlElement* thumb = node->FirstChildElement("thumb");
      while (thumb)
      {
        info.thumbUrl.ParseElement(thumb);
        thumb = thumb->NextSiblingElement("thumb");
      }
      const char* clear=node->Attribute("clear");
      if (clear && stricmp(clear,"true"))
        m_cast.clear();
      m_cast.push_back(info);
    }
    node = node->NextSiblingElement("actor");
  }
  XMLUtils::GetAdditiveString(movie,"set",g_advancedSettings.m_videoItemSeparator,m_strSet);
  XMLUtils::GetAdditiveString(movie,"studio",g_advancedSettings.m_videoItemSeparator,m_strStudio);
  // artists
  node = movie->FirstChildElement("artist");
  while (node)
  {
    const TiXmlNode* pNode = node->FirstChild("name");
    const char* pValue=NULL;
    if (pNode && pNode->FirstChild())
      pValue = pNode->FirstChild()->Value();
    else if (node->FirstChild())
      pValue = node->FirstChild()->Value();
    if (pValue)
    {
      const char* clear=node->Attribute("clear");
      if (m_strArtist.IsEmpty() || (clear && stricmp(clear,"true")==0))
        m_strArtist += pValue;
      else
        m_strArtist += g_advancedSettings.m_videoItemSeparator + pValue;
    }
    node = node->NextSiblingElement("artist");
  }

  m_streamDetails.Reset();
  node = movie->FirstChildElement("fileinfo");
  if (node)
  {
    // Try to pull from fileinfo/streamdetails/[video|audio|subtitle] 
    const TiXmlNode *nodeStreamDetails = node->FirstChild("streamdetails");
    if (nodeStreamDetails)
    {
      const TiXmlNode *nodeDetail = NULL;
      while ((nodeDetail = nodeStreamDetails->IterateChildren("audio", nodeDetail)))
      {
        CStreamDetailAudio *p = new CStreamDetailAudio();
        XMLUtils::GetString(nodeDetail, "codec", p->m_strCodec);
        XMLUtils::GetString(nodeDetail, "language", p->m_strLanguage);
        XMLUtils::GetInt(nodeDetail, "channels", p->m_iChannels);
        p->m_strCodec.MakeLower();
        p->m_strLanguage.MakeLower();
        m_streamDetails.AddStream(p);
      }
      nodeDetail = NULL;
      while ((nodeDetail = nodeStreamDetails->IterateChildren("video", nodeDetail)))
      {
        CStreamDetailVideo *p = new CStreamDetailVideo();
        XMLUtils::GetString(nodeDetail, "codec", p->m_strCodec);
        XMLUtils::GetFloat(nodeDetail, "aspect", p->m_fAspect);
        XMLUtils::GetInt(nodeDetail, "width", p->m_iWidth);
        XMLUtils::GetInt(nodeDetail, "height", p->m_iHeight);
        XMLUtils::GetInt(nodeDetail, "durationinseconds", p->m_iDuration);
        p->m_strCodec.MakeLower();
        m_streamDetails.AddStream(p);
      }
      nodeDetail = NULL;
      while ((nodeDetail = nodeStreamDetails->IterateChildren("subtitle", nodeDetail)))
      {
        CStreamDetailSubtitle *p = new CStreamDetailSubtitle();
        XMLUtils::GetString(nodeDetail, "language", p->m_strLanguage);
        p->m_strLanguage.MakeLower();
        m_streamDetails.AddStream(p);
      }
    }
    m_streamDetails.DetermineBestStreams();
  }  /* if fileinfo */

  const TiXmlElement *epguide = movie->FirstChildElement("episodeguide");
  if (epguide)
  {
    // DEPRECIATE ME - support for old XML-encoded <episodeguide> blocks.
    if (epguide->FirstChild() && strnicmp("<episodeguide", epguide->FirstChild()->Value(), 13) == 0)
      m_strEpisodeGuide = epguide->FirstChild()->Value();
    else
    {
      stringstream stream;
      stream << *epguide;
      m_strEpisodeGuide = stream.str();
    }
  }

  // fanart
  const TiXmlElement *fanart = movie->FirstChildElement("fanart");
  if (fanart)
  {
    m_fanart.m_xml << *fanart;
    m_fanart.Unpack();
  }

  // resumePoint
  const TiXmlNode *resume = movie->FirstChild("resume");
  if (resume)
  {
    XMLUtils::GetDouble(resume, "position", m_resumePoint.timeInSeconds);
    XMLUtils::GetDouble(resume, "total", m_resumePoint.totalTimeInSeconds);
  }
}

void CVideoInfoTag::ParseMyMovies(const TiXmlElement *movie)
{
  XMLUtils::GetString(movie, "LocalTitle", m_strTitle);
  XMLUtils::GetString(movie, "OriginalTitle", m_strOriginalTitle);
  XMLUtils::GetString(movie, "SortTitle", m_strSortTitle);
  XMLUtils::GetFloat(movie, "Rating", m_fRating);
  XMLUtils::GetString(movie, "IMDB", m_strIMDBNumber);
  XMLUtils::GetInt(movie, "ProductionYear", m_iYear);
  int runtime = 0;
  XMLUtils::GetInt(movie, "RunningTime", runtime);
  m_strRuntime.Format("%i:%02d", runtime/60, runtime%60); // convert from minutes to hh:mm
  XMLUtils::GetString(movie, "TagLine", m_strTagLine);
  XMLUtils::GetString(movie, "Description", m_strPlot);
  if (m_strTagLine.IsEmpty())
  {
    if (m_strPlot.find("\r\n") > 0)
      m_strPlotOutline = m_strPlot.substr(0, m_strPlot.find("\r\n") - 1);
    else
      m_strPlotOutline = m_strPlot;
  }

  // thumb
  CStdString strTemp;
  const TiXmlNode *node = movie->FirstChild("Covers");
  while (node)
  {
    const TiXmlNode *front = node->FirstChild("Front");
    if (front && front->FirstChild())
    {
      strTemp = front->FirstChild()->Value();
      if (!strTemp.IsEmpty())
        m_strPictureURL.ParseString(strTemp);
    }
    node = node->NextSibling("Covers");
  }
  // genres
  node = movie->FirstChild("Genres");
  const TiXmlNode *genre = node ? node->FirstChildElement("Genre") : NULL;
  while (genre)
  {
    if (genre && genre->FirstChild())
    {
      strTemp = genre->FirstChild()->Value();
      if (m_strGenre.IsEmpty())
        m_strGenre = strTemp;
      else
        m_strGenre += g_advancedSettings.m_videoItemSeparator+strTemp;
    }
    genre = genre->NextSiblingElement("Genre");
  }
  // countries
  node = movie->FirstChild("Countries");
  const TiXmlNode *country = node ? node->FirstChildElement("Country") : NULL;
  while (country)
  {
    if (country && country->FirstChild())
    {
      strTemp = country->FirstChild()->Value();
      if (m_strCountry.IsEmpty())
        m_strCountry = strTemp;
      else
        m_strCountry += g_advancedSettings.m_videoItemSeparator+strTemp;
    }
    country = country->NextSibling("Countries");
  }
  // MyMovies categories to genres
  if (g_advancedSettings.m_bVideoLibraryMyMoviesCategoriesToGenres)
  {
    node = movie->FirstChild("Categories");
    const TiXmlNode *category = node ? node->FirstChildElement("Category") : NULL;
    while (category)
    {
      if (category && category->FirstChild())
      {
        strTemp = category->FirstChild()->Value();
        if (m_strGenre.IsEmpty())
          m_strGenre = strTemp;
        else
          m_strGenre += g_advancedSettings.m_videoItemSeparator+strTemp;
      }
      category = category->NextSiblingElement("Category");
    }
  }
  // studios
  node = movie->FirstChild("Studios");
  while (node)
  {
    const TiXmlNode *studio = node->FirstChild("Studio");
    if (studio && studio->FirstChild())
    {
      strTemp = studio->FirstChild()->Value();
      if (m_strStudio.IsEmpty())
        m_strStudio = strTemp;
      else
        m_strStudio += g_advancedSettings.m_videoItemSeparator+strTemp;
    }
    node = node->NextSibling("Studios");
  }
  // persons
  int personType = -1;
  node = movie->FirstChild("Persons");
  const TiXmlElement *element = node ? node->FirstChildElement("Person") : NULL;
  while (element)
  {
    element->Attribute("Type", &personType);
    const TiXmlNode *person = element->FirstChild("Name");
    if (person && person->FirstChild())
    {
      if (personType == 1) // actor
      {
        SActorInfo info;
        info.strName = person->FirstChild()->Value();
        const TiXmlNode *roleNode = element->FirstChild("Role");
        if (roleNode && roleNode->FirstChild())
          info.strRole = roleNode->FirstChild()->Value();
        m_cast.push_back(info);
      }
      else if (personType == 2) // director
      {
        strTemp = person->FirstChild()->Value();
        if (m_strDirector.IsEmpty())
          m_strDirector = strTemp;
        else
          m_strDirector += g_advancedSettings.m_videoItemSeparator+strTemp;
      }
    }
    element = element->NextSiblingElement("Person");
  }
}

bool CVideoInfoTag::HasStreamDetails() const
{
  return m_streamDetails.HasItems();
}

bool CVideoInfoTag::IsEmpty() const
{
  return (m_strTitle.IsEmpty() &&
          m_strFile.IsEmpty() &&
          m_strPath.IsEmpty());
}
