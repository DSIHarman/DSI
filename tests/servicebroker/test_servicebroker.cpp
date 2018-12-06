/*****************************************************************
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* Copyright (c) 2012 Harman International Industries, Inc.
* All rights reserved
****************************************************************/

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "dsi/clientlib.h"

#include <errno.h>     // for ENOTTY test
#include <sys/resource.h>

#include "io.hpp"
#include "../src/servicebroker/clientIo.h"

using namespace DSI;


void sleepMs(unsigned int millis)
{
   ::poll(0, 0, millis);
}


struct TestSocket
{
   TestSocket()
    : connected(false)
   {
      // NOOP
   }

   Unix::Acceptor accpt;
   Unix::StreamSocket sock;
   bool connected;
};


struct _pulse
{
   int32_t code;

   union
   {
      int32_t sival_int;
   } value;
};


uint64_t createReceiverChannel()
{
   TestSocket* ts = new TestSocket();
   char buf[128];
   (void)ts->accpt.bind(make_unix_path(buf, sizeof(buf), getpid(), (uint64_t)ts));
   (void)ts->accpt.listen();

   return (uint64_t)ts;
}


int receivePulse(int fd, void* buf, int buflen)
{
   io::error_code ec = io::unset;

   TestSocket* ts = (TestSocket*)fd;

   do
   {
      if (!ts->connected)
      {
         (void)ts->accpt.accept(ts->sock, 0);
         ts->connected = true;
      }

      ts->sock.read_all(buf, buflen, ec);
      if (ec == io::eof)
      {
         ts->sock.close();
         ts->connected = false;
      }
      else
         break;
   }
   while(true);

   return ec == io::ok ? 0 : 1;
}


int sendPulse(int fd, int code, int value)
{
   int buf[2];
   TestSocket* ts = (TestSocket*)fd;
   if (!ts->connected)
   {
      (void)ts->accpt.accept(ts->sock, 0);
      ts->connected = true;
   }
   io::error_code ec = io::unset;
   buf[0] = code;
   buf[1] = value;
   ts->sock.write_all(buf, sizeof(buf), ec);
   return ec == io::ok ? 0 : 1;
}


void destroyReceiverChannel(int fd)
{
   delete ((TestSocket*)fd);
}


/**
 * Testing servicebroker clientlib interaction.
 * This test needs the following servicebrokers running:
 *
 * servicebroker -vvv -c -f servicebroker.cfg -p /master
 * servicebroker -vvv -c -f servicebroker.cfg -p /slave -m /master
 */
class ServiceBrokerTest : public CppUnit::TestFixture
{
public:
   CPPUNIT_TEST_SUITE(ServiceBrokerTest);
      CPPUNIT_TEST(testRun);
      CPPUNIT_TEST(testGlobalIdTest);
      CPPUNIT_TEST(floodTest);
      CPPUNIT_TEST(multiRegistration);
      CPPUNIT_TEST(testTcpServices);
      CPPUNIT_TEST(testTcpMultiServices);
      CPPUNIT_TEST(testMasterSlaveInteraction);
      CPPUNIT_TEST(testIPRewrite);
      CPPUNIT_TEST(testDisconnectRace);
   CPPUNIT_TEST_SUITE_END();

public:
   void testRun();
   void floodTest();
   void testGlobalIdTest();
   void multiRegistration();
   void testTcpServices();
   void testTcpMultiServices();
   void testInvalidDCmd();
   void testMasterSlaveInteraction();
   void testIPRewrite();
   void testDisconnectRace();

private:
   void waitForCleanServicebroker();
};

CPPUNIT_TEST_SUITE_REGISTRATION(ServiceBrokerTest);

// -------------------------------------------------------------------------

static struct _IfTable
{
   const char* ifName ;
   int majorVerion ;
   int minorVersion ;
   SPartyID serverId ;
} IfTable[] =
{
   { "Interface1", 1, 1 },
   { "Interface2", 1, 2 },
   { "Interface3", 1, 3 },
   { "Interface4", 1, 4 },
   { "Interface5", 1, 5 }
};

#define INTERFACE_AVAILABLE       111
#define CLIENT_DETACH             112
#define SERVER_DISCONNECT         113
#define SERVERLIST_CHANGED        114
#define INTERFACE_AVAILABLE_MATCH 115
#define END_MSGRECEIVE            116

void ServiceBrokerTest::testRun()
{
   int fd;
   int ret;
   struct _pulse pulse ;

   // this is the index of the test interface
   int ifIndex = 2 ;

   fd = SBOpen("/master");
   CPPUNIT_ASSERT(fd > 0);

   int64_t chid = createReceiverChannel();
   CPPUNIT_ASSERT( chid > 0 );

   // we set a notification first
   unsigned int notificationID = 0 ;
   ret = SBSetServerAvailableNotification( fd
                                         , IfTable[ifIndex].ifName, IfTable[ifIndex].majorVerion, IfTable[ifIndex].minorVersion
                                         , chid
                                         , INTERFACE_AVAILABLE
                                         , ifIndex
                                         , &notificationID );
   CPPUNIT_ASSERT(ret == 0);
   CPPUNIT_ASSERT(notificationID > 0);

   unsigned int notificationID_Match = 0 ;
   ret = SBSetInterfaceMatchChangeNotification( fd
                                        , "face5$"
                                        , chid
                                        , INTERFACE_AVAILABLE_MATCH
                                        , 555
                                        , &notificationID_Match );

   for( int i=0; i<(int)(sizeof(IfTable)/sizeof(IfTable[0])); i++ )
   {
      ret = SBRegisterInterface(fd, IfTable[i].ifName, IfTable[i].majorVerion, IfTable[i].minorVersion, chid, &IfTable[i].serverId);
      CPPUNIT_ASSERT(ret == 0);
      CPPUNIT_ASSERT(IfTable[i].serverId.s.localID > 0);
   }

   // we should receive the notification now
   ret = receivePulse( chid, &pulse, sizeof(pulse) );
   CPPUNIT_ASSERT( 0 == ret ); // pulse
   CPPUNIT_ASSERT( pulse.code == INTERFACE_AVAILABLE );
   CPPUNIT_ASSERT( pulse.value.sival_int == ifIndex );

   // we should receive the match notification now
   ret = receivePulse( chid, &pulse, sizeof(pulse) );
   CPPUNIT_ASSERT( 0 == ret ); // pulse
   CPPUNIT_ASSERT( pulse.code == INTERFACE_AVAILABLE_MATCH );
   CPPUNIT_ASSERT( pulse.value.sival_int == 555 );

   // the pulse is ok ... now attach to the interface
   SConnectionInfo connInfo ;
   ret = SBAttachInterface( fd, IfTable[ifIndex].ifName, IfTable[ifIndex].majorVerion, IfTable[ifIndex].minorVersion, &connInfo );
   CPPUNIT_ASSERT( 0 == ret );
   CPPUNIT_ASSERT( connInfo.channel.nid == 0 );
   CPPUNIT_ASSERT( connInfo.channel.pid == getpid() );
   CPPUNIT_ASSERT( connInfo.channel.chid == chid );
   CPPUNIT_ASSERT( connInfo.serverID.globalID == IfTable[ifIndex].serverId.globalID );

   // set the client disconnect notification
   ret = SBSetClientDetachNotification( fd, connInfo.clientID, chid, CLIENT_DETACH, 0xAFFE, &notificationID );
   CPPUNIT_ASSERT( 0 == ret );
   CPPUNIT_ASSERT(notificationID > 0);

   // set the server disconnect notification
   ret = SBSetServerDisconnectNotification( fd, IfTable[ifIndex].serverId, chid, SERVER_DISCONNECT, ifIndex, &notificationID );
   CPPUNIT_ASSERT( 0 == ret );
   CPPUNIT_ASSERT(notificationID > 0);

   // disconnect the client
   ret = SBDetachInterface( fd, connInfo.clientID );
   CPPUNIT_ASSERT( 0 == ret ); // pulse

   // get the notification
   ret = receivePulse( chid, &pulse, sizeof(pulse) );
   CPPUNIT_ASSERT( 0 == ret ); // pulse
   CPPUNIT_ASSERT( pulse.code == CLIENT_DETACH );
   CPPUNIT_ASSERT( pulse.value.sival_int == 0xAFFE );

   // set the server disconnect notification and clear it again
   ret = SBSetServerDisconnectNotification( fd, IfTable[0].serverId, chid, SERVER_DISCONNECT, 0, &notificationID );
   CPPUNIT_ASSERT( 0 == ret );
   CPPUNIT_ASSERT(notificationID > 0);

   ret = SBClearNotification( fd, notificationID );
   CPPUNIT_ASSERT_EQUAL( 0, ret );

   // query interface list
   int count = 0 ;
   static SFNDInterfaceDescription ifs[2048];
   ret = SBGetInterfaceList( fd, ifs, 2048, &count );
   CPPUNIT_ASSERT_EQUAL( 0, ret );
   CPPUNIT_ASSERT_EQUAL( 5, count );
   for( int i=0; i<count; i++ )
   {
      CPPUNIT_ASSERT( 0 == strcmp(IfTable[4-i].ifName, ifs[i].name ));
      CPPUNIT_ASSERT( IfTable[4-i].majorVerion == ifs[i].version.majorVersion );
      CPPUNIT_ASSERT( IfTable[4-i].minorVersion == ifs[i].version.minorVersion );
   }

   ret = SBGetInterfaceList( fd, ifs, 3, &count );
   CPPUNIT_ASSERT( 0 != ret ); // buffer too small

   // now test on slave - it must forward the request
   int slavefd = SBOpen("/slave");
   CPPUNIT_ASSERT(slavefd > 0);

   ret = SBGetInterfaceList( slavefd, ifs, 3, &count );
   CPPUNIT_ASSERT( 0 != ret ); // buffer too small

   ret = SBGetInterfaceList( slavefd, ifs, 5, &count );
   CPPUNIT_ASSERT_EQUAL( 0, ret );
   CPPUNIT_ASSERT_EQUAL( 5, count );

   // find interfaces by regular expression
   memset( ifs, 0, sizeof(ifs));
   ret = SBMatchInterfaceList( fd, "face[24]$", ifs, 10, &count );
   CPPUNIT_ASSERT_EQUAL( 0, ret );
   CPPUNIT_ASSERT_EQUAL( 2, count );

   CPPUNIT_ASSERT( 0 == strcmp(IfTable[3].ifName, ifs[0].name ));
   CPPUNIT_ASSERT( IfTable[3].majorVerion == ifs[0].version.majorVersion );
   CPPUNIT_ASSERT( IfTable[3].minorVersion == ifs[0].version.minorVersion );

   CPPUNIT_ASSERT( 0 == strcmp(IfTable[1].ifName, ifs[1].name ));
   CPPUNIT_ASSERT( IfTable[1].majorVerion == ifs[1].version.majorVersion );
   CPPUNIT_ASSERT( IfTable[1].minorVersion == ifs[1].version.minorVersion );

   ret = SBSetInterfaceListChangeNotification( fd, chid, SERVERLIST_CHANGED, 0xAFFEDEAD, &notificationID );
   CPPUNIT_ASSERT_EQUAL(0, ret);
   CPPUNIT_ASSERT(notificationID > 0);

   // unregister all interfaces
   for( int i=0; i<(int)(sizeof(IfTable)/sizeof(IfTable[0])); i++ )
   {
      ret = SBUnregisterInterface(fd, IfTable[i].serverId);
      CPPUNIT_ASSERT(ret == 0);
      IfTable[i].serverId.globalID = 0 ;

      if( i <= 1 )
      {
         // get the interface list change notification
         ret = receivePulse( chid, &pulse, sizeof(pulse) );
         CPPUNIT_ASSERT( 0 == ret ); // pulse
         CPPUNIT_ASSERT( pulse.code == SERVERLIST_CHANGED );
         CPPUNIT_ASSERT( pulse.value.sival_int == (int)0xAFFEDEAD );

         if( 1 == i )
         {
            // clear interface list change notification
            ret = SBClearNotification( fd, notificationID );
            CPPUNIT_ASSERT( 0 == ret );
         }
      }
   }

   // get the server disconnect notification
   ret = receivePulse( chid, &pulse, sizeof(pulse) );
   CPPUNIT_ASSERT( 0 == ret ); // pulse
   CPPUNIT_ASSERT( pulse.code == SERVER_DISCONNECT );
   CPPUNIT_ASSERT( pulse.value.sival_int == ifIndex );

   ret = SBClearNotification( fd, notificationID_Match );
   CPPUNIT_ASSERT( 0 == ret );

   ret = SBAttachInterface( fd, "UnknownInterface", 1, 0, &connInfo );
   CPPUNIT_ASSERT( 2 == ret );

   destroyReceiverChannel( chid );

   SBClose(slavefd);
   SBClose(fd);
}

void ServiceBrokerTest::testGlobalIdTest()
{
   int ret ;
   notificationid_t notificationID = 0 ;
   struct _pulse pulse ;
   SPartyID slaveID, masterID ;

   int fd_slave = SBOpen("/slave");
   CPPUNIT_ASSERT(fd_slave > 0);

   int fd_master = SBOpen("/master");
   CPPUNIT_ASSERT(fd_master > 0);

   int64_t chid = createReceiverChannel();
   CPPUNIT_ASSERT( chid > 0 );


   ret = SBRegisterInterface(fd_slave, "A_Slave.Interface", 1, 0, chid, &slaveID);
   CPPUNIT_ASSERT(ret == 0);
   CPPUNIT_ASSERT(slaveID.globalID > 0);

   ret = SBRegisterInterface(fd_master, "B_Master.Interface", 1, 0, chid, &masterID);
   CPPUNIT_ASSERT(ret == 0);
   CPPUNIT_ASSERT(masterID.globalID > 0);

   // make sure that both local ID's are the same
   if( slaveID.s.localID > masterID.s.localID )
   {
      while( slaveID.s.localID > masterID.s.localID )
      {
         SBUnregisterInterface( fd_master, masterID );
         sleepMs( 30 );
         ret = SBRegisterInterface(fd_master, "B_Master.Interface", 1, 0, chid, &masterID);
         CPPUNIT_ASSERT(ret == 0);
         CPPUNIT_ASSERT(masterID.globalID > 0);
      }
   }
   else if( slaveID.s.localID < masterID.s.localID )
   {
      while( slaveID.s.localID < masterID.s.localID )
      {
         SBUnregisterInterface( fd_slave, slaveID );
         sleepMs( 30 );
         ret = SBRegisterInterface(fd_slave, "A_Slave.Interface", 1, 0, chid, &slaveID);
         CPPUNIT_ASSERT(ret == 0);
         CPPUNIT_ASSERT(slaveID.globalID > 0);
      }
   }

   // be aware of this race condition here!
   SConnectionInfo connInfo ;
   int loopCount = 0;
   do
   {
      ret = SBAttachInterface( fd_master, "A_Slave.Interface", 1, 0, &connInfo );
      sleepMs(5);
   }
   while(ret == FNDUnknownInterface && loopCount++ < 40);
   CPPUNIT_ASSERT( 0 == ret );

   ret = SBDetachInterface( fd_master, connInfo.clientID );
   CPPUNIT_ASSERT( 0 == ret );

   static const int SLAVE_NOTIFICATION = 1 ;
   static const int MASTER_NOTIFICATION = 2 ;

   ret = SBSetServerDisconnectNotification( fd_master, connInfo.serverID, chid, SERVER_DISCONNECT, SLAVE_NOTIFICATION, &notificationID );
   CPPUNIT_ASSERT(ret == 0);

   ret = SBSetServerDisconnectNotification( fd_master, masterID, chid, SERVER_DISCONNECT, MASTER_NOTIFICATION, &notificationID );
   CPPUNIT_ASSERT(ret == 0);

   // unregister the slave Interface
   SBUnregisterInterface( fd_slave, slaveID );

   // get the server disconnect notification
   ret = receivePulse( chid, &pulse, sizeof(pulse) );
   CPPUNIT_ASSERT( 0 == ret ); // pulse
   CPPUNIT_ASSERT( pulse.code == SERVER_DISCONNECT );
   CPPUNIT_ASSERT( pulse.value.sival_int == SLAVE_NOTIFICATION );

   // unregister the master Interface
   SBUnregisterInterface( fd_master, masterID );

   destroyReceiverChannel( chid );

   SBClose( fd_master );
   SBClose( fd_slave );
}

struct SInterfaceData
{
   const char* name ;
   int major ;
   int minor ;
   SPartyID id ;
};


static SInterfaceData interfaces_slave[] =
{
     { "WLANPresCtrl.DSCPWlanDiag", 1, 0 }
   , { "WLANPresCtrl.DSCPWLAN", 1, 0 }
   , { "SCPMobileOfficeCtrl.DSCPMOCallstack", 0, 1 }
   , { "SCPMobileOfficeCtrl.DSCPMOAddressbook", 0, 1 }
   , { "SCPMobileOfficeCtrl.DSCPMOMessaging", 0, 1 }
   , { "SCPPASSCtrl.DSCPPASS", 0, 1 }
   , { "TelephonyCtrlBlue.DSCPTelephony_NTG5", 0, 1 }
   , { "TelephonyCtrlBlue.DSCPTelephony", 1, 0 }
   , { "BluetoothPresCtrl.DSCPBluetooth", 2, 1 }
   , { "CarfinderCtrl.DSCPCarFinder", 0, 1 }
   , { "SCPServiceCallCtrl.DSCPServiceCallHMI", 0, 1 }
   , { "SCPServiceCallCtrl.DSCPServiceCall", 0, 1 }
   , { "SCPOnOffCtrl.SPSCPOnOff", 0, 1 }
   , { "UserHandlerCtrl.DSCPUserHandler", 0, 1 }
   , { "Bluephone.SGPhone", 6, 3 }
   , { "Local.LocalService", 1, 0 }
   , { "Slave.LocalService", 1, 0 }
   , { "NWS_PF.SGPf", 1, 0 }
   , { "NWS_IP.SGIp", 1, 3 }
   , { "NWS_IEEE80211,SGIeee80211", 1, 0 }
   , { "Bluephone.SGPhoneDiag", 2, 0 }
   , { "NWS_ETH.SGEth", 1, 0 }
   , { "BSS_GAP.SGBluetoothGap", 10, 0 }
   , { "NWS.SGNWS", 1, 0 }
   , { "NWS_DHCP.SGDhcp", 1, 0 }
   , { "Callstacks.SGCallstacks", 1, 0 }
   , { "Notes.SGNotes", 1, 0 }
   , { "Tasks.SGTasks", 1, 0 }
   , { "Contacts.SGContacts", 1, 0 }
   , { "Calendar.SGCalendar", 1, 1 }
   , { "BSS_DI.SGBluetoothDid", 1, 0 }
   , { "BSS_DIAG.SGBluetoothDiag", 2, 0 }
   , { "ProtocolGatewayCan.DSYSProtocolGateway", 0, 3 }
   , { "signalGateway.DSYSignalGateway", 0, 1 }
   , { "StartupShutdown.DSYSStartupShutdown", 0, 2 }
   , { "ElecWire.DSYS_ElecWire", 1, 1 }
   , { "UserControl.DSYSUserControl", 0, 1 }
   , { "Amp.DSYSAmp", 0, 1 }
   , { "LevelMonitoringVoltage.DSYSLevelMonitoringVoltage", 0, 1 }
   , { "LevelMonitoring.DSYSLevelMonitor", 1, 1 }
   , { "Persistency.DSYSPersistency", 2, 0 }
   , { "Persistency.DSYSPersistencyAdmin", 2, 0 }
   , { "Persistency.DSYSPersistencyFsc", 1, 0 }
   , { "Persistency.DSYSOnOff", 1, 1 }
   , { "CANSubSystem.DSYSCANStatus", 1, 0 }
   , { "CANSubSystem.DSYSErrorMemoryAppClient", 1, 0 }
   , { "CANSubSystem.DSYSCANSignal", 0, 3 }
   , { "CANSubSystem.DSYSCAN_TP", 3, 0 }
   , { "ConsoleController.DSYSConsoleStatus", 1, 0 }
   , { "PowerStateController.DSYSOnOffLifecycle", 1, 0 }
   , { "PowerStateController.DSYSNTG5LifecycleInfo", 1, 0 }
   , { "MOSTPower.DSYSOnOff", 1, 1 }
   , { "MOSTSubSystem.DSYSMOSTRouting", 1, 0 }
   , { "MOSTSubSystem.DSYSMOSTServer", 2, 1 }
   , { "MOSTSubSystem.DSYSNetworkManagement", 1, 0 }
   , { "AudioManagement.DSYSAudioSetting", 1, 0 }
   , { "AudioManagement.DSYSAudioManagement", 1, 0 }
   , { "AMFMTunerAudioSource.DSYSAudioSource", 1, 1 }
   , { "AudioRouter.DSYSRouting", 0, 2 }
   , { "AMFMTunerDSIDevice.DMMTunerAnnouncement", 0, 1 }
   , { "AMFMTunerDSIDevice.DMMTunerStation", 0, 1 }
   , { "POISearchEngine.DNAVIPOISearch", 1, 0 }
   , { "PosMap.DNAVIPosMap", 1, 0 }
   , { "RouteGuidance.DNAVIRouteGuidance", 1, 0 }
   , { "ManeuverListCalculator.DNAVIManeuverListCalculator", 1, 0 }
   , { "PosDemo.DNAVIPosDemo", 1, 0 }
   , { "Aroma.DNAVIPosRouteMatched", 1, 0 }
   , { "TourManager.DNAVITourManager", 1, 0 }
   , { "TourManager.DNAVIFileProviderControl", 1, 0 }
   , { "LocationInputEngine.DNAVILocationInput", 1, 0 }
   , { "DataAccessHub.DNAVIDatabaseConfiguration", 1, 0 }
   , { "DataAccessHub.DNAVIDataAccessHub", 1, 0 }
   , { "OnOffEngine.DNAVIOnOff", 1, 0 }
   , { "OnOffEngine.DNAVIOnOffSubSystem", 1, 0 }
   , { "AMFMTunerDSIDevice.DMMTunerTMC", 1, 0 }
   , { "AMFMTunerDSIDevice.DMMAmFmTunerControl", 0, 4 }
   , { "AMFMTunerDSIDevice.DMMAmFmTunerDiagnosis", 0, 2 }
   , { "FlashGuiService_Driver.HBFlashGUIService", 0, 1 }
   , { "mme.DSYSOnOff", 1, 1 }
   , { "mme.DSYSDmsAdministration", 2, 0 }
   , { "mme.DSYSDmsSql", 3, 1 }
   , { "PosReal.DNAVIPosReal", 1, 0 }
   , { "PosReal.DNAVIPos", 1, 0 }
   , { "OnOffPositioning.DNAVIOnOff", 1, 0 }
   , { "OnOffPositioning.DNAVIOnOffSubSystem", 1, 0 }
   , { "OnOffNavigation.DSYSOnOff", 1, 1 }
   , { "OnOffNavigation.DNAVIOnOff", 1, 0 }
   , { "KeyInputPresCtrl.DCARNTG5KeyInput", 0, 1 }
   , { "Climate.DCARNTG5Climate", 0, 1 }
   , { "RSEControl.DCARNTG5RSEControl", 0, 1 }
   , { "KleerData.DCARNTG5Kleer", 0, 1 }
   , { "System.DCARNTG5System", 1, 0 }
   , { "RVCamera.DCARNTG5RVCamera", 0, 1 }
   , { "UserData.DCARNTG5UserData", 1, 3 }
   , { "Vehicle.DCARNTG5VehicleFunctions", 1, 0 }
   , { "CtrlC.DCARNTG5HUnitSystemInfo", 1, 0 }
   , { "CtrlC.DCARNTG5HapticController", 1, 0 }
   , { "CtrlC.DCARNTG5CtrlCInput", 1, 1 }
   , { "ICExtKIProtocol.DCARNTG5ICExtKIProtocol", 1, 0 }
   , { "ScopeHandler.DSYSNTG5ScopeHandler", 0, 1 }
   , { "LogManager.DSYSNTG5PathoLogManager", 1, 3 }
};

static SInterfaceData interfaces_master[] =
{
     { "NavCtrl_CoDriver.DNAVNTG5Audio", 1, 0 }
   , { "NavCtrl_CoDriver.DNAVNTG5Ti", 1, 0 }
   , { "NavCtrl_CoDriver.DNAVNTG5SpeedLimit", 1, 0 }
   , { "NavCtrl_CoDriver.DNAVNTG5FreeTextInput", 0, 1 }
   , { "NavCtrl_CoDriver.DNAVNTG5POISelection", 3, 0 }
   , { "NavCtrl_CoDriver.DNAVNTG5Pos", 1, 0 }
   , { "NavCtrl_CoDriver.DNAVNTG5LocationInput", 1, 1 }
   , { "NavCtrl_CoDriver.DNAVNTG5RG", 1, 0 }
   , { "NavCtrl_CoDriver.DNAVNTG5Map", 1, 1 }
   , { "NavCtrl_CoDriver.DNAVNTG5Main", 1, 0 }
   , { "NavCtrl_Driver.DNAVNTG5Audio", 1, 0 }
   , { "NavCtrl_Driver.DNAVNTG5Ti", 1, 0 }
   , { "NavCtrl_Driver.DNAVNTG5SpeedLimit", 1, 0 }
   , { "NavCtrl_Driver.DNAVNTG5FreeTextInput", 0, 1 }
   , { "NavCtrl_Driver.DNAVNTG5POISelection", 3, 0 }
   , { "NavCtrl_Driver.DNAVNTG5Pos", 1, 0 }
   , { "NavCtrl_Driver.DNAVNTG5LocationInput", 1, 1 }
   , { "NavCtrl_Driver.DNAVNTG5RG", 1, 0 }
   , { "NavCtrl_Driver.DNAVNTG5Map", 1, 1 }
   , { "NavCtrl_Driver.DNAVNTG5Main", 1, 0 }
   , { "Bluephone2.SGPhone", 6, 3 }
   , { "Local.LocalService", 1, 0 }
   , { "Master.LocalService", 1, 0 }
   , { "ZonePlayerPassenger.DMMNTG5ZonePlayer", 0, 1 }
   , { "ZonePlayerDriver.DMMNTG5ZonePlayer", 0, 1 }
   , { "MMAudioSource.DSYSAudioSource", 1, 1 }
   , { "DeviceManager.DMMNTG5DeviceManager", 0, 1 }
   , { "SoundPresCtrl.DAVTSound", 0, 4 }
   , { "MutePresCtrl.DAVTMute", 0, 1 }
   , { "PAuxInPresCtrl.DAVTAuxIn", 0, 2 }
   , { "TVTunerPresCtrl.DAVTTunerTV", 0, 3 }
   , { "TunerPresCtrl.DAVTAppTunerAM", 0, 3 }
   , { "TunerPresCtrl.DAVTGlobalLists", 0, 3 }
   , { "TunerPresCtrl.DAVTAppTunerFM", 0, 3 }
   , { "Entertainment.DAVTEntertainment", 0, 2 }
   , { "HDDMgr.DSYSOnOff", 1, 1 }
   , { "HDDMgrDiag.HDDMgrDiag", 1, 0 }
   , { "HDDMgrDebug.HDDMgrDebug", 1, 1 }
   , { "HDDMgr.HDDMgr", 1, 0 }
   , { "MOSTSubSystemIntel.DSYSMOSTServer", 2, 1 }
   , { "KeyboardService.HBKeyboardService", 2, 0 }
};

static bool find( const SFNDInterfaceDescription* ifDecr, int size, const char* ifName )
{
   for( int idx=0; idx<size; idx++ )
   {
      if( 0 == strcmp( ifDecr[idx].name, ifName ))
      {
         return true ;
      }
   }
   return false ;
}


void dump(SFNDInterfaceDescription* ifs, size_t count)
{
   std::cout << "-------------------------------------" << std::endl;
   for (size_t i=0; i<count; ++i)
   {
      std::cout << "'" << ifs[i].name << "'" << std::endl;
   }
   std::cout << "-------------------------------------" << std::endl;
}


void ServiceBrokerTest::floodTest()
{
   int ret ;
   notificationid_t notificationID = 0 ;
   SConnectionInfo connInfo ;

   int fd_slave = SBOpen("/slave");
   CPPUNIT_ASSERT(fd_slave > 0);

   int fd_master = SBOpen("/master");
   CPPUNIT_ASSERT(fd_master > 0);

   int64_t chid = createReceiverChannel();
   CPPUNIT_ASSERT( chid > 0 );

   for( unsigned int idx=0; idx<sizeof(interfaces_slave)/sizeof(interfaces_slave[0]); idx++ )
   {
      ret = SBRegisterInterface( fd_slave, interfaces_slave[idx].name, interfaces_slave[idx].major, interfaces_slave[idx].minor, chid, &interfaces_slave[idx].id );
      CPPUNIT_ASSERT_EQUAL( 0, ret );
   }

   for( unsigned int idx=0; idx<sizeof(interfaces_master)/sizeof(interfaces_master[0]); idx++ )
   {
      ret = SBRegisterInterface( fd_master, interfaces_master[idx].name, interfaces_master[idx].major, interfaces_master[idx].minor, chid, &interfaces_master[idx].id );
      CPPUNIT_ASSERT_EQUAL( 0, ret );
   }


   int count_slave = 0 ;
   int count_master = 0 ;
   static SFNDInterfaceDescription ifs_slave[150];
   static SFNDInterfaceDescription ifs_master[150];

   ret = SBGetInterfaceList( fd_slave, ifs_slave, sizeof(ifs_slave)/sizeof(ifs_slave[0]), &count_slave );
   CPPUNIT_ASSERT_EQUAL( 0, ret );
   CPPUNIT_ASSERT_EQUAL( 138, count_slave );

   ret = SBGetInterfaceList( fd_master, ifs_master, sizeof(ifs_master)/sizeof(ifs_master[0]), &count_master );
   CPPUNIT_ASSERT_EQUAL( 0, ret );
   CPPUNIT_ASSERT_EQUAL( 136, count_master );

   //dump(ifs_slave, count_slave);
   //dump(ifs_master, count_master);

   // check local slave interface
   CPPUNIT_ASSERT( find( ifs_slave, count_slave, "AudioRouter.DSYSRouting") && !find( ifs_master, count_master, "AudioRouter.DSYSRouting") );
   CPPUNIT_ASSERT( find( ifs_slave, count_slave, "AMFMTunerDSIDevice.DMMTunerAnnouncement") && !find( ifs_master, count_master, "AMFMTunerDSIDevice.DMMTunerAnnouncement") );
   CPPUNIT_ASSERT( find( ifs_slave, count_slave, "AMFMTunerDSIDevice.DMMTunerStation") && !find( ifs_master, count_master, "AMFMTunerDSIDevice.DMMTunerStation") );
   CPPUNIT_ASSERT( find( ifs_slave, count_slave, "WLANPresCtrl.DSCPWlanDiag") && !find( ifs_master, count_master, "WLANPresCtrl.DSCPWlanDiag") );
   CPPUNIT_ASSERT( find( ifs_slave, count_slave, "Slave.LocalService") && !find( ifs_master, count_master, "Slave.LocalService") );

   // check local master interface
   CPPUNIT_ASSERT( !find( ifs_slave, count_slave, "ZonePlayerDriver.DMMNTG5ZonePlayer") && find( ifs_master, count_master, "ZonePlayerDriver.DMMNTG5ZonePlayer") );
   CPPUNIT_ASSERT( !find( ifs_slave, count_slave, "DeviceManager.DMMNTG5DeviceManager") && find( ifs_master, count_master, "DeviceManager.DMMNTG5DeviceManager") );
   CPPUNIT_ASSERT( !find( ifs_slave, count_slave, "Master.LocalService") && find( ifs_master, count_master, "Master.LocalService") );

   // check Local.LocalService
   CPPUNIT_ASSERT( find( ifs_slave, count_slave, "Local.LocalService") && find( ifs_master, count_master, "Local.LocalService") );

   // connecting to different services
   ret = SBAttachInterface( fd_slave, "Local.LocalService", 1, 0, &connInfo );
   CPPUNIT_ASSERT_EQUAL( 0, ret );

   ret = SBDetachInterface( fd_slave, connInfo.clientID );
   CPPUNIT_ASSERT_EQUAL( 0, ret );

   ret = SBAttachInterface( fd_master, "Local.LocalService", 1, 0, &connInfo );
   CPPUNIT_ASSERT_EQUAL( 0, ret );

   ret = SBDetachInterface( fd_master, connInfo.clientID );
   CPPUNIT_ASSERT_EQUAL( 0, ret );


   ret = SBAttachInterface( fd_master, "Slave.LocalService", 1, 0, &connInfo );
   CPPUNIT_ASSERT_EQUAL( 2, ret );

   ret = SBAttachInterface( fd_slave, "Slave.LocalService", 1, 0, &connInfo );
   CPPUNIT_ASSERT_EQUAL( 0, ret );

   ret = SBDetachInterface( fd_slave, connInfo.clientID );
   CPPUNIT_ASSERT_EQUAL( 0, ret );

   ret = SBAttachInterface( fd_slave, "Master.LocalService", 1, 0, &connInfo );
   CPPUNIT_ASSERT_EQUAL( 2, ret );

   ret = SBAttachInterface( fd_master, "Master.LocalService", 1, 0, &connInfo );
   CPPUNIT_ASSERT_EQUAL( 0, ret );

   ret = SBDetachInterface( fd_master, connInfo.clientID );
   CPPUNIT_ASSERT_EQUAL( 0, ret );

   ret = SBAttachInterface( fd_slave, "NavCtrl_CoDriver.DNAVNTG5RG", 1, 0, &connInfo );
   CPPUNIT_ASSERT_EQUAL( 0, ret );

   ret = SBSetServerDisconnectNotification( fd_slave, connInfo.serverID, chid, 0, 0, &notificationID );
   CPPUNIT_ASSERT_EQUAL( 0, ret );

   ret = SBClearNotification( fd_slave, notificationID );
   CPPUNIT_ASSERT( 0 == ret );

   ret = SBSetClientDetachNotification( fd_slave, connInfo.clientID, chid, 0, 0, &notificationID );
   CPPUNIT_ASSERT_EQUAL( 0, ret );

   ret = SBClearNotification( fd_slave, notificationID );
   CPPUNIT_ASSERT( 0 == ret );

   ret = SBDetachInterface( fd_slave, connInfo.clientID );
   CPPUNIT_ASSERT_EQUAL( 0, ret );

   int count = 0;
   memset( ifs_slave, 0, sizeof(ifs_slave));
   ret = SBMatchInterfaceList( fd_slave, "\\.SGPhone$", ifs_slave, sizeof(ifs_slave)/sizeof(ifs_slave[0]), &count );
   CPPUNIT_ASSERT_EQUAL( 0, ret );
   CPPUNIT_ASSERT_EQUAL( 2, count );
   CPPUNIT_ASSERT_EQUAL( 0, strcmp( ifs_slave[0].name, "Bluephone2.SGPhone" ) );
   CPPUNIT_ASSERT_EQUAL( 0, strcmp( ifs_slave[1].name, "Bluephone.SGPhone" ) );

   for( unsigned int idx=0; idx<sizeof(interfaces_master)/sizeof(interfaces_master[0]); idx++ )
   {
      ret = SBUnregisterInterface( fd_master, interfaces_master[idx].id );
      CPPUNIT_ASSERT( 0 == ret );
   }

   for( unsigned int idx=0; idx<sizeof(interfaces_slave)/sizeof(interfaces_slave[0]); idx++ )
   {
      ret = SBUnregisterInterface( fd_slave, interfaces_slave[idx].id );
      CPPUNIT_ASSERT( 0 == ret );
   }

   ret = SBRegisterInterface( fd_slave, interfaces_slave[10].name, interfaces_slave[10].major, interfaces_slave[10].minor, chid, &interfaces_slave[10].id );
   CPPUNIT_ASSERT_EQUAL( 0, ret );
   ret = SBUnregisterInterface( fd_slave, interfaces_slave[10].id );
   CPPUNIT_ASSERT( 0 == ret );

   destroyReceiverChannel( chid );

   SBClose( fd_master );
   SBClose( fd_slave );
}

void ServiceBrokerTest::multiRegistration()
{
   SConnectionInfo connInfo ;

   int fd = SBOpen("/master");
   CPPUNIT_ASSERT(fd > 0);

   struct InputData
   {
      char name[255];
      int major;
      int minor;
   };

   InputData data[] =
   {
      { "NavCtrl_Driver.DNAVNTG5POISelection0", 1, 0 },
      { "NavCtrl_Driver.DNAVNTG5POISelection1", 2, 0 },
      { "NavCtrl_Driver.DNAVNTG5POISelection2", 3, 0 },
      { "NavCtrl_Driver.DNAVNTG5POISelection3", 4, 0 },
      { "NavCtrl_Driver.DNAVNTG5POISelection4", 3, 0 },
      { "NavCtrl_Driver.DNAVNTG5POISelection5", 4, 0 },
      { "NavCtrl_Driver.DNAVNTG5POISelection6", 5, 0 },
      { "NavCtrl_Driver.DNAVNTG5POISelection7", 6, 0 },
      { "NavCtrl_Driver.DNAVNTG5POISelection8", 7, 0 },
      { "NavCtrl_Driver.DNAVNTG5POISelection8", 7, 0 }
   };

   SPartyID ids[11];
   SFNDInterfaceDescription ifd[11];
   for( int i=0; i<10; i++ )
   {
      strcpy( ifd[i].name, data[i].name );
      ifd[i].version.majorVersion = data[i].major ;
      ifd[i].version.minorVersion = data[i].minor ;
   }

   int ret = SBRegisterInterfaceEx( fd, ifd, 10, 0, ids );
   CPPUNIT_ASSERT(0 == ret);

   /* duplicate interface */
   CPPUNIT_ASSERT( (unsigned long long)-1 == ids[9].globalID );

   for( int i=0; i<8; i++ )
   {
      ret = SBAttachInterface( fd, data[i].name, data[i].major, data[i].minor, &connInfo );
      CPPUNIT_ASSERT_EQUAL( 0, ret );

      CPPUNIT_ASSERT_EQUAL( connInfo.serverID.globalID, ids[i].globalID );

      ret = SBDetachInterface( fd, connInfo.clientID );
      CPPUNIT_ASSERT_EQUAL( 0, ret );
   }

   for( int i=0; i<8; i++ )
   {
      SBUnregisterInterface( fd, ids[i] );
   }

   SBClose( fd );
}


void ServiceBrokerTest::testTcpServices()
{
   int fd = SBOpen("/master");
   CPPUNIT_ASSERT(fd > 0);

   int64_t chid = createReceiverChannel();
   CPPUNIT_ASSERT( chid > 0 );

   unsigned int notificationID = 0 ;
   int ret = SBSetServerAvailableNotification( fd, "HalloWelt.hallowelt", 1, 2, chid, INTERFACE_AVAILABLE, 42, &notificationID );
   CPPUNIT_ASSERT(ret == 0);
   CPPUNIT_ASSERT(notificationID > 0);

   SPartyID pid1;
   pid1.globalID = 0;
   ret = SBRegisterInterface( fd, "HalloWelt.hallowelt", 1, 2, chid, &pid1 );
   CPPUNIT_ASSERT( 0 == ret );
   CPPUNIT_ASSERT(pid1.s.localID > 0);

   SPartyID pid2;
   pid2.globalID = 0;
   ret = SBRegisterInterfaceTCP( fd, "HalloWelt.hallowelt", 1, 2, htonl(INADDR_LOOPBACK), htons(9999), &pid2 );
   CPPUNIT_ASSERT( 0 == ret );
   CPPUNIT_ASSERT(pid2.s.localID > 0);

   struct _pulse pulse;
   ret = receivePulse( chid, &pulse, sizeof(pulse) );
   CPPUNIT_ASSERT( 0 == ret ); // pulse
   CPPUNIT_ASSERT( pulse.code == INTERFACE_AVAILABLE );
   CPPUNIT_ASSERT( pulse.value.sival_int == 42 );

   struct STCPConnectionInfo connInfo;
   ret = SBAttachInterfaceTCP(fd, "HalloWelt.hallowelt", 1, 2, &connInfo);
   CPPUNIT_ASSERT( 0 == ret);
   CPPUNIT_ASSERT(connInfo.ifVersion.majorVersion == 1);
   CPPUNIT_ASSERT(connInfo.ifVersion.minorVersion == 2);
   CPPUNIT_ASSERT(connInfo.socket.ipaddress == htonl(INADDR_LOOPBACK));
   CPPUNIT_ASSERT(connInfo.socket.port == htons(9999));
   CPPUNIT_ASSERT(connInfo.serverID.globalID = pid2);

   destroyReceiverChannel(chid);
   SBClose(fd);
}


void ServiceBrokerTest::testTcpMultiServices()
{
   int fd = SBOpen("/master");
   CPPUNIT_ASSERT(fd > 0);

   int64_t chid = createReceiverChannel();
   CPPUNIT_ASSERT( chid > 0 );

   unsigned int notificationID = 0 ;
   int ret = SBSetServerAvailableNotification( fd, "HalloWelt.hallowelt", 1, 2, chid, INTERFACE_AVAILABLE, 42, &notificationID );
   CPPUNIT_ASSERT(ret == 0);
   CPPUNIT_ASSERT(notificationID > 0);

   SPartyID ids1[2];
   SFNDInterfaceDescription ifd[2];
   strcpy( ifd[0].name, "HalloWelt2.hallowelt2" );
   ifd[0].version.majorVersion = 2 ;
   ifd[0].version.minorVersion = 3 ;
   strcpy( ifd[1].name, "HalloWelt.hallowelt" );
   ifd[1].version.majorVersion = 1 ;
   ifd[1].version.minorVersion = 2 ;

   ids1[0].globalID = 0;
   ids1[1].globalID = 0;
   ret = SBRegisterInterfaceEx( fd, ifd, 2, chid, ids1 );
   CPPUNIT_ASSERT( 0 == ret );
   CPPUNIT_ASSERT(ids1[0].s.localID > 0);
   CPPUNIT_ASSERT(ids1[1].s.localID > 0);

   SPartyID ids2[2];
   ids2[0].globalID = 0;
   ids2[1].globalID = 0;
   ret = SBRegisterInterfaceExTCP( fd, ifd, 2, htonl(INADDR_LOOPBACK), htons(9999), ids2 );
   CPPUNIT_ASSERT( 0 == ret );
   CPPUNIT_ASSERT(ids2[1].s.localID > 0);
   CPPUNIT_ASSERT(ids2[1].s.localID > 0);

   struct _pulse pulse;
   ret = receivePulse( chid, &pulse, sizeof(pulse) );
   CPPUNIT_ASSERT( 0 == ret ); // pulse
   CPPUNIT_ASSERT( pulse.code == INTERFACE_AVAILABLE );
   CPPUNIT_ASSERT( pulse.value.sival_int == 42 );

   struct STCPConnectionInfo connInfo;
   ret = SBAttachInterfaceTCP(fd, "HalloWelt.hallowelt", 1, 2, &connInfo);
   CPPUNIT_ASSERT( 0 == ret);
   CPPUNIT_ASSERT(connInfo.ifVersion.majorVersion == 1);
   CPPUNIT_ASSERT(connInfo.ifVersion.minorVersion == 2);
   CPPUNIT_ASSERT(connInfo.socket.ipaddress == htonl(INADDR_LOOPBACK));
   CPPUNIT_ASSERT(connInfo.socket.port == htons(9999));
   CPPUNIT_ASSERT(connInfo.serverID.globalID = ids2[1]);

   destroyReceiverChannel(chid);
   SBClose(fd);
}


void ServiceBrokerTest::testMasterSlaveInteraction()
{
   int ret ;
   notificationid_t notificationID = 0 ;

   int fd_slave = SBOpen("/slave");
   CPPUNIT_ASSERT(fd_slave > 0);

   int fd_master = SBOpen("/master");
   CPPUNIT_ASSERT(fd_master > 0);

   int64_t chid = createReceiverChannel();
   CPPUNIT_ASSERT( chid > 0 );

   ret = SBSetServerAvailableNotification( fd_slave, "HalloWelt.hallowelt", 1, 2, chid, INTERFACE_AVAILABLE, 42, &notificationID );
   CPPUNIT_ASSERT(ret == 0);
   CPPUNIT_ASSERT(notificationID > 0);

   SPartyID pid1;
   pid1.globalID = 0;
   ret = SBRegisterInterface( fd_master, "HalloWelt.hallowelt", 1, 2, chid, &pid1 );
   CPPUNIT_ASSERT( 0 == ret );
   CPPUNIT_ASSERT(pid1.s.localID > 0);

   struct _pulse pulse;
   ret = receivePulse( chid, &pulse, sizeof(pulse) );

   CPPUNIT_ASSERT( 0 == ret ); // pulse
   CPPUNIT_ASSERT( pulse.code == INTERFACE_AVAILABLE );
   CPPUNIT_ASSERT( pulse.value.sival_int == 42 );

   SConnectionInfo connInfo;
   ret = SBAttachInterface( fd_slave, "HalloWelt.hallowelt", 1, 2, &connInfo );
   CPPUNIT_ASSERT_EQUAL( 0, ret );
   CPPUNIT_ASSERT_EQUAL( connInfo.serverID.globalID, pid1.globalID );

   destroyReceiverChannel(chid);

   SBClose(fd_slave);
   SBClose(fd_master);
}


void ServiceBrokerTest::testIPRewrite()
{

   const char* ip = getenv("SB_EXPECTED_IP");

   if (ip == 0)
   {
      std::cout
         << std::endl
         << "Note: this test is only fully functional if the servicebrokers are" << std::endl
         << "      also connected via a non-loopback device and the environment" << std::endl
         << "      variable SB_EXPECTED_IP is set appropriately." << std::endl;

      ip = "127.0.0.1";   // set this as default (as done by DSI applications)
   }

   IPv4::Endpoint localhost("127.0.0.1", 7777);
   IPv4::Endpoint registration(localhost);

   IPv4::Endpoint exptected(ip, 7777);
   int ret ;

   int fd_slave = SBOpen("/slave");
   CPPUNIT_ASSERT(fd_slave > 0);

   int fd_master = SBOpen("/master");
   CPPUNIT_ASSERT(fd_master > 0);

   SPartyID partyid;
   partyid.globalID = 0;

   SPartyID partyid_tcp;
   partyid_tcp.globalID = 0;

   // register interface at master - connect from slave
   ret = SBRegisterInterface(fd_master, "HalloWelt.hallowelt", 1, 2, 888, &partyid);
   CPPUNIT_ASSERT( 0 == ret );
   CPPUNIT_ASSERT(partyid.s.localID > 0);

   ret = SBRegisterInterfaceTCP(fd_master, "HalloWelt.hallowelt", 1, 2, registration.getIP(), registration.getPort(), &partyid_tcp);
   CPPUNIT_ASSERT( 0 == ret );
   CPPUNIT_ASSERT(partyid_tcp.s.localID > 0);

   STCPConnectionInfo connInfo;
   ret = SBAttachInterfaceTCP(fd_slave, "HalloWelt.hallowelt", 1, 2, &connInfo);
   CPPUNIT_ASSERT_EQUAL( 0, ret );
   CPPUNIT_ASSERT_EQUAL( connInfo.serverID.globalID, partyid_tcp.globalID );

   // retrieved actual tranlated IP
   std::cout
      << "expected <-> actual: " << std::hex << exptected.getIP()
      << " <-> " << std::hex << connInfo.socket.ipaddress
      << std::endl;

   CPPUNIT_ASSERT_EQUAL(static_cast<unsigned int>(exptected.getIP()), connInfo.socket.ipaddress);
   CPPUNIT_ASSERT_EQUAL((uint16_t)exptected.getPort(), connInfo.socket.port);

   // cleanup
   (void)SBUnregisterInterface(fd_master, partyid);
   (void)SBUnregisterInterface(fd_master, partyid_tcp);
   partyid.globalID = 0;
   partyid_tcp.globalID = 0;

   // register interface at slave - connect from master
   ret = SBRegisterInterface(fd_slave, "HalloWelt.hallowelt", 1, 2, 888, &partyid);
   CPPUNIT_ASSERT( 0 == ret );
   CPPUNIT_ASSERT(partyid.s.localID > 0);

   ret = SBRegisterInterfaceTCP(fd_slave, "HalloWelt.hallowelt", 1, 2, registration.getIP(), registration.getPort(), &partyid_tcp);
   CPPUNIT_ASSERT( 0 == ret );
   CPPUNIT_ASSERT(partyid_tcp.s.localID > 0);

   int loopCount = 0;
   do
   {
      ret = SBAttachInterfaceTCP( fd_master, "HalloWelt.hallowelt", 1, 2, &connInfo );
   }
   while(ret == FNDUnknownInterface && loopCount++ < 40);
   CPPUNIT_ASSERT( 0 == ret );

   // retrieved actual tranlated IP
   std::cout
      << "expected <-> actual: " << std::hex << exptected.getIP()
      << " <-> " << std::hex << connInfo.socket.ipaddress
      << std::endl;

   CPPUNIT_ASSERT_EQUAL(static_cast<unsigned int>(exptected.getIP()), connInfo.socket.ipaddress);
   CPPUNIT_ASSERT_EQUAL((uint16_t)exptected.getPort(), connInfo.socket.port);

   // final cleanup
   SBClose(fd_slave);
   SBClose(fd_master);
}


void ServiceBrokerTest::waitForCleanServicebroker()
{
   //wait until the servicebrokers are clean from previous test
   const unsigned int RETRIES(40);
   int loopCount(0);
   int ret(0);
   int ifsCnt(0);
   struct SFNDInterfaceDescription ifs[1];
   int fd_server = SBOpen("/master");
   CPPUNIT_ASSERT(fd_server > 0);

   loopCount = 0;
   do
   {
      ret = SBMatchInterfaceList( fd_server, ".*", ifs, 1, &ifsCnt );
      usleep(200000);//300 ms
   } while (loopCount++ < RETRIES && (ret != 0 || ifsCnt > 0));
   (void)SBClose(fd_server);
}


void ServiceBrokerTest::testDisconnectRace()
{
   // 3x testen: server und client auf gleichem oder unterschiedlichem servicebroker
   const char* server_sb[] = { "/master", "/master", "/slave" };
   const char* client_sb[] = { "/master", "/slave", "/master" };

   waitForCleanServicebroker();

   for (int i=0; i<3; ++i)
   {
      for (int j=0; j<2; ++j)
      {
         // open server OCB
         int fd_server = SBOpen(server_sb[i]);
         CPPUNIT_ASSERT(fd_server > 0);

         // open client OCB
         int fd_client = SBOpen(client_sb[i]);
         CPPUNIT_ASSERT(fd_client > 0);

         // open notification channel
         int64_t chid = createReceiverChannel();
         CPPUNIT_ASSERT( chid > 0 );

         // server register Interface
         SPartyID partyid;
         partyid.globalID = 0;

         int ret = SBRegisterInterface(fd_server, "HalloWelt.hallowelt", 1, 2, 888, &partyid);
         CPPUNIT_ASSERT( 0 == ret );
         CPPUNIT_ASSERT(partyid.s.localID > 0);

         // client attach interface
         SConnectionInfo connInfo ;
         ret = SBAttachInterface(fd_client, "HalloWelt.hallowelt", 1, 2, &connInfo);
         CPPUNIT_ASSERT_EQUAL(0, ret);

         unsigned int notificationID = 0;

         // 1.
         // server unregister interface
         // client server disconnect notification setzen

         // 2.
         // client server disconnect notification setzen
         // server unregister interface

         if (j == 0)
         {
            ret = SBSetServerDisconnectNotification(fd_client, partyid, chid, SERVER_DISCONNECT, 4711, &notificationID);
            CPPUNIT_ASSERT_EQUAL(0, ret);
            CPPUNIT_ASSERT(notificationID > 0);

            ret = SBUnregisterInterface(fd_server, partyid);
            CPPUNIT_ASSERT_EQUAL(0, ret);
         }
         else
         {
            ret = SBUnregisterInterface(fd_server, partyid);
            CPPUNIT_ASSERT_EQUAL(0, ret);

            ret = SBSetServerDisconnectNotification(fd_client, partyid, chid, SERVER_DISCONNECT, 4711, &notificationID);
            CPPUNIT_ASSERT_EQUAL(0, ret);
            CPPUNIT_ASSERT(notificationID > 0);
         }

         // client muss jetzt pulse kriegen
         struct _pulse pulse;
         ret = receivePulse( chid, &pulse, sizeof(pulse) );
         CPPUNIT_ASSERT_EQUAL(0, ret); // pulse
         CPPUNIT_ASSERT(pulse.code == SERVER_DISCONNECT);

         // cleanup
         ret = SBClearNotification(fd_client, notificationID);
         CPPUNIT_ASSERT(ret == 0 || ret == FNDInvalidNotificationID);   // will be deleted after beeing sent

         ret = SBDetachInterface(fd_client, connInfo.clientID);
         CPPUNIT_ASSERT(ret == 0 || ret == FNDInvalidClientID);

         destroyReceiverChannel(chid);

         (void)SBClose(fd_server);
         (void)SBClose(fd_client);

         sleepMs(50);
      }
   }
}
