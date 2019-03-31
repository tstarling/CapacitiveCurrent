#include "CapacitiveCurrent.h"

const double REF_VOLTAGE = 1.1; // V
const double CAPACITANCE = 1.3; // nF
const double TIMEOUT = 20; // s
const long DISCHARGE_TIME = 10; // us

static uint32_t CapacitiveCurrent::m_value = 0;
static uint32_t CapacitiveCurrent::m_period = 0;
static uint16_t CapacitiveCurrent::m_highCount = 0;
static uint8_t CapacitiveCurrent::m_timerMode = CapacitiveCurrent::IDLE;

ISR(TIMER1_OVF_vect) {
	CapacitiveCurrent::onOverflow();
}

ISR(TIMER1_CAPT_vect) {
	CapacitiveCurrent::onCapture();
}

void CapacitiveCurrent::setup() {
	// Disable input buffer on AIN0 pin
	DIDR1 |= _BV(AIN0D);

	// Reset Timer1 configuration
	TCCR1A = 0;
	TCCR1B = 0;

	// Capture rising edge
	TCCR1B |= _BV(ICES1);

	// Enable input capture interrupt and overflow interrupt
	TIMSK1 |= _BV(ICIE1) | _BV(TOIE1);

	startDischargeCycle();
}

void CapacitiveCurrent::update() {
	if (m_timerMode == IDLE) {
		startChargeCycle();
	}
}

void CapacitiveCurrent::startChargeCycle() {
	stopTimer();
	m_timerMode = CHARGING;
	setTimerCounter(0);
	enableCapture();
	charge();
	startTimer();
}

void CapacitiveCurrent::onOverflow() {
	if (m_timerMode == DISCHARGING) {
		// Finished discharging, but don't start charging until update() is called
		stopTimer();
		m_timerMode = IDLE;
	} else {
		m_highCount++;
		if (m_highCount > F_CPU * TIMEOUT / 65536) {
			// Reached timeout
			stopTimer();
			setValue(0);
			startDischargeCycle();
		} else {
			// In the case of a sudden drop in current to near zero, the value
			// would be wrong for a long time if we waited for comparator capture.
			// So let's look at the top 16 bits of the counter, and if it suggests
			// a lower value than the stored one, we'll temporarily use it. The
			// response time to a drop in current will thus be <4ms.
			uint32_t maxValue = calculateValue((uint32_t)m_highCount << 16);
			if (m_value > maxValue) {
				m_value = maxValue;
			}
		}
	}
}

void CapacitiveCurrent::onCapture() {
	stopTimer();
	setValue(ICR1);
	startDischargeCycle();
}

void CapacitiveCurrent::setValue(uint16_t lowCount) {
	m_period = ((uint32_t)m_highCount << 16) | lowCount;
	m_value = calculateValue(m_period);
}

uint32_t CapacitiveCurrent::calculateValue(uint32_t period) {
	if (period == 0) {
		return 0xffffffffLL;
	} else {
		return (uint32_t)(CAPACITANCE * REF_VOLTAGE * F_CPU * 10) / period;
	}
}

void CapacitiveCurrent::startDischargeCycle() {
	m_timerMode = DISCHARGING;
	discharge();
	// Start the timer so that it overflows after the configured discharge time
	setTimerCounter((uint16_t)-(int16_t)(DISCHARGE_TIME * F_CPU / 1000000L));
	disableCapture();
	startTimer();
}

