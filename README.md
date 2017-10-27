# Humity Based Fan Controller / Dehumidifier

## Authors
Christian Auspland
Matthew Blanchard

## Overview
The Humidity Based Fan Controller / Dehumidifier (HBFC/D) project aims to prototype
a humidity management system, which can reduce the humidity inside an enclosed space
by driving an exhaust fan. The exhaust fan will push humid air out of the space 
whenever the interior humidity is greater than the exterior humidity. Interior and
exterior humidities are read by two ESP-12S modules (containing an ESP-8266 Wifi 
Module/Microcontroller), which communicatr with one another. The interior ESP-12S 
determines the humidity difference and drives the exhaust fan accordingly.

The project also contains a user program, designed for Linux CLI use, which will
locate and communicate with the ESP-12S modules, for debugging/logging purposes 
as well as control purposes.. 
