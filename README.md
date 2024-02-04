# Wasserrakete mit File (.csv) download im Browser


## Installation 

Verwendete Bibliotheken:

| Name | Link |
|------|------|
| ESPAsyncWebserver | https://github.com/me-no-dev/ESPAsyncWebServer?utm_source=platformio&utm_medium=piohome |
| Adafruit_BME280_Library | https://github.com/adafruit/Adafruit_BME280_Library?utm_source=platformio&utm_medium=piohome |

## Upload des Programms auf einen neuen ESP

Das Programm nutzt LittleFS um die index.html Datei auf dem ESP zu speichern. Über LittleFS können Dateien auf dem ESP abgelegt werden, ähnlich einem USB Stick. Wichtig hierbei ist, dass der Upload der Dateien **NICHT** beim Upload des Arduino Programms erfolgt. Der Upload der index.html Datei muss manuell angestoßen werden. Wie dies geht, ist hier beschrieben: https://github.com/earlephilhower/arduino-esp8266littlefs-plugin

Am besten wird für den Upload der index.html (für das LittlsFS) die Arduino IDE 1 oder 2 verwendet. Die Verwendung von VS Code wurde nicht getestet.

Kurz zusammengefasst. Es muss eine `.jar` Datei heruntergeladen und an einem bestimmten Pfad entpackt werden. Der Pfad lautet:
```bash
<SketchbookPfad>/tools/ESP8266LittleFS/tool/esp8266littlefs.jar
```
Den Sketchbookpfad bekommt man über die Einstellungen der Arduino IDE raus (Ctrl+,). Es ist der Ordner, in dem sich default mäßig alle Arduino Sketches befinden.

Arduino IDE Neustarten. Ordner des Wasserraketensketchs öffnen. In diesem Ordner leigt ein weiterer Ordner namens data. Nun kann in der Arduino IDE über Tools > ESP8266 LittleFS Data Upload der gesamte Inhalt des data Ordners hochgeladen werden.

Falls dies nicht gemacht wird, so wird eine leere / weiße Website angezeigt.

## Beschreibung

Der ESP8266 macht ein WLAN Netzwerk mit dem Namen `Wasserrakete` auf. Über die IP Adresse [10.10.10.10](http://10.10.10.10) auf Port `80` kann man auf eine Website zugreifen, über die man die aktuellen Messwerte sehen kann. Zusätzlich kann man eine Messung starten und wieder beenden. Nach dem Beenden wird automatisch eine `.csv`-Datei mit den gemessenen Werten herunter geladen.

### Import der csv-Datei in Excel

Da in der csv-Datei Kommata (`,`) als Trennzeichen der Werte und ein Punkt (`.`) als Trennzeichen bei "Kommazahlen" verwendet wird, kann das zu Problemen beim Import in Excel führen. Jedenfalls, wenn man Excel auf Deutsch gestellt hat. 

Hierzu gibt es zwei Möglichkeiten:

#### Möglichkeit 1 (Verbindung)

1. In Excel den Reiter `Daten` auswählen
2. `Externe Daten abrufen`
3. `Aus Text`
4. csv Datei auswählen
5. Es öffnet sich der Textkonvertiertungsassistent
6. Bei Schritt 1 sind keine Änderungen notwendig &rarr; `Weiter`
7. Bei Schritt 2 `Komma` als Trennzeichen auswählen &rarr; `weiter`
8. Bei Schritt 3 auf `Weitere` klicken. Es öffnet sich ein neues Fenster. Als Dezimaltrennzeichen `.`(Punkt) verwenden. Als 1000er-Trennzeichen `,` (Komma) verwenden &rarr; `Ok`
9. `Fertig stellen`
10. Wenn man die Daten als Tabelle (also mit der Möglichkeit Filter anzuwenden oder die Daten zu sortieren) haben möchte, das Häkchen bei `Dem Datenmodell diese Daten hinzufügen` setzen &rarr; `Ok`

#### Möglichkeit 2 (Abfrage)

Man kann das Gebietsschema für die Abfrage externen Dateien in Excel auf ein englisches Format stellt, dann hat man keine Probleme.  Diese Einstellung bezieht sich nur auf die aktuelle Arbeitsmappe. Sie muss also für jede Arbeitsmappe wiederholt werden.

##### Umstellen des Gebietsschemas

1. Excel öffnen
2. Reiter `Daten`
3. `Neue Abfrage` (Beachte Unterschied zu Externe Daten Abrufen)
4. `Abfrageoptionen` (Es öffnet sich ein neues Fenster)
5. `Regionale Einstellungen`
6. Gebietsschema auf `Englisch (USA)`stellen

Referenz: [Microsoft Forum](https://techcommunity.microsoft.com/t5/excel/decimal-separator-after-csv-import/m-p/134617)

##### Datei importieren

1. In Excel den Reiter `Daten` auswählen
2. `Neue Abfrage`
3. `Aus Datei`
4. `Aus CSV`
5. csv Datei auswählen
6. `Laden in..` auswählen um die Daten in dem aktuellen Tabellenblatt einzufügen. `Laden` fügt eine neue Tabellenseite hinzu

## Pinout

### BME280

Der BME280 ist ein Temperatur-, Luftdruck- und Luftfeuchtigkeitssensor.

![BME280 Pinout - Temperature Humidity Barometric Pressure Sensor](assets/BME280-Pinout-Temperature-Humidity-Barometric-Pressure-Sensor.png)

### Arduino ESP8266

![Arduino ESP8266 Pinout](assets/ESP_Pinout.png)

### Verbindung

| BME280 | ESP8266 |
| ------ | ------- |
| VCC    | 3.3 V   |
| GND    | GND     |
| SCL    | D1      |
| SDA    | D2      |



## Javascript

### Functions to populate IDs

Here some example code to change values:

```javascript
document.getElementById("state").innerHTML = "recording";
document.getElementById("temperature").innerHTML = "31.50";
document.getElementById("altitude").innerHTML = "655.0";
document.getElementById("pressure").innerHTML = "999.0";
```

Change values with format

```javascript
var value = 999.0;
document.getElementById("pressure").innerHTML = value.toFixed(2);
document.getElementById("pressure").innerHTML = value.toFixed(4);
```

### HTML special characters

ö - \&ouml;
° - \&deg;
