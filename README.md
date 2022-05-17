# RowerPlus MCU Firmware

STM32F401 MCU firmware for measuring the parameters of an ergometer. I made it to be interfaced to an Android app
through USB-OTG connection and serial communication.

This firmware is a general purpose firmware that you can use on any type of ergometer with a flywheel.
The only modification you have to make is adding four equally distanced (90° angle between them) magnets to the flywheel,
and adding a Hall sensor to the frame. Some ergometer models could already have the magnets and the hall sensor set up,
but in this case you probably don't need the app as the builtin computer should do its job.

This is a picture of the hall sensor applied to the from of my ergometer, and the magnets glued to the flywheel:

<img src="/readme/hall_sensor.jpg" />

Here's a picture of the STM32 board, the AT24C256 EEPROM and the wirings:

<img src="/readme/wiring.jpg" />

## The Hall sensor

The Hall sensor I'm using is the A3144. It's a switch sensor, meaning that when the magnetic field is over a certain
treshold, the switch closes, and if the field drops under a treshold the switch opens again. This behavior can be 
detected through an external interrupt. 

The A3144 I purchased is, unwillingly, a latching sensor, meaning that the switch changes its state when a field 
of a certain polarity is over a certain treshold, and then keeps this state even if the field drops. The switch
state changes again when a field of opposite polarity gets detected, and so on. To account for this behavior I
flipped two of the four magnets, and now I have it in this configuration: NSNS (supposing the first one is North polarity).

The angular velocity values measured from the hall sensor have a lot of noise. To remove the noise, an average filter is applied
to the measurements. Here you can see the difference:

<img src="/readme/angular_velocity_zoom.png" />

## The EEPROM

Since it takes many strokes to compute the damping constants, I added an EEPROM to store the values when the device is unplugged.
The memory is constantly updated every time new values are computed.

## Why the STM32?

The STM32 has been chosen for the high clock speed. Additionally, the STM32F401RE has a FPU (floating point unit) 
that should speed up the floating point calculations inside the firmware.

## What does it measure?

The firmware measures the angular velocity of the flywheel by measuring the time elapsed between two magnet detections,
and knowing that the magnets are 90 deg apart we can get the angular velocity through the formula:

<img src="https://latex.codecogs.com/svg.image?\omega&space;=&space;\frac{(\pi&space;/&space;2)*1000}{t_2&space;-&space;t_1}" title="https://latex.codecogs.com/svg.image?\omega = \frac{(\pi / 2)*1000}{t_2 - t_1}" />

The time values in this formula are in milliseconds.

---

I used this website as a reference for the other calculations: [http://eodg.atm.ox.ac.uk/user/dudhia/rowing/physics/ergometer.html](http://eodg.atm.ox.ac.uk/user/dudhia/rowing/physics/ergometer.html)

- The configuration is stored in the `config.h` header file.

- The computed values are the energy spent (in Joules), the distance traveled (in meters) and the average stroke power.