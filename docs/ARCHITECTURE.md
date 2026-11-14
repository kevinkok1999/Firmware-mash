# BW16 PRO — ontwerp en validatie

## Hardware
- Doelchip: Realtek RTL8720DN op Ai-Thinker BW16.
- Ontwikkelbasis: officiële AmebaD Arduino SDK.
- Huidige behuizing: OLED, knoppen en 2,4/5GHz-schakelaar; type OLED-controller, pinmapping, voedingsrails en bootloader zijn **nog niet vastgesteld**.
- Geen GPIO- of displaydriver aannemen voordat de specifieke PCB is geïnspecteerd.

## V0.1: wat werkt in de broncode?
- Wi-Fi-netwerken opvragen via `WiFi.scanNetworks()`.
- SSID, RSSI en gerapporteerd beveiligingstype via de seriële monitor.
- Handmatige en periodieke scans.
- Zonder verbinding met een AP, credentials of packet-injectie.

Nog niet gecompileerd op echte hardware; boardvariant en gebruikte Ameba-core moeten gecontroleerd worden.

## V0.2: na hardwarevalidatie
- I2C/SPI-OLED driver identificeren; scherm exact mappen.
- Knoppen debouncen, menu en lijstweergave.
- Kanaal- en bandinformatie via de officiële AmebaD advanced scan-API; SDK-versie controleren.
- CSV-export via seriële interface.
- Optionele toegestane wifi/BLE-inventarisatie.

## V0.3: netwerkgezondheid
- Signaalhistorie, roamingobservaties en kanaaldrukte.
- Router-eigen loggegevens importeren en trends tonen.
- PMF-instellingen controleren aan de hand van routerconfiguratie; niet aannemen dat een gewone SSID-scan alle PMF-mogelijkheden betrouwbaar laat zien.
- Beheer- en hersteladviezen, geen verstoring.

## Hypothetische scenario's
De offline simulator beschrijft op hoog niveau:
1. deauthentication en de rol van PMF;
2. misleidende beacon-advertenties en identificatie;
3. AP-beschikbaarheidsproblemen en monitoring.

Deze module verzendt geen wifi-frames en berekent geen aanvalssnelheden, packet-injectie of ontregelingsparameters.

## QA/gates
1. Bewaar eerst een geverifieerde herstelroute voor de bestaande firmware.
2. Compileer met de officiële RTL8720DN BW16 boardvariant.
3. Test via een isolerend labnetwerk.
4. Controleer OLED en knoppen afzonderlijk voordat de interface wordt ingeschakeld.
5. Voeg GitHub Actions compilechecks toe als de boardtoolchain reproduceerbaar is.

## Bronnen
- Realtek AmebaD Arduino: https://github.com/Ameba-AIoT/ameba-arduino-d
- Advanced ScanNetworks: https://github.com/Ameba-AIoT/ameba-arduino-d/blob/dev/Arduino_package/hardware/libraries/WiFi/examples/ScanNetworksAdvanced/ScanNetworksAdvanced.ino
