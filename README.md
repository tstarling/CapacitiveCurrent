An Arduino library for measuring a small current by using it to charge a capacitor
(suggested 1nF). The low side of the capacitor is connected to ground, the
high side is connected to the current source and the microcontroller's analog
comparator input (AIN0, Arduino Leonardo digital pin 7).

This can be used to measure ambient light with a phototransistor. A
phototransistor provides a current proportional to light intensity, with
very little dependence on voltage.

We use Timer1's capture module to get the time at which the comparator
switches, accurate to one system clock cycle. Note that this requires
exclusive use of Timer1. Do not use analogOutput() on Leonardo pin 9 or 10
since this will reconfigure Timer1.

To use the class, create an object, call setup() initially, then call
update() on it regularly. Some time after update() is called (up to 20
seconds), the value returned by getValue() will be updated.

Example:

```
    CapacitiveCurrent cc;

    void setup() {
      cc.setup();
    }

    void loop() {
      cc.update();
      uint32_t value = cc.getValue();
      ...
    }
```

It doesn't make sense to have more than one CapacitiveCurrent object, since
there is only one analog comparator and one timer connected to it. The
internal data is static, so all object instances actually access the same
data.

You will have to modify CapacitiveCurrent.cpp to configure the capacitance.
When the capacitance is correct, getValue() will return the current in
units of 0.1nA.

The value will theoretically overflow its 32-bit limit if the
current is more than 429mA, however, it is not possible to measure such a large
current with this circuit, since the microcontroller would not be able to
discharge the capacitor while the current source is still connected, and
damage to microcontroller would probably result. This design is useful for
measuring currents from ~1nA to ~1mA.
