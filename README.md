# hygge
Arduino-based humidor temperature and humidity monitoring system.

I am a nerd who enjoys cigars, and this is my fun project, so expect it to be both hacky and over-the-top.

This is being developed in multiple phases:

1. [x] A thing that lives inside the humidor. Open the humidor to see the humidity.
2. [ ] Two things that do the same work...
  1. [ ] A thing that lives inside the humidor, and sends periodic reports wirelessly.
  2. [ ] A thing that lives on a desk (or wherever), receives the reports, and displays the values.
3. [ ] Three things that are collectively overengineered.
  1. [ ] A thing that lives inside the humidor, and sends periodic reports wirelessly.
  2. [ ] A thing that lives...somewhere, is network-attached, receives the reports, and stores the values in a time series database (probably influxdb).
  3. [ ] A thing that lives on a desk (or wherever), periodically queries Thing-3.2's time series database, and displays the values.
  4. [ ] A web app, probably just Grafana, served by Thing-3.2, providing improved visualization of the data, including an over-time view to see how well the humidor is performing.
4. [ ] Supplement Things-3.x with an additional monitor just outside the humidor so that we have an inside/outside metric for humidor performance. (e.g. it does well until outer humidity drops below 20%RH, when it starts to struggle, or whatevs.)

## Phase 1 - Thing-1

Thing-1 has already been developed, and consists of two components (less the USB travel battery currently powering the thing):

1. For the processor board, I chose the [MagTag by Adafruit](https://www.adafruit.com/product/4800), because of its forward compatibility with my goals.
2. For the temperature and humidity sensor, I chose the [Adafruit SHTC3 breakout board](https://www.adafruit.com/product/4636), because I had one on hand. For the next hardware iteration, I will switch to the [Adafruit SHT31-D breakout](https://www.adafruit.com/product/2857).

## Phase 2

Qi power, magnet mounts, 433 LoRa...

### Appease the FCC...

...with documentation of the wireless protocol that I'll be using.

#### Modulation

**Let it be known** that LoRa:registered: is a registered trademark of [LoRa Alliance](https://lora-alliance.org/). I'm stating this here to avoid potential issues by not appending :registered: to every mention of the term, because I'm lazy and don't like dealing with lawyers.

Transmitted data will use the very efficient, proprietary LoRa modulation on 433MHz between +0dBm and +20dBm, most likely using a Semtech chipset.

#### Data Format

**TODO:** Move this piece to its own repository.

Because I use LoRa for multiple purposes, I'm formalizing both my message encapsulation scheme, as well as the specific message definition for this piece of the project.

**Message Encapsulation Format (MEF)**

All of my wireless messages follow the following format:

| Name         | Description                                           | Byte Length     |
|--------------|-------------------------------------------------------|-----------------|
| Callsign     | Amateur radio callsign in plaintext. Case-insensitive | 4 - 6 bytes     |
| Message Type | Message type                                          | 2 bytes         |
| Data         | Message payload                                       | 1 or more bytes |

- Message type, for the purposes of this project, is `0x02`
- Overall MEF message length should be as short as possible, and not exceed a total of 255 bytes.

**Message Type 0x02**

Name: Temperature and Humidity
Format: two 32-bit floats, little endian; the first is temperature, and the second is humidity.
