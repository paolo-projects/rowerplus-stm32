# RowerPlus MCU Firmware

STM32F401 MCU firmware for measuring the parameters of an ergometer. I made it to be interfaced to an Android app
through USB-OTG connection and serial communication.

This firmware is a general purpose firmware that you can use on any type of ergometer with a flywheel.
The only modification you have to make is adding four equally distanced (90° angle between them) magnets to the flywheel,
and adding one Hall sensor to the frame. Some ergometer models could already have the magnets and the hall sensor set up,
but in this case you probably don't need the app as the builtin computer should do its job.

This is a picture of the hall sensor applied to the frame of my ergometer, and the magnets glued to the flywheel:

<img src="/readme/hall_sensor.jpg" />

Here's a picture of the STM32 board, the AT24C256 EEPROM and the wiring:

<img src="/readme/wiring.jpg" />

## The Hall sensor

The Hall sensor I'm using is the A3144. It's a switch sensor, meaning that when the magnetic field is above a certain
treshold, the switch closes, and if the field drops under a treshold the switch opens again. This behavior can be 
detected through an external interrupt. 

The A3144 I purchased is, unwillingly, a latching sensor, meaning that the switch changes its state when a field 
of a certain polarity is over a certain treshold, and then keeps this state even if the field drops. The switch
state changes again when a field of opposite polarity gets detected, and so on. In other words, the sensor changes its state every time 
a magnetic field of polarity opposite from the last one gets above the treshold. To account for this behavior I
flipped two of the four magnets, and now I have them in this configuration: NSNS (supposing the first one is North polarity).

The angular velocity values measured from the hall sensor have a lot of noise. To remove the noise, an average filter is applied
to the measurements. Here you can see the difference:

<img src="/readme/angular_velocity_zoom.png" />

## The EEPROM

Since the quadratic fit for the damping constants has to have its R<sup>2</sup> > 0.9, it could take many strokes to compute the values.
Hence I added an EEPROM to store the values when the device is unplugged and read them when it's on again.
The memory is constantly updated every time new values are computed. This could be an issue as EEPROMs don't have too many write cycles,
so using a memory that supports more write cycles such as an SD card could be better.

## Why the STM32?

The STM32 has been chosen for the high clock speed. Additionally, the STM32F401RE has a FPU (floating point unit) 
that should speed up the floating point calculations inside the firmware. The device also supports USB device capabilities, and I'm working
on a version of the firmware that communicates through USB HID protocol, with the USB cable wired to the main board bypassing the STLink USB-Serial converter.

Direct USB communication should help when using boards with no embedded STLink. An alternative would be to set up the USB as a vcom and use serial communication
through on-board usb. If no STLink is available on the board, make sure there's a crystal to feed the HSE clock for the USB device.

## What does it measure?

The firmware measures the angular velocity of the flywheel by measuring the time elapsed between two magnet detections,
and knowing that the magnets are 90 deg apart we can get the angular velocity through the formula:

<img src="https://latex.codecogs.com/svg.image?\omega&space;=&space;\frac{(\pi&space;/&space;2)*10^6}{t_2&space;-&space;t_1}" title="https://latex.codecogs.com/svg.image?\omega = \frac{(\pi / 2)*10^6}{t_2 - t_1}" />

The time values in this formula are in microseconds.

I used this website as a reference for the calculations: [http://eodg.atm.ox.ac.uk/user/dudhia/rowing/physics/ergometer.html](http://eodg.atm.ox.ac.uk/user/dudhia/rowing/physics/ergometer.html)

The energy of the previous stroke is calculated at the start of a new one through this equation:

<img src="https://latex.codecogs.com/svg.latex?E%20%3D%20%5Csum_%7Bi%7D%7B%28%5Ctheta_i%20-%20%5Ctheta_%7Bi-1%7D%29%5BI%28%5Cfrac%7B%5Comega_i%20-%20%5Comega_%7Bi-1%7D%7D%7Bt_i%20-%20t_%7Bi-1%7D%7D%29%20&plus;%20K_a%5Comega_i%5E2%20&plus;%20K_m%20%5Comega_i%20&plus;%20K_s%5D%7D" />

The constant Ka is for the air damping, Km is for the magnetic damping and Ks accounts for the friction independent of the angular velocity.

These three values are calculated after every stroke by measuring the free deceleration of the flywheel, and fitting to this equation:

<img src="https://latex.codecogs.com/svg.latex?%5Cfrac%7Bdw%7D%7Bdt%7D%3D-%5Cfrac%7BK_a%7D%7BI%7D%5Comega%5E2-%5Cfrac%7BK_m%7D%7BI%7D%5Comega-%5Cfrac%7BK_s%7D%7BI%7D" />

If the fit is good, the current values are updated and stored into the EEPROM.

The distance is calculated through this formula:

<img src="https://latex.codecogs.com/svg.latex?D%20%3D%20%5Csum_%7Bi%7D%7B%28%5Ctheta_i%20-%20%5Ctheta_%7Bi-1%7D%29%5Csqrt%5B3%5D%7B%5Cfrac%7BK_a%7D%7Bc%7D&plus;%5Cfrac%7BK_m%7D%7Bc%5Ccdot%5Comega_i%7D&plus;%5Cfrac%7BK_s%7D%7Bc%5Ccdot%20%5Comega_i%5E2%7D%7D%7D" />

Where `c` is a constant of the water resistance for an hypothetical boat motion, that I have taken from the link above.

---

- The configuration is stored in the `config.h` header file.

- The computed values are the energy spent (in KCal), the distance traveled (in meters) and the average stroke power (in Watts).

	*Notes*
	
	The energy in KCal is an approximation of the energy spent by the rower, while the stroke power is an approximation of just the energy spent
	to accelerate the flywheel. The difference is that the first value takes into account (with a big approximation) the energy burned by the
	body for the entire movement, while the second one is just the energy that actually gets to the flywheel.

## The Moment of Inertia

To use the previous equations, an exact value of the moment of inertia of the flywheel is needed. I disassembled my ergometer until the last piece
to get my hands on the flywheel alone, and by approximating its shape I calculated the moment of inertia. 

The problem was that the calories and distance values were too low, so I further approximated the shape of the flywheel to a solid cylinder and calculated a new moment of inertia,
which turned out to be close to 3 times the previous one. The values were still too low, so I arbitrarily multiplied this last value by 3 and finally
got values in line with other ergometers.

If you want to replicate this project, this is the trickiest step to do, as the flywheel is generally linked to the handle through a belt and it takes a long
time to take it apart. When you have it in your hands, use an approximation of the shape, you can try with a combination of simple 3D shapes, and calculate the moment of intertia.

## USB-UART Communication

The device communicates with the android app through the builtin STLink USB-Serial interface.
The data is sent through this packet structure:

```c++
 typedef struct {
	char bt[4];
	float32_t energy;
	float32_t mean_power;
	float32_t distance;
	uint16_t checksum;
	char pd[2];
	char et[4];
} out_data_t;
```

`bt` are two begin-trasmission control characters set to "BT", while `et` are end-transmission control characters set to "ET". The other 2+2 bytes are for alignment. The energy field is the float energy value in KCal. The same applies to the power (in Watts), and the distance (in meters).

The checksum field is a checksum of the entire structure with the checksum field set to 0. The byte order used by the STM32 board is little-endian.

## Stroke and distance value

Since the params are sent as soon as the flywheel starts decelerating, the distance value cannot be calculated in its entirety, since it needs the values 
from the decelerating phase.
To fix this, after the decelerating phase is over, when the firmware calculates the points for the damping constants, it calculates the "excess"
distance for the decelerating phase and adds it to the next stroke. So each stroke's distance is partially made by the current stroke's accelerating phase, and the previous stroke's decelerating phase.

## License

This software is released under GNU GPLv3 License. See the LICENSE file for more options.
