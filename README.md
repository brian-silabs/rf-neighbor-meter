## README

This repository contains a plugin made for EmberZNet 6.7.10 (Simplicity v4 and Appbuilder)

# Purpose 
This is a demonstrator aiming to collect LQIs and RSSIs of the neihboring nodes of the one running this plugin
The RSSIs and LQIs are updated upon reception of ZDO commands in this example
These metrics are collected on both Neighbors and Child devices
Finally, all the data can be sent Over the Air using ZCL commands :
* The Server side of that cluster sends updates whenever a ZDO message is received by an entry
* The clien side can query entries individually as well as the entire table
The customized ZCL protocol is attached as an extension in `rf-neighbor-meter-zcl-extensions.xml`

The ultimate goal is to be able to have a full instant RF link view from a device and getting it remotely

# How to install the plugin
* Simply clone this repository within `<sdk_folder>/protocol/zigbee/app/framework/plugin`
* Once done reload the SDK from Simplicity Studio preferences 

# How to deploy the plugin within a project' ISC file
1. First add the ZCL extension to the project using the `Zigbee Stack` tab
2. Then Enable the `RF Neighbor Meter` Server cluster
3. Also add the support for client commands handling if you want to be able to receive and handle it
    `getAllEntries`, `getEntry`, `setRemoteShortId`
4. In the `Plugins` tab of the ISC, look for `RF Neighbor Meter` and add it
5. Enable 3 callback functions from the `Callbacks` tab
    `Get All Entries`, `Get Entry`, `Set Remote Short Id`
6. Generate and build the project

# Usage of the ZCL commands

By default the Server implememntation sends commands to a *Coordinator* (Address `0x0000`)

The ZCL commands the Server can receiver are :

| Command               | Argument      | Description                                           |
| --------------------- | ------------- | ----------------------------------------------------- |
| setRemoteShortId      | uint16_t      | Sets the destionation short Id to send the entries to |
| getEntry              | EUI64         | Queries an entry for the given Node EUI64             |
| getAllEntries         | None          | Queries all entries at once                           |

An example of a remote querier is provided in the `RF_Neighbor_Meter-Remote_Reader_Demo.c` file
It is extracted from a previous alpha and won't work as is, but gives an idea of how to request data through ZCL commands

# Notes 

I nor Silicon Labs aim at maintaining this, use it as is
It is not meant to be optimized, therefore meant to be used for testing only
It can however provide help in building a more suitable plugin which can keep track of all the network topology
