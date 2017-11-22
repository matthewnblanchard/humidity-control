# Humity Based Fan Controller / Dehumidifier

## Authors
Christian Auspland  
Matthew Blanchard

## Overview
The Humidity Based Fan Controller / Dehumidifier (HBFC/D) project aims to prototype
a humidity management system, which can reduce the humidity inside an enclosed space
by driving an exhaust fan. The exhaust fan will push humid air out of the space 
whenever the interior humidity is greater than the exterior humidity and a user defined 
threshold. Interior and exterior humidities are read by two ESP-12S modules (containing an 
ESP-8266 Wifi Module/Microcontroller), which communicate with one another. The interior ESP-12S 
determines the humidity difference and drives the exhaust fan accordingly.

The interior system also serves a web page to allow for user configuration/monitoring.
A WebSocket is utilized to provide real time updates through the webpage. The fan drive speed
and humidity threshold can be configured from this WebSocket

## Summary of Files
---
### user\_main
The main program file. The program begins here, and the task scheduling for the program is handled here.
THe program begins by initializing 
