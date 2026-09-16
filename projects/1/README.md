# Miniproject 1 - cotnrolling relay switch via the photoresistor and transistor.

## Setup

![Setup](doc/setup.jpg)

## Explanation

Photoresistor detects the level of lighting. The base of the transistor is controlled via the other output, which in turns swithces on and off the relay, which powers a diode.

Notes:
* the 5Vin on my ESP32 pin is not working (which could have powered the primary circuit of the relay), so the primary and secondary circuit of the relay is controlled from the same power board.

* the green diode on the relay is not working (the indicator of switching)

## Demo

https://github.com/user-attachments/assets/026c4b06-9d8b-4b44-b3ce-fe1d2a8c781b

