# Arduino based telephone caller ID display unit

This project is a simple Caller ID (CLI) decoder made using the [Arduino UNO](https://www.arduino.cc/en/Main/ArduinoBoardUno) and a custom-made [HT9032D](https://www.holtek.com/documents/10179/116711/HT9032C_Dv170.pdf) module.

## HT9032D module

The HT9032D module design files are available at *[/ht9032d-module](https://github.com/dilshan/arduino-caller-id/ht9032d-module) directory*. *Gerber files* for PCB production are in the [Release](https://github.com/dilshan/arduino-caller-id/releases) section of this repository.

The HT9032D module consists of standard through-hole type components. No special soldering equipment is needed to assemble this module.

The following table lists all the components needed to complete the HT9032D module.

| Designator | Value                                                                                                                         | Quantity |
|------------|-------------------------------------------------------------------------------------------------------------------------------|----------|
| C1, C3     | 0.01μF / 400V                                                                                                                 | 2        |
| C2         | 0.1μF / 50V                                                                                                                   | 1        |
| C4, C5     | 33pF                                                                                                                          | 2        |
| C6, C7     | 0.22μF / 400V                                                                                                                 | 2        |
| D1         | [2W10](https://octopart.com/2w10g-e4%2F51-vishay-42173035) Bridge rectifier                                                   | 1        |
| R1, R2     | 200KΩ (¼W Carbon film resistor - 5%)                                                                                          | 2        |
| R3         | 22KΩ (¼W Carbon film resistor - 5%)                                                                                           | 1        |
| R4         | 10MΩ (¼W Carbon film resistor - 5%)                                                                                           | 1        |
| R5         | 470KΩ (¼W Carbon film resistor - 5%)                                                                                          | 1        |
| R6         | 18KΩ (¼W Carbon film resistor - 5%)                                                                                           | 1        |
| R7         | 15KΩ (¼W Carbon film resistor - 5%)                                                                                           | 1        |
| U1         | [HT9032D](https://octopart.com/ht9032d-holtek-20921619) (DIP-8)                                                               | 1        |
| Y1         | 3.58MHz (HC49) Crystal                                                                                                        | 1        |
| J1         | RJ11 / 6P6C, 6-pin, right angle, PCB socket ([Molex 52018-6616](https://octopart.com/52018-6616-molex-7664937) or equivalent) | 1        |
| J2         | 5-pin 2.54mm Pin header                                                                                                       | 1        |

## Assembling the Caller ID display unit

The following components and modules are required to assemble the call ID display unit.

1.  *Assembled HT9032D module*
2.  *Arduino UNO* board
3.  [16x2 HD44780 character LCD panel](https://www.adafruit.com/product/181)
4.  [2N3904](https://octopart.com/2n3904tf-on+semiconductor-84408862) (or equivalent NPN) Transistor
5.  50KΩ Trimpot (RM-063 or equivalent)
6.  560Ω (¼W) resistor
7.  22KΩ (¼W) resistor
8.  Few [mini/small breadboard(s)](https://www.adafruit.com/product/65)

Both *pictorial* and *symbolic* wire diagrams are provided to minimize the complexity of assembly.

The Arduino sketch required for this caller ID is available here.

The current version of this caller ID decoder sketch can handle the caller line ID, name, and date/time MDMF (*Multiple Data Message Format*) packet IDs only.

## License

All the source codes in this repository are released under the terms of the [MIT License](https://github.com/dilshan/arduino-caller-id/blob/main/LICENSE). The *HT9032D module* design files are released under the [CERN-OHL-W Version 2.0](https://ohwr.org/cern_ohl_w_v2.txt) License.

