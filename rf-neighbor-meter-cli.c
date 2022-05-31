/***************************************************************************//**
 * @file
 * @brief CLI for the RF Neighbor meter plugin.
 *******************************************************************************

 ******************************************************************************/

// -----------------------------------------------------------------------------
// Helper macros and functions

#include "rf-neighbor-meter.h"

void printRfNeighborTable(void)
{
  uint8_t used = 0;
  uint8_t i;
  neighboringNodeRfInfo_t n;

  emberAfAppPrintln("\n#  id     latest lqi     latest rssi " "  eui");

  for (i = 0; i < NEGIGHBORING_RF_TABLE_SIZE; i++) {
    n = neighboringNodesRfTable[i];
    if (n.nodeShortId == EMBER_NULL_NODE_ID) {
      continue;//Leave the entry
    }

    used++;

    emberAfAppPrint("%d: 0x%2X      %d               %d  ",
                    i,
                    n.nodeShortId,
                    n.lastPacketLQI,
                    n.lastPacketRssi
                    );

    emberAfAppDebugExec(emberAfPrintBigEndianEui64(n.nodeEUI));

    emberAfAppPrintln("");
    emberAfAppFlush();
  }

  emberAfAppPrintln("\n%d of %d entries used.", used, NEGIGHBORING_RF_TABLE_SIZE);
}
