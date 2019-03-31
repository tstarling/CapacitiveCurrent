#ifndef CAPACITIVE_CURRENT_H
#define CAPACITIVE_CURRENT_H

#include <Arduino.h>

/**
 * A class for measuring a small current by using it to charge a capacitor
 * (suggested 1nF). The low side of the capacitor is connected to ground, the
 * high side is connected to the current source and the microcontroller's analog
 * comparator input (AIN0, Arduino Leonardo digital pin 7).
 *
 * We use Timer1's capture module to get the time at which the comparator
 * switches, accurate to one system clock cycle. Note that this requires
 * exclusive use of Timer1. Do not use analogOutput() on Leonardo pin 9 or 10
 * since this will reconfigure Timer1.
 *
 * To use the class, create an object, call setup() initially, then call
 * update() on it regularly. Some time after update() is called (up to 20
 * seconds), the value returned by getValue() will be updated.
 *
 * Example:
 *
 *     CapacitiveCurrent cc;
 *
 *     void setup() {
 *       cc.setup();
 *     }
 *
 *     void loop() {
 *       cc.update();
 *       uint32_t value = cc.getValue();
 *       ...
 *     }
 *
 * It doesn't make sense to have more than one CapacitiveCurrent object, since
 * there is only one analog comparator and one timer connected to it. The
 * internal data is static, so all object instances actually access the same
 * data.
 */
class CapacitiveCurrent {
public:
	CapacitiveCurrent() {}

	/** Set up the device */
	void setup();
	
	/**
	 * Start a charge cycle, if one is not running already. This should be
	 * called regularly to update the value.
	 */
	void update();

	/**
	 * Get the current in units of 0.1 nA
	 */
	uint32_t getValue() {
		cli();
		uint32_t value = m_value;
		sei();
		return value;
	}

	/**
	 * Get the most recent cycle count
	 */
	uint32_t getPeriod() {
		return m_period;
	}

	/**
	 * Call this from the TIMER1_OVF interrupt
	 */
	static void onOverflow();

	/**
	 * Call this from the TIMER1_CAPT interrupt
	 */
	static void onCapture();

private:
	/**
	 *  Set the pin direction to input, so that the capacitor starts charging
	 */
	static void charge() {
		DDRE &= ~_BV(DDE6);
	}

	/**
	 * Set the pin direction to output, so that the capacitor starts discharging
	 */
	static void discharge() {
		DDRE |= _BV(DDE6);
	}

	/**
	 * In the analog comparator, enable timer capture
	 */
	static void enableCapture() {
		ACSR |= _BV(ACIC);	
	}

	/**
	 * In the analog comparator, disable timer capture
	 */
	static void disableCapture() {
		ACSR &= ~_BV(ACIC);
	}

	/**
	 * Set the Timer1 counter value
	 */
	static void setTimerCounter(uint32_t value) {
		TCNT1 = value & 0xffff;
		m_highCount = value >> 16;
	}

	/**
	 * Stop Timer1
	 */
	static void stopTimer() {
		TCCR1B &= ~7;
	}

	/**
	 * Start Timer1 with the prescaler set to 1, so that the counter is
	 * incremented every system clock cycle.
	 */
	static void startTimer() {
		TCCR1B |= _BV(CS10);
	}

	/**
	 * Update the current estimate, given the low word of the timer counter.
	 * The high word is in m_highCount.
	 */
	static void setValue(uint16_t low);

	/**
	 * Calculate the value from the period
	 */
	static uint32_t calculateValue(uint32_t period);

	/**
	 * Switch the pin to charge mode and time how long it takes for the analog
	 * comparator to go high.
	 */
	static void startChargeCycle();

	/**
	 * Switch the pin to discharge mode and wait for 10us for discharging
	 */
	static void startDischargeCycle();

	/**
	 * The estimated current in units of 0.1nA 
	 */
	static uint32_t m_value;

	/**
	 * The last integration period in clock cycles. This is not essential and
	 * could be removed to save memory.
	 */
	static uint32_t m_period;

	/**
	 * The high word of the simulated 32-bit counter
	 */
	static uint16_t m_highCount;

	/**
	 * Constants for the state of the timer
	 */
	enum {
		IDLE,
		CHARGING,
		DISCHARGING
	};
	
	static uint8_t m_timerMode;
};

#endif
