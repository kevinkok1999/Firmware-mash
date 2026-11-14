# BW16 Analyzer PRO — prototype v0.1

Een uitbreidbaar firmwareproject voor de **Ai-Thinker BW16 / Realtek RTL8720DN**.

## Status
**Broncode aanwezig; niet gecompileerd of geflasht op echte hardware.** Het originele OLED-scherm, de knoppen en de 2,4/5GHz-schakelaar zijn nog niet aangesloten: exacte pinnen en displaycontroller zijn onbekend.

## Bestanden
- `firmware/BW16_Analyzer/BW16_Analyzer.ino` — wifi-scanner via USB-seriële monitor, met handmatig en periodiek scannen.
- `simulator/scenarios.py` — offline hypothesemodel voor drie wifi-beveiligingsscenario's.
- `simulator/test_scenarios.py` — eenvoudige unittests voor het model.
- `docs/ARCHITECTURE.md` — hardwarereview, roadmap en testcriteria.

## Arduino installeren
1. Installeer Arduino IDE.
2. Voeg via **Preferences → Additional Boards Manager URLs** de officiële stabiele AmebaD-package toe:
   `https://github.com/Ameba-AIoT/ameba-arduino-d/raw/master/Arduino_package/package_realtek_amebad_index.json`
3. Installeer **Realtek AmebaD** via Boards Manager.
4. Selecteer de **RTL8720DN BW16**-boardvariant. Controleer vóór iedere upload de juiste printplaat, USB-serieelinterface, bootprocedure en een herstelfunctie voor de huidige firmware.
5. Open de `.ino`-sketch. **Compileer eerst zonder te flashen**.
6. Als compilatie en hardwareherstel zijn geverifieerd: gebruik de serial monitor op **115200 baud**.

Seriële commando's: `s` = scan, `a` = periodieke scan in- of uitschakelen, `i` = informatie, `h` = help.

## Offline simulaties uitvoeren
```bash
python3 simulator/scenarios.py
python3 -m unittest discover -s simulator -v
```

Dit is een *educatief conceptmodel*, geen voorspelling van de effectiviteit van wifi-aanvallen. Het model zendt geen radiosignalen uit.

## Grenzen
De firmware bevat netwerkdetectie, analyse en offline scenario's, **geen** deauth-injectie, beacon flooding, phishing of AP-DoS.

## Primaire bronnen
- https://github.com/Ameba-AIoT/ameba-arduino-d
- https://github.com/Ameba-AIoT/ameba-arduino-d/blob/dev/Arduino_package/hardware/libraries/WiFi/examples/ScanNetworks/ScanNetworks.ino
- https://github.com/Ameba-AIoT/ameba-arduino-d/blob/dev/Arduino_package/hardware/libraries/WiFi/examples/ScanNetworksAdvanced/ScanNetworksAdvanced.ino
