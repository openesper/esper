![PCB](https://imgur.com/jdanInhl.jpg)

# Esper Hardware Design

## Goals

All Esper hardware is designed to be as easy to modify and manufacture as possible. For the most part this means using free, open-source tools and simplifying the design to have the smallest DOM possible. 

### PCB & Schematic

KiCAD is used for all PCB design since it is free and has a large community support. 

### Manufacturing

The Esper device is designed to be manufactured and assembled at JLCPCB. At low quantities (5-10 boards), no other board house comes close on price and ease of use.

This may change in the future if other companies start matching JLCPCB in price, or if Esper starts being manufactures at higher volumes.

### Part Selection

To make the process of manufacturing and assembly as easy as possible, all components (except connectors) are chosen from the [JLCPCB parts library](https://jlcpcb.com/parts) so JLCPCB assembly service can be used. This means that connectors are the only parts that need to be hand soldered, but JLC recently added connectors to their parts library so this may change soon.

Also, in order to minimize the amount of work needed to keep custom schematic symbols and footprints up-to-date, it was a priority to minimize the amount of custom components and only use parts found in the default KiCAD library. Almost every part in the project has footprints and symbols included in KiCAD, or has an equivalent component included.

### Enclosure

There are many 3D modeling tools out there, to varying degrees of free-ness, but I am not qualified to go over the pros and cons of each. FreeCAD was chosen for this project simply because it seemed to be the most popular open-source program, but almost any would work.

The enclosure is currently designed to be 3D printed, but alternatives are being looked into.


## BOM

To fully assemble an Esper, these parts are needed:

| Part | Quantity |
| --- | --- |
| Enclosure Top | 1 |
| Enclosure Bottom | 1 |
| Enclosure Dome | 1 |
| Esper PCBA | 1 |
| [Heat-Set Inserts (M2 x 0.4mm) x 2.5mm](https://www.mcmaster.com/catalog/127/3973) | 2 |
| [Screws (M2 x 0.4mm) x 6mm](https://www.mcmaster.com/catalog/127/3251) | 2 |