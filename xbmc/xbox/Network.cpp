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

#include "system.h"

#include "Network.h"
#ifdef HAS_XBOX_NETWORK
#include "Undocumented.h"
#endif
#include "Application.h"
#include "FileSystem/SmbFile.h"
#include "lib/libscrobbler/lastfmscrobbler.h"
#include "lib/libscrobbler/librefmscrobbler.h"
#include "settings/Settings.h"
#include "GUIWindowManager.h"
#include "ApplicationMessenger.h"
#include "utils/RssReader.h"
#include "utils/Weather.h"
#include "utils/log.h"

// Time to wait before we give up on network init
#define WAIT_TIME 10000

#ifdef _XBOX
static char* inet_ntoa (struct in_addr in)
{
  static char _inetaddress[32];
  sprintf(_inetaddress, "%d.%d.%d.%d", in.S_un.S_un_b.s_b1, in.S_un.S_un_b.s_b2, in.S_un.S_un_b.s_b3, in.S_un.S_un_b.s_b4);
  return _inetaddress;
}
#endif

#ifdef HAS_XBOX_NETWORK
/* translator function wich will take our network info and make TXNetConfigParams of it */
/* returns true if config is different from default */
static bool TranslateConfig( const struct network_info& networkinfo, TXNetConfigParams &params )
{    
  bool bDirty = false;

  if ( !networkinfo.DHCP )
  {
    bool bXboxVersion2 = (params.V2_Tag == 0x58425632 );  // "XBV2"

    if (bXboxVersion2)
    {
      if (params.V2_IP.s_addr != inet_addr(networkinfo.ip))
      {
        params.V2_IP.s_addr = inet_addr(networkinfo.ip);
        bDirty = true;
      }
    }
    else
    {
      if (params.V1_IP.s_addr != inet_addr(networkinfo.ip))
      {
        params.V1_IP.s_addr = inet_addr(networkinfo.ip);
        bDirty = true;
      }
    }

    if (bXboxVersion2)
    {
      if (params.V2_Subnetmask.s_addr != inet_addr(networkinfo.subnet))
      {
        params.V2_Subnetmask.s_addr = inet_addr(networkinfo.subnet);
        bDirty = true;
      }
    }
    else
    {
      if (params.V1_Subnetmask.s_addr != inet_addr(networkinfo.subnet))
      {
        params.V1_Subnetmask.s_addr = inet_addr(networkinfo.subnet);
        bDirty = true;
      }
    }

    if (bXboxVersion2)
    {
      if (params.V2_Defaultgateway.s_addr != inet_addr(networkinfo.gateway))
      {
        params.V2_Defaultgateway.s_addr = inet_addr(networkinfo.gateway);
        bDirty = true;
      }
    }
    else
    {
      if (params.V1_Defaultgateway.s_addr != inet_addr(networkinfo.gateway))
      {
        params.V1_Defaultgateway.s_addr = inet_addr(networkinfo.gateway);
        bDirty = true;
      }
    }

    if (bXboxVersion2)
    {
      if (params.V2_DNS1.s_addr != inet_addr(networkinfo.DNS1))
      {
        params.V2_DNS1.s_addr = inet_addr(networkinfo.DNS1);
        bDirty = true;
      }
    }
    else
    {
      if (params.V1_DNS1.s_addr != inet_addr(networkinfo.DNS1))
      {
        params.V1_DNS1.s_addr = inet_addr(networkinfo.DNS1);
        bDirty = true;
      }
    }

    if (bXboxVersion2)
    {
      if (params.V2_DNS2.s_addr != inet_addr(networkinfo.DNS2))
      {
        params.V2_DNS2.s_addr = inet_addr(networkinfo.DNS2);
        bDirty = true;
      }
    }
    else
    {
      if (params.V1_DNS2.s_addr != inet_addr(networkinfo.DNS2))
      {
        params.V1_DNS2.s_addr = inet_addr(networkinfo.DNS2);
        bDirty = true;
      }
    }

    if (params.Flag != (0x04 | 0x08) )
    {
      params.Flag = 0x04 | 0x08;
      bDirty = true;
    }
  }
  else
  {

    if( params.Flag != 0 )
    {
      params.Flag = 0;
      bDirty = true;
    }

#if 0
    TXNetConfigParams oldconfig;
    memcpy(&oldconfig, &params, sizeof(TXNetConfigParams));

    oldconfig.Flag = 0;

    unsigned char *raw = (unsigned char*)&params;
    
    //memset( raw, 0, sizeof(params)); /* shouldn't be needed, xbox should still remember what ip's where set statically */

    /**     Set DHCP-flags from a known DHCP mode  (maybe some day we will fix this)  **/
    /* i'm guessing these are some dhcp options */
    /* debug dash doesn't set them like this thou */
    /* i have a feeling we don't even need to touch these */
    raw[40] = 33;  raw[41] = 223; raw[42] = 196; raw[43] = 67;    //  param.Data_28
    raw[44] = 6;   raw[45] = 145; raw[46] = 157; raw[47] = 118;   //  param.Data_2c
    raw[48] = 182; raw[49] = 239; raw[50] = 68;  raw[51] = 197;   //  param.Data_30
    raw[52] = 133; raw[53] = 150; raw[54] = 118; raw[55] = 211;   //  param.Data_34
    raw[56] = 38;  raw[57] = 87;  raw[58] = 222; raw[59] = 119;   //  param.Data_38
    
    /* clears static ip flag */
    raw[64] = 0; // first part of params.Flag.. wonder if params.Flag really should be a DWORD
    
    //raw[72] = 0; raw[73] = 0; raw[74] = 0; raw[75] = 0; /* this would have cleared the v2 ip, just silly */
    
    /* no idea what this is, could be additional dhcp options, but's hard to tell */
    raw[340] = 160; raw[341] = 93; raw[342] = 131; raw[343] = 191; raw[344] = 46;

    /* if something was changed, update with this */
    if( memcmp(&oldconfig, &params, sizeof(TXNetConfigParams)) != 0 ) 
      bDirty = true;
#endif
  }

  return bDirty;
}
#endif

bool CNetwork::Initialize(int iAssignment, const char* szLocalAddress, const char* szLocalSubnet, const char* szLocalGateway, const char* szNameServer, const char* szNameServerAlt)
{
#ifdef HAS_XBOX_NETWORK
  XNetStartupParams xnsp = {};
  WSADATA WsaData = {};
  TXNetConfigParams params = {};
  DWORD dwState = 0;
  bool dashconfig = false;

  memset(&m_networkinfo , 0, sizeof(m_networkinfo ));

  /* load current params */
  XNetLoadConfigParams( &params );

  if (iAssignment == NETWORK_DHCP)
  {
    m_networkinfo.DHCP = true;    
    strcpy(m_networkinfo.ip, "0.0.0.0");
    
    TranslateConfig(m_networkinfo, params);
    CLog::Log(LOGNOTICE, "Network: Using DHCP IP settings");
  }
  else if (iAssignment == NETWORK_STATIC)
  {
    m_networkinfo.DHCP = false;
    strcpy(m_networkinfo.ip, szLocalAddress);
    strcpy(m_networkinfo.subnet, szLocalSubnet);
    strcpy(m_networkinfo.gateway, szLocalGateway);
    strcpy(m_networkinfo.DNS1, szNameServer);
    strcpy(m_networkinfo.DNS2, szNameServerAlt);

    TranslateConfig(m_networkinfo, params);
    CLog::Log(LOGNOTICE, "Network: Using static IP settings");
  }
  else
  {
    dashconfig = true;
    CLog::Log(LOGNOTICE, "Network: Using dashboard IP settings");
  }

  /* configure addresses */  
  if( !dashconfig )
  { 
    /* override dashboard setting with this, if it was different */      
    XNetSaveConfigParams( &params );    
  }

  // Zero struct, just in case
  memset(&xnsp, 0, sizeof(xnsp));

  /* okey now startup the settings we wish to use */
  xnsp.cfgSizeOfStruct = sizeof(xnsp);

  // Bypass security so that we may connect to 'untrusted' hosts
  xnsp.cfgFlags = XNET_STARTUP_BYPASS_SECURITY;
  // create more memory for networking
  xnsp.cfgPrivatePoolSizeInPages = 64; // == 256kb, default = 12 (48kb)
  xnsp.cfgEnetReceiveQueueLength = 16; // == 32kb, default = 8 (16kb)
  xnsp.cfgIpFragMaxSimultaneous = 16; // default = 4
  xnsp.cfgIpFragMaxPacketDiv256 = 32; // == 8kb, default = 8 (2kb)
  xnsp.cfgSockMaxSockets = 64; // default = 64
  xnsp.cfgSockDefaultRecvBufsizeInK = 128; // default = 16
  xnsp.cfgSockDefaultSendBufsizeInK = 128; // default = 16

  dwState = XNetStartup(&xnsp);
  if( dwState != 0 )
  {
    CLog::Log(LOGERROR, __FUNCTION__" - XNetStartup failed with error %d", dwState);
    return false;
  }

  if( !dashconfig )
  {
    dwState = XNetConfig( &params, 0 );
    if( dwState != 0 )
    {
      CLog::Log(LOGERROR, __FUNCTION__" - XNetConfig failed with error %d", dwState);
      return false;
    }      
  }

  /* startup winsock */  
  dwState = WSAStartup( MAKEWORD(2, 2), &WsaData );
  if( NO_ERROR != dwState )
  {
    CLog::Log(LOGERROR, __FUNCTION__" - WSAStartup failed with error %d", dwState);
    return false;
  }

#endif
  m_inited = true;
  return true;
}

void CNetwork::NetworkDown()
{
  CLog::Log(LOGDEBUG, "%s - Network service is down", __FUNCTION__);
  
  memset(&m_networkinfo, 0, sizeof(m_networkinfo));
  m_lastlink = 0;
  m_laststate = 0;
  m_networkup = false;
  g_application.getApplicationMessenger().NetworkMessage(SERVICES_DOWN, 0);
}

void CNetwork::NetworkUp()
{
  CLog::Log(LOGDEBUG, "%s - Network service is up", __FUNCTION__);
#ifdef HAS_XBOX_NETWORK
  
  /* get the current status */
  TXNetConfigStatus status;
  XNetGetConfigStatus(&status);

  /* fill local network info */
  strcpy(m_networkinfo.ip, inet_ntoa(status.ip));
  strcpy(m_networkinfo.subnet, inet_ntoa(status.subnet));
  strcpy(m_networkinfo.gateway, inet_ntoa(status.gateway));
  strcpy(m_networkinfo.dhcpserver, "");
  strcpy(m_networkinfo.DNS1, inet_ntoa(status.dns1));
  strcpy(m_networkinfo.DNS2, inet_ntoa(status.dns2));

  m_networkinfo.DHCP = !(status.dhcp == 0);
#endif

  m_networkup = true;
  
  g_application.getApplicationMessenger().NetworkMessage(SERVICES_UP, 0);
}

/* update network state, call repeatedly while return value is XNET_GET_XNADDR_PENDING */
DWORD CNetwork::UpdateState()
{
#ifdef HAS_XBOX_NETWORK
  CSingleLock lock (m_critSection);
  
  XNADDR xna;
  DWORD dwState = XNetGetTitleXnAddr(&xna);
  DWORD dwLink = XNetGetEthernetLinkStatus();

  if( m_lastlink != dwLink || m_laststate != dwState )
  {
    if (m_networkup)
      NetworkDown();

    m_lastlink = dwLink;
    m_laststate = dwState;

    if ((dwLink & XNET_ETHERNET_LINK_ACTIVE) && (dwState & XNET_GET_XNADDR_DHCP || dwState & XNET_GET_XNADDR_STATIC) && !(dwState & XNET_GET_XNADDR_NONE || dwState & XNET_GET_XNADDR_TROUBLESHOOT || dwState & XNET_GET_XNADDR_PENDING))
      NetworkUp();
    
    LogState();
  }

  return dwState;
#else
  NetworkUp();
  return 0;
#endif
}

bool CNetwork::CheckNetwork(int count)
{
#ifdef HAS_XBOX_NETWORK
  static DWORD lastLink;  // will hold the last link, to notice changes
  static int netRetryCounter;

  // get our network state
  DWORD dwState = UpdateState();
  DWORD dwLink = XNetGetEthernetLinkStatus();
  
  // Check the network status every count itterations
  if (++netRetryCounter > count || lastLink != dwLink)
  {
    netRetryCounter = 0;
    lastLink = dwLink;
    
    // In case the network failed, try to set it up again
    if ( !(dwLink & XNET_ETHERNET_LINK_ACTIVE) || !IsInited() || dwState & XNET_GET_XNADDR_NONE || dwState & XNET_GET_XNADDR_TROUBLESHOOT )
    {
      Deinitialize();

      if (dwLink & XNET_ETHERNET_LINK_ACTIVE)
      {
        CLog::Log(LOGWARNING, "%s - Network error. Trying re-setup", __FUNCTION__);
        SetupNetwork();
        return true;
      }
    }
  }
  return false;
#else
  return true;
#endif
}

bool CNetwork::SetupNetwork()
{
  // setup network based on our settings
  // network will start its init procedure but ethernet must be connected
  if (IsEthernetConnected())
  {
    CLog::Log(LOGDEBUG, "%s - Setting up network...", __FUNCTION__);
    
    Initialize(g_guiSettings.GetInt("network.assignment"),
      g_guiSettings.GetString("network.ipaddress").c_str(),
      g_guiSettings.GetString("network.subnet").c_str(),
      g_guiSettings.GetString("network.gateway").c_str(),
      g_guiSettings.GetString("network.dns").c_str(),
      g_guiSettings.GetString("network.dns2").c_str());
      
    return true;
  }
  
  // Setup failed
  CLog::Log(LOGDEBUG, "%s - Not setting up network as ethernet is not connected!", __FUNCTION__);
  return false;
}

bool CNetwork::IsEthernetConnected()
{
#ifdef HAS_XBOX_NETWORK
  if (!(XNetGetEthernetLinkStatus() & XNET_ETHERNET_LINK_ACTIVE))
    return false;
#endif

  return true;
}

bool CNetwork::WaitForSetup(DWORD timeout)
{
#ifdef HAS_XBOX_NETWORK
  // Wait until the net is inited
  DWORD timestamp = GetTickCount() + timeout;

  do
  {
    DWORD dwState = UpdateState();
    
    if (IsEthernetConnected() && (dwState & XNET_GET_XNADDR_DHCP || dwState & XNET_GET_XNADDR_STATIC) && !(dwState & XNET_GET_XNADDR_NONE || dwState & XNET_GET_XNADDR_TROUBLESHOOT || dwState & XNET_GET_XNADDR_PENDING))
      return true;
    
    Sleep(100);
  } while (GetTickCount() < timestamp);

  CLog::Log(LOGDEBUG, "%s - Waiting for network setup failed!", __FUNCTION__);
  return false;
#else
  return true;
#endif
}

/* slightly modified in_ether taken from the etherboot project (http://sourceforge.net/projects/etherboot) */
bool in_ether (char *bufp, unsigned char *addr)
{
  if (strlen(bufp) != 17)
    return false;

  char c, *orig;
  unsigned char *ptr = addr;
  unsigned val;

  int i = 0;
  orig = bufp;

  while ((*bufp != '\0') && (i < 6))
  {
    val = 0;
    c = *bufp++;

    if (isdigit(c))
      val = c - '0';
    else if (c >= 'a' && c <= 'f')
      val = c - 'a' + 10;
    else if (c >= 'A' && c <= 'F')
      val = c - 'A' + 10;
      else
      return false;

    val <<= 4;
    c = *bufp;
    if (isdigit(c))
      val |= c - '0';
    else if (c >= 'a' && c <= 'f')
      val |= c - 'a' + 10;
    else if (c >= 'A' && c <= 'F')
      val |= c - 'A' + 10;
    else if (c == ':' || c == '-' || c == 0)
      val >>= 4;
    else
      return false;

    if (c != 0)
      bufp++;

    *ptr++ = (unsigned char) (val & 0377);
    i++;

    if (*bufp == ':' || *bufp == '-')
      bufp++;
  }

  if (bufp - orig != 17)
    return false;

  return true;
}

CNetwork::CNetwork(void)
{
  memset(&m_networkinfo, 0, sizeof(m_networkinfo));
  m_lastlink = 0;
  m_laststate = 0;
  m_networkup = false;
  m_inited = false;
}

CNetwork::~CNetwork(void)
{
  Deinitialize();
}

void CNetwork::Deinitialize()
{
  if( m_networkup )
    NetworkDown();
  
  m_inited = false;
  WSACleanup();
#ifdef HAS_XBOX_NETWORK
  XNetCleanup();
#endif
}

void CNetwork::LogState()
{
  DWORD dwLink = m_lastlink;
  DWORD dwState = m_laststate;

#ifdef HAS_XBOX_NETWORK
  if ( dwLink & XNET_ETHERNET_LINK_FULL_DUPLEX )
    CLog::Log(LOGINFO, __FUNCTION__" - Link: full duplex");

  if ( dwLink & XNET_ETHERNET_LINK_HALF_DUPLEX )
    CLog::Log(LOGINFO, __FUNCTION__" - Link: half duplex");

  if ( dwLink & XNET_ETHERNET_LINK_100MBPS )
    CLog::Log(LOGINFO, __FUNCTION__" - Link: 100 mbps");

  if ( dwLink & XNET_ETHERNET_LINK_10MBPS )
    CLog::Log(LOGINFO, __FUNCTION__" - Link: 10 mbps");
    
  if ( !(dwLink & XNET_ETHERNET_LINK_ACTIVE) )
    CLog::Log(LOGINFO, __FUNCTION__" - Link: none");

  if ( dwState & XNET_GET_XNADDR_DNS )
    CLog::Log(LOGINFO, __FUNCTION__" - State: dns");

  if ( dwState & XNET_GET_XNADDR_ETHERNET )
    CLog::Log(LOGINFO, __FUNCTION__" - State: ethernet");

  if ( dwState & XNET_GET_XNADDR_NONE )
    CLog::Log(LOGINFO, __FUNCTION__" - State: none");

  if ( dwState & XNET_GET_XNADDR_ONLINE )
    CLog::Log(LOGINFO, __FUNCTION__" - State: online");

  if ( dwState & XNET_GET_XNADDR_PENDING )
    CLog::Log(LOGINFO, __FUNCTION__" - State: pending");

  if ( dwState & XNET_GET_XNADDR_TROUBLESHOOT )
    CLog::Log(LOGINFO, __FUNCTION__" - State: error");

  if ( dwState & XNET_GET_XNADDR_PPPOE )
    CLog::Log(LOGINFO, __FUNCTION__" - State: pppoe");

  if ( dwState & XNET_GET_XNADDR_STATIC )
    CLog::Log(LOGINFO, __FUNCTION__" - State: static");

  if ( dwState & XNET_GET_XNADDR_DHCP )
    CLog::Log(LOGINFO, __FUNCTION__" - State: dhcp");
#endif
  CLog::Log(LOGINFO,  "%s - ip: %s", __FUNCTION__, m_networkinfo.ip);
  CLog::Log(LOGINFO,  "%s - subnet: %s", __FUNCTION__, m_networkinfo.subnet);
  CLog::Log(LOGINFO,  "%s - gateway: %s", __FUNCTION__, m_networkinfo.gateway);
//  CLog::Log(LOGINFO,  __FUNCTION__" - DHCPSERVER: %s", m_networkinfo.dhcpserver);
  CLog::Log(LOGINFO,  "%s - dns: %s, %s", __FUNCTION__, m_networkinfo.DNS1, m_networkinfo.DNS2);
}

bool CNetwork::IsAvailable(bool wait)
{
  // if network isn't up, wait for it to setup
  if( !m_networkup && wait )
    WaitForSetup(WAIT_TIME);

#ifdef HAS_XBOX_NETWORK
  return m_networkup;
#else
  return true;
#endif
}

void CNetwork::NetworkMessage(EMESSAGE message, DWORD dwParam)
{
  switch( message )
  {
    case SERVICES_UP:
    {
      CLog::Log(LOGDEBUG, "%s - Starting network services",__FUNCTION__);
      g_application.StartTimeServer();
      g_application.StartWebServer();
      g_application.StartFtpServer();
      g_application.StartUPnP();
      g_application.StartEventServer();
      CLastfmScrobbler::GetInstance()->Init();
      CLibrefmScrobbler::GetInstance()->Init();
      g_rssManager.Start();
      g_weatherManager.Refresh();
    }
    break;
    case SERVICES_DOWN:
    {
      CLog::Log(LOGDEBUG, "%s - Stopping network services",__FUNCTION__);
      g_application.StopTimeServer();
      g_application.StopWebServer();
      g_application.StopFtpServer();
      g_application.StopUPnP();
      g_application.StopEventServer();
      CLastfmScrobbler::GetInstance()->Term();
      CLibrefmScrobbler::GetInstance()->Term();
      // smb.Deinit(); if any file is open over samba this will break.

      g_rssManager.Stop();
    }
    break;
  }
}

bool CNetwork::WakeOnLan(char* mac)
{
  int i, j, packet;
  unsigned char ethaddr[8];
  unsigned char buf [128];
  unsigned char *ptr;

  // Fetch the hardware address
  if (!in_ether(mac, ethaddr))
  {
    CLog::Log(LOGERROR, "%s - Invalid hardware address specified (%s)", __FUNCTION__, mac);
    return false;
  }

  // Setup the socket
  if ((packet = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
  {
    CLog::Log(LOGERROR, "%s - Unable to create socket (%s)", __FUNCTION__, strerror (errno));
    return false;
  }
 
  // Set socket options
  struct sockaddr_in saddr;
  saddr.sin_family = AF_INET;
  saddr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
  saddr.sin_port = htons(60000);

  unsigned int value = 1;
  if (setsockopt (packet, SOL_SOCKET, SO_BROADCAST, (char*) &value, sizeof( unsigned int ) ) == SOCKET_ERROR)
  {
    CLog::Log(LOGERROR, "%s - Unable to set socket options (%s)", __FUNCTION__, strerror (errno));
    closesocket(packet);
    return false;
  }
 
  // Build the magic packet (6 x 0xff + 16 x MAC address)
  ptr = buf;
  for (i = 0; i < 6; i++)
    *ptr++ = 0xff;

  for (j = 0; j < 16; j++)
    for (i = 0; i < 6; i++)
      *ptr++ = ethaddr[i];
 
  // Send the magic packet
  if (sendto (packet, (char *)buf, 102, 0, (struct sockaddr *)&saddr, sizeof (saddr)) < 0)
  {
    CLog::Log(LOGERROR, "%s - Unable to send magic packet (%s)", __FUNCTION__, strerror (errno));
    closesocket(packet);
    return false;
  }

  closesocket(packet);
  CLog::Log(LOGINFO, "%s - Magic packet send to '%s'", __FUNCTION__, mac);
  return true;
}
