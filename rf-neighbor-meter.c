/***************************************************************************//**
 * @file
 * @brief Definitions for the RF Neighbor Meter plugin, which provides a way
 *        to record LQI and RSSI of neighbors upon packet reception.
 *******************************************************************************
 
 ******************************************************************************/

#include "rf-neighbor-meter.h"

void refreshEui64TableEventHandler(void);

static void     sendEntryResponse                 (EmberEUI64 eui);
static void     sendEntryUpdate                   (uint8_t entryIndex);
static uint8_t  getNeighborRfEntryIndexByShortID  (EmberNodeId sender);
static uint8_t  getNeighborRfEntryIndexByEUI64    (EmberEUI64 senderEui);
static uint8_t  getNeighborRfEmptyEntryIndex      (void);

neighboringNodeRfInfo_t neighboringNodesRfTable[NEGIGHBORING_RF_TABLE_SIZE];//Portage Metrics
static uint8_t          neighborEntryToRefresh;

void initCallback(void)
{
  uint8_t var = 0;

  for (var = 0; var < NEGIGHBORING_RF_TABLE_SIZE; ++var) {
	  neighboringNodesRfTable[var].nodeShortId = EMBER_NULL_NODE_ID;
  }

}

void refreshEui64TableEventHandler(void)
{
	neighboringNodeRfInfo_t n;
	emberEventControlSetInactive(refreshEui64TableEventControl);

	emberAfCorePrintln("Updating RF Entry %d", neighborEntryToRefresh);

	n = neighboringNodesRfTable[neighborEntryToRefresh];
	if (n.nodeShortId != EMBER_NULL_NODE_ID) {
		//Check if eui64 is empty
		if(memcmp(n.nodeEUI, emberAfNullEui64, EUI64_SIZE) == 0)
		{
			emberAfCorePrintln("EUI not present, sending IEEE request !!");
			emberIeeeAddressRequest(n.nodeShortId, true, 0, EMBER_APS_OPTION_ZDO_RESPONSE_REQUIRED);
			emberAfCorePrintln("Requested EUI64 for 0x%2X", n.nodeShortId);
		}
	} else {
		emberAfCorePrintln("Invalid RF Entry to Refresh");
	}

	sendEntryUpdate(neighborEntryToRefresh);
}

static uint8_t getNeighborRfEntryIndexByShortID(EmberNodeId sender)//Portage Metrics
{
	uint8_t var;
	uint8_t returnValue = 0xFF;

	for (var = 0; var < NEGIGHBORING_RF_TABLE_SIZE; ++var) {
		if(neighboringNodesRfTable[var].nodeShortId == sender)
		{
			returnValue = var;
			break;
		}
	}

	return returnValue;
}

static uint8_t getNeighborRfEntryIndexByEUI64(EmberEUI64 senderEui)//Portage Metrics
{
	uint8_t var;
	uint8_t returnValue = 0xFF;

	for (var = 0; var < NEGIGHBORING_RF_TABLE_SIZE; ++var) {
		if(memcmp(neighboringNodesRfTable[var].nodeEUI, senderEui, EUI64_SIZE) == 0)
		{
			returnValue = var;
			break;
		}
	}

	return returnValue;
}


static uint8_t getNeighborRfEmptyEntryIndex(void)//Portage Metrics
{
	uint8_t var;
	uint8_t returnValue = 0xFF;

	for (var = 0; var < NEGIGHBORING_RF_TABLE_SIZE; ++var) {
		if(neighboringNodesRfTable[var].nodeShortId == EMBER_NULL_NODE_ID)
		{
			returnValue = var;
			break;
		}
	}

	return returnValue;
}


void sendEntryResponse(EmberEUI64 eui)
{
	EmberNodeId destination;
	EmberStatus status;
	uint8_t entryIndex = 0xFF;

	emberAfReadManufacturerSpecificServerAttribute(
			LIGHT_ENDPOINT,
			ZCL_RF_NEGHBORHOOD_DEBUG_CLUSTER_ID,
			ZCL_DESTINATION_SHORT_ID_ATTRIBUTE_ID,
			0x1003,
			(uint8_t *)(&destination),
			2);

	emberAfGetCommandApsFrame()->sourceEndpoint = LIGHT_ENDPOINT;
	emberAfGetCommandApsFrame()->destinationEndpoint = LIGHT_ENDPOINT;

	entryIndex = getNeighborRfEntryIndexByEUI64(eui);
	if(entryIndex != 0xFF)
	{
		emberAfFillCommandRfNeghborhoodDebugClustergetEntryResponse(&neighboringNodesRfTable[entryIndex], sizeof(neighboringNodeRfInfo_t));
		emberAfCorePrintln("sendEntryResponse found entry");
	} else {
		//TODO send ZCL error
		//emberAfFillCommandGlobalServerToClientReadAttributesResponse()
		emberAfCorePrintln("sendEntryResponse found no entry");
	}

	status = emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT, destination);
	emberAfCorePrintln("Sent entry command to : 0x%2X, status = 0x%X", (uint16_t)(destination), status);
}

void sendEntryUpdate(uint8_t entryIndex)
{
	EmberNodeId destination;
	uint8_t updatesEnabled = 0;
	EmberStatus status;

	emberAfReadManufacturerSpecificServerAttribute(
			LIGHT_ENDPOINT,
			ZCL_RF_NEGHBORHOOD_DEBUG_CLUSTER_ID,
			ZCL_UPDATE_THRESHOLD_ATTRIBUTE_ID,
			0x1003,
			&updatesEnabled,
			1);

	if(updatesEnabled)
	{
		emberAfReadManufacturerSpecificServerAttribute(
				LIGHT_ENDPOINT,
				ZCL_RF_NEGHBORHOOD_DEBUG_CLUSTER_ID,
				ZCL_DESTINATION_SHORT_ID_ATTRIBUTE_ID,
				0x1003,
				(uint8_t *)(&destination),
				2);

		emberAfGetCommandApsFrame()->sourceEndpoint = LIGHT_ENDPOINT;
		emberAfGetCommandApsFrame()->destinationEndpoint = LIGHT_ENDPOINT;

		if(entryIndex != 0xFF)
		{
			emberAfFillCommandRfNeghborhoodDebugClusterentryUpdate(&neighboringNodesRfTable[entryIndex], sizeof(neighboringNodeRfInfo_t));
			emberAfCorePrintln("sendEntryUpdate found entry");
			status = emberAfSendCommandUnicast(EMBER_OUTGOING_DIRECT, destination);
			emberAfCorePrintln("Sent entry command to : 0x%2X, status = 0x%X", (uint16_t)(destination), status);
		} else {
			emberAfCorePrintln("sendEntryUpdate found no entry");
		}
	} else {
		emberAfCorePrintln("Updates are disabled, write the cmd attribute to enable it");
	}
}

////////////////////////////////////////////////////////////////
/////////////////// Callback implementations ///////////////////
////////////////////////////////////////////////////////////////

/** @brief Incoming Packet Filter
 *
 * NOTE: REQUIRES INCLUDING THE PACKET-HANDOFF PLUGIN. This is called by the
 * Packet Handoff plugin when the stack receives a packet from one of the
 * protocol layers specified in ::EmberZigbeePacketType. The packetType argument
 * is one of the values of the ::EmberZigbeePacketType enum. If the stack
 * receives an 802.15.4 MAC beacon, it will call this function with the
 * packetType argument set to ::EMBER_ZIGBEE_PACKET_TYPE_BEACON. The
 * implementation of this callback may alter the data contained in packetData,
 * modify options and flags in the auxillary data, or consume the packet itself,
 * either sending the message, or discarding it as it sees fit.
 *
 * @param packetType the type of packet and associated protocol layer  Ver.:
 * always
 * @param packetData flat buffer containing the packet data associated with the
 * packet type  Ver.: always
 * @param size_p a pointer containing the size value of the packet  Ver.: always
 * @param data auxillary data included with the packet  Ver.: always
 */
EmberPacketAction emberAfIncomingPacketFilterCallback(EmberZigbeePacketType packetType,
                                                      int8u* packetData,
                                                      int8u* size_p,
                                                      void* data)
{
  EmberApsFrame *pApsStruct = NULL;
  //EmberStatus status = EMBER_SUCCESS;
  EmberNodeId sender = emCurrentSender;// emberGetSender();//emCurrentSender;
  //uint32_t i = 0;

  if ((EMBER_ZIGBEE_PACKET_TYPE_ZDO != packetType)//Early return the packet if not a ZDO one
      ||(sender == emberGetNodeId()))//Early return the packet has been sent by us
  {
      return EMBER_ACCEPT_PACKET;
  }

  pApsStruct = (EmberApsFrame *)data;
  switch (pApsStruct->clusterId) {
    case LEAVE_REQUEST:
      emberAfCorePrintln("ZDO leave request received");
      emberEventControlSetActive(scheduleSteeringEventControl);
      break;
    case PERMIT_JOINING_REQUEST:
      emberAfCorePrintln("ZDO permit joining request received");
      //We do not send the permit join request manually, stack will do it
      //We just parse the payload to handle proprietary behavior
      if(packetData[1]){//permit joining time
          emberAfCorePrintln("Network Opened by peer");
      } else {
          emberAfCorePrintln("Network Closed by peer");
      }
      break;
    default:
      emberAfCorePrintln("Unhandled by App ZDO request received");
      break;
  }

  //TODO Add packet filtering
  if (sender != EMBER_NULL_NODE_ID) {//Portage Metrics
    uint8_t entryIndex = getNeighborRfEntryIndexByShortID(sender);
    if (entryIndex == 0xFF) {
    	//No entry exists for this node, we query some new
    	entryIndex = getNeighborRfEmptyEntryIndex();
    	//If we have space left
    	if (entryIndex != 0xFF)
    		neighboringNodesRfTable[entryIndex].nodeShortId = sender;
    	//TODO cleanup this piece of code maybe using a counter instead
    }

    if (entryIndex != 0xFF) {
    	emberGetLastHopLqi(&(neighboringNodesRfTable[entryIndex].lastPacketLQI));
    	emberGetLastHopRssi(&(neighboringNodesRfTable[entryIndex].lastPacketRssi));
#if(NEGIGHBORING_RF_TABLE_TRACK_EUI)
    	if ( (pApsStruct->clusterId) == IEEE_ADDRESS_RESPONSE ) {
    		memcpy(&(neighboringNodesRfTable[entryIndex].nodeEUI), &packetData[2], EUI64_SIZE);
    		emberAfCorePrintln("EUI Received !!");
    	}
#endif
    	neighborEntryToRefresh = entryIndex;
    	emberEventControlSetActive(refreshEui64TableEventControl);
    } else {
    	emberAfCorePrintln("No more space left in RF neighbor table");
    }
  } else {
	  emberAfCorePrintln("RF neighbor ignored");
  }

  return EMBER_ACCEPT_PACKET; //Always let the stack do the job for handling ZDO commands
}

/** @brief RF Neighbors Debug Cluster Get All Entries
 *
 *
 *
 */
boolean emberAfRfNeghborhoodDebugClusterGetAllEntriesCallback(void)
{
	return false;
}

/** @brief RF Neighbors Debug Cluster Get Entry
 *
 *
 *
 * @param argOne   Ver.: always
 */
boolean emberAfRfNeghborhoodDebugClusterGetEntryCallback(int8u* neighborEui)
{
	EmberEUI64 queriedEui;

	memcpy(queriedEui, neighborEui, EUI64_SIZE);

	emberAfCorePrint("Get entry command received for :");
    emberAfAppDebugExec(emberAfPrintBigEndianEui64(neighborEui));
    emberAfCorePrintln("");

	sendEntryResponse(queriedEui);

	return true;
}

/** @brief RF Neighbors Debug Cluster Set Remote Short Id
 *
 *
 *
 * @param argOne   Ver.: always
 */
boolean emberAfRfNeghborhoodDebugClusterSetRemoteShortIdCallback(int16u argOne)
{
	//write the new destination into the attribute
	  EmberStatus status;
	  status =
	    emberAfWriteManufacturerSpecificServerAttribute(LIGHT_ENDPOINT,
	    												ZCL_RF_NEGHBORHOOD_DEBUG_CLUSTER_ID,
														ZCL_DESTINATION_SHORT_ID_ATTRIBUTE_ID,
	                                                    0x1003,
	                                                    (uint8_t *) &argOne,
	                                                    ZCL_INT16U_ATTRIBUTE_TYPE);

	emberAfCorePrintln("New destination address set : 0x%2X, status = 0x%X", argOne, status);
	return true;
}
