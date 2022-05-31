/***************************************************************************//**
 * @file
 * @brief
 *******************************************************************************
 * # License
 * <b>Copyright 2018 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/

// This callback file is created for your convenience. You may add application
// code to this file. If you regenerate this file over a previous version, the
// previous version will be overwritten and any code you have added will be
// lost.

#include "app/framework/include/af.h"

#include EMBER_AF_API_NETWORK_STEERING
#include EMBER_AF_API_FIND_AND_BIND_TARGET
#include EMBER_AF_API_ZLL_PROFILE

#include "client-command-macro.h"

#define LIGHT_ENDPOINT (1)

typedef struct {
	uint16_t nodeShortId;
	EmberEUI64 nodeEUI;
	int8_t lastPacketRssi;
	uint8_t lastPacketLQI;
} neighboringNodeRfInfo_t;//Portage Metrics

EmberEventControl commissioningLedEventControl;
EmberEventControl findingAndBindingEventControl;

static void setRemoteShortDestination(void);
static void getRFEntryStats(void);
static void getAllRFEntriesStats(void);

EmberCommandEntry emberAfCustomCommands[] = {
  emberCommandEntryAction("setRemoteShortDest", setRemoteShortDestination, "", "Sets Destination for Sending RF reports"),
  emberCommandEntryAction("getAllRFEntries", getAllRFEntriesStats, "", "Queries latest Report for all entries"),
  emberCommandEntryAction("getRFEntry", getRFEntryStats, "b", "Queries latest Report for EUI64"),
  emberCommandEntryTerminator()
};

static void setRemoteShortDestination(void)
{
	EmberStatus status;
	emberAfGetCommandApsFrame()->sourceEndpoint = LIGHT_ENDPOINT;
	emberAfGetCommandApsFrame()->destinationEndpoint = LIGHT_ENDPOINT;
	emberAfFillCommandRfNeghborhoodDebugClustersetRemoteShortId((uint16_t)emberGetNodeId());
	status = emberAfSendCommandBroadcast(0xFFFF);//TODO Routers only
	emberAfCorePrintln("Broadcast sent Set Remote Short Destination to 0x%2X, status=0x%X", emberGetNodeId(), status);
}

static void getRFEntryStats(void)
{
	EmberEUI64 requestedEui;
	EmberStatus status;

	if (0 == emberCopyBigEndianEui64Argument(0, requestedEui)) {
	emberAfAppPrintln("Error in eui ");
	}

	emberAfGetCommandApsFrame()->sourceEndpoint = LIGHT_ENDPOINT;
	emberAfGetCommandApsFrame()->destinationEndpoint = LIGHT_ENDPOINT;
	emberAfFillCommandRfNeghborhoodDebugClustergetEntry(requestedEui);
	status = emberAfSendCommandBroadcast(0xFFFF);//TODO Unicast
    emberAfCorePrintln("Sent Get Entry Command status = 0x%X",status);
}

static void getAllRFEntriesStats(void)
{
	EmberStatus status;

	emberAfGetCommandApsFrame()->sourceEndpoint = LIGHT_ENDPOINT;
	emberAfGetCommandApsFrame()->destinationEndpoint = LIGHT_ENDPOINT;
	emberAfFillCommandRfNeghborhoodDebugClustergetAllEntries();
	status = emberAfSendCommandBroadcast(0xFFFF);//TODO Unicast
    emberAfCorePrintln("Sent Get All Entries Command status = 0x%X",status);
}

void commissioningLedEventHandler(void)
{
  emberEventControlSetInactive(commissioningLedEventControl);

  if (emberAfNetworkState() == EMBER_JOINED_NETWORK) {
    uint16_t identifyTime;
    emberAfReadServerAttribute(LIGHT_ENDPOINT,
                               ZCL_IDENTIFY_CLUSTER_ID,
                               ZCL_IDENTIFY_TIME_ATTRIBUTE_ID,
                               (uint8_t *)&identifyTime,
                               sizeof(identifyTime));
    if (identifyTime > 0) {
      halToggleLed(COMMISSIONING_STATUS_LED);
      emberEventControlSetDelayMS(commissioningLedEventControl,
                                  LED_BLINK_PERIOD_MS << 1);
    } else {
      halSetLed(COMMISSIONING_STATUS_LED);
    }
  } else {
    EmberStatus status = emberAfPluginNetworkSteeringStart();
    emberAfCorePrintln("%p network %p: 0x%X", "Join", "start", status);
  }
}

void findingAndBindingEventHandler()
{
  if (emberAfNetworkState() == EMBER_JOINED_NETWORK) {
    emberEventControlSetInactive(findingAndBindingEventControl);
    emberAfCorePrintln("Find and bind target start: 0x%X",
                       emberAfPluginFindAndBindTargetStart(LIGHT_ENDPOINT));

  }
}

/** @brief Stack Status
 *
 * This function is called by the application framework from the stack status
 * handler.  This callbacks provides applications an opportunity to be notified
 * of changes to the stack status and take appropriate action.  The return code
 * from this callback is ignored by the framework.  The framework will always
 * process the stack status after the callback returns.
 *
 * @param status   Ver.: always
 */
bool emberAfStackStatusCallback(EmberStatus status)
{
  // Note, the ZLL state is automatically updated by the stack and the plugin.
  if (status == EMBER_NETWORK_DOWN) {
    halClearLed(COMMISSIONING_STATUS_LED);
  } else if (status == EMBER_NETWORK_UP) {
    halSetLed(COMMISSIONING_STATUS_LED);
    emberEventControlSetActive(findingAndBindingEventControl);
  }

// This value is ignored by the framework.
  return false;
}

/** @brief Main Init
 *
 * This function is called from the application's main function. It gives the
 * application a chance to do any initialization required at system startup.
 * Any code that you would normally put into the top of the application's
 * main() routine should be put into this function.
        Note: No callback
 * in the Application Framework is associated with resource cleanup. If you
 * are implementing your application on a Unix host where resource cleanup is
 * a consideration, we expect that you will use the standard Posix system
 * calls, including the use of atexit() and handlers for signals such as
 * SIGTERM, SIGINT, SIGCHLD, SIGPIPE and so on. If you use the signal()
 * function to register your signal handler, please mind the returned value
 * which may be an Application Framework function. If the return value is
 * non-null, please make sure that you call the returned function from your
 * handler to avoid negating the resource cleanup of the Application Framework
 * itself.
 *
 */
void emberAfMainInitCallback(void)
{
  emberEventControlSetActive(commissioningLedEventControl);
}

/** @brief Complete
 *
 * This callback is fired when the Network Steering plugin is complete.
 *
 * @param status On success this will be set to EMBER_SUCCESS to indicate a
 * network was joined successfully. On failure this will be the status code of
 * the last join or scan attempt. Ver.: always
 * @param totalBeacons The total number of 802.15.4 beacons that were heard,
 * including beacons from different devices with the same PAN ID. Ver.: always
 * @param joinAttempts The number of join attempts that were made to get onto
 * an open Zigbee network. Ver.: always
 * @param finalState The finishing state of the network steering process. From
 * this, one is able to tell on which channel mask and with which key the
 * process was complete. Ver.: always
 */
void emberAfPluginNetworkSteeringCompleteCallback(EmberStatus status,
                                                  uint8_t totalBeacons,
                                                  uint8_t joinAttempts,
                                                  uint8_t finalState)
{
  emberAfCorePrintln("%p network %p: 0x%X", "Join", "complete", status);

  if (status != EMBER_SUCCESS) {
    // Initialize our ZLL security now so that we are ready to be a touchlink
    // target at any point.
    status = emberAfZllSetInitialSecurityState();
    if (status != EMBER_SUCCESS) {
      emberAfCorePrintln("Error: cannot initialize ZLL security: 0x%X", status);
    }
  }
}

/** @brief On/off Cluster Server Post Init
 *
 * Following resolution of the On/Off state at startup for this endpoint, perform any
 * additional initialization needed; e.g., synchronize hardware state.
 *
 * @param endpoint Endpoint that is being initialized  Ver.: always
 */
void emberAfPluginOnOffClusterServerPostInitCallback(uint8_t endpoint)
{
  // At startup, trigger a read of the attribute and possibly a toggle of the
  // LED to make sure they are always in sync.
  emberAfOnOffClusterServerAttributeChangedCallback(endpoint,
                                                    ZCL_ON_OFF_ATTRIBUTE_ID);
}

/** @brief Server Attribute Changed
 *
 * On/off cluster, Server Attribute Changed
 *
 * @param endpoint Endpoint that is being initialized  Ver.: always
 * @param attributeId Attribute that changed  Ver.: always
 */
void emberAfOnOffClusterServerAttributeChangedCallback(uint8_t endpoint,
                                                       EmberAfAttributeId attributeId)
{
  // When the on/off attribute changes, set the LED appropriately.  If an error
  // occurs, ignore it because there's really nothing we can do.
  if (attributeId == ZCL_ON_OFF_ATTRIBUTE_ID) {
    bool onOff;
    if (emberAfReadServerAttribute(endpoint,
                                   ZCL_ON_OFF_CLUSTER_ID,
                                   ZCL_ON_OFF_ATTRIBUTE_ID,
                                   (uint8_t *)&onOff,
                                   sizeof(onOff))
        == EMBER_ZCL_STATUS_SUCCESS) {
      if (onOff) {
        halSetLed(ON_OFF_LIGHT_LED);
      } else {
        halClearLed(ON_OFF_LIGHT_LED);
      }
    }
  }
}

/** @brief Hal Button Isr
 *
 * This callback is called by the framework whenever a button is pressed on the
 * device. This callback is called within ISR context.
 *
 * @param button The button which has changed state, either BUTTON0 or BUTTON1
 * as defined in the appropriate BOARD_HEADER.  Ver.: always
 * @param state The new state of the button referenced by the button parameter,
 * either ::BUTTON_PRESSED if the button has been pressed or ::BUTTON_RELEASED
 * if the button has been released.  Ver.: always
 */
void emberAfHalButtonIsrCallback(uint8_t button, uint8_t state)
{
  if (state == BUTTON_RELEASED) {
    emberEventControlSetActive(findingAndBindingEventControl);
  }
}

/** @brief RF Neighbors Debug Cluster Get Entry Response
 *
 *
 *
 * @param argOne   Ver.: always
 */
boolean emberAfRfNeghborhoodDebugClusterGetEntryResponseCallback(int8u* neighborStruct)
{

	neighboringNodeRfInfo_t *receivedEEntry = (neighboringNodeRfInfo_t *)(neighborStruct);

	emberAfCorePrintln("Entry Response received");

	emberAfAppPrintln("\n id  latest lqi     latest rssi " "  eui");
	emberAfAppPrint("0x%2X      %d               %d  ",
		 receivedEEntry->nodeShortId,
		 receivedEEntry->lastPacketLQI,
		 receivedEEntry->lastPacketRssi);
	emberAfAppDebugExec(emberAfPrintBigEndianEui64(receivedEEntry->nodeEUI));

	emberAfAppPrintln("");
	emberAfAppFlush();


	return true;
}


/** @brief RF Neighbors Debug Cluster Entry Update
 *
 *
 *
 * @param argOne   Ver.: always
 */
boolean emberAfRfNeghborhoodDebugClusterEntryUpdateCallback(int8u* neighborStruct)
{
	neighboringNodeRfInfo_t *receivedEEntry = (neighboringNodeRfInfo_t *)(neighborStruct);

	emberAfCorePrintln("Entry Update received");

	emberAfAppPrintln("\n id  latest lqi     latest rssi " "  eui");
	emberAfAppPrint("0x%2X      %d               %d  ",
		 receivedEEntry->nodeShortId,
		 receivedEEntry->lastPacketLQI,
		 receivedEEntry->lastPacketRssi);
	emberAfAppDebugExec(emberAfPrintBigEndianEui64(receivedEEntry->nodeEUI));

	emberAfAppPrintln("");
	emberAfAppFlush();


	return true;
}

