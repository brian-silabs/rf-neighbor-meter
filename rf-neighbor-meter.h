/***************************************************************************//**
 * @file
 * @brief Definitions for the RF Neighbor Meter plugin.
 *******************************************************************************
 ******************************************************************************/

// -----------------------------------------------------------------------------
// Constants

#include "app/framework/include/af.h"

#define NEGIGHBORING_RF_TABLE_SIZE        (15) //Portage Metrics
#define NEGIGHBORING_RF_TABLE_TRACK_EUI   (1)

typedef struct {
  uint16_t nodeShortId;
  EmberEUI64 nodeEUI;
  int8_t lastPacketRssi;
  uint8_t lastPacketLQI;
} neighboringNodeRfInfo_t;//Portage Metrics

void initCallback(void);