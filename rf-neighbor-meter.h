/***************************************************************************//**
 * @file
 * @brief Definitions for the RF Neighbor Meter plugin.
 *******************************************************************************
 ******************************************************************************/

// -----------------------------------------------------------------------------
// Constants

typedef struct {
  uint16_t nodeShortId;
  EmberEUI64 nodeEUI;
  int8_t lastPacketRssi;
  uint8_t lastPacketLQI;
} neighboringNodeRfInfo_t;//Portage Metrics