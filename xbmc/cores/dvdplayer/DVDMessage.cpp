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
 
#include "DVDMessage.h"
#include "DVDDemuxers/DVDDemuxUtils.h"
#include "DVDStreamInfo.h"


/**
 * CDVDMsgGeneralSynchronize --- GENERAL_SYNCRONIZR
 */
CDVDMsgGeneralSynchronize::CDVDMsgGeneralSynchronize(DWORD timeout, DWORD sources) : CDVDMsg(GENERAL_SYNCHRONIZE)
{
  if( sources )
    m_sources = sources;
  else
    m_sources = SYNCSOURCE_ALL;

  m_objects = 0;
  m_timeout = timeout;
}

void CDVDMsgGeneralSynchronize::Wait(volatile bool *abort, DWORD source)
{
  /* if we are not requested to wait on this object just return, reference count will be decremented */
  if (source && !(m_sources & source)) return;

  InterlockedIncrement(&m_objects);

  DWORD timeout = GetTickCount() + m_timeout;

  if (abort)
    while( m_objects < GetNrOfReferences() && timeout > GetTickCount() && !(*abort)) Sleep(1);
  else
    while( m_objects < GetNrOfReferences() && timeout > GetTickCount() ) Sleep(1);
}

/**
 * CDVDMsgDemuxerPacket --- DEMUXER_PACKET
 */
CDVDMsgDemuxerPacket::CDVDMsgDemuxerPacket(DemuxPacket* packet, bool drop) : CDVDMsg(DEMUXER_PACKET)
{
  m_packet = packet;
  m_drop   = drop;
}

CDVDMsgDemuxerPacket::~CDVDMsgDemuxerPacket()
{
  if (m_packet)
    CDVDDemuxUtils::FreeDemuxPacket(m_packet);
}
