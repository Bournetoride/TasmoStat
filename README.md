# TasmoStat
Modified XDV39 Thermostat to work with Solar Hot Water with 3 DS18B20 :- Manifold, Tank Top, Tank Bottom
Morning setpoint is Tank Top + Hysteresis (4C)
Afternoon setpoint gradually moves to Tank Bottom + 2* Hysteresis up to senset minute or minute 1111 when sun leaves manifold
DS18B20s are on separate Inputs so they can easily be assigned without reference to serial ID number
DS18B20-1 is Manifold
DS18B20-2 is Tank Top
DS18B20-3 is Tank Bottom
