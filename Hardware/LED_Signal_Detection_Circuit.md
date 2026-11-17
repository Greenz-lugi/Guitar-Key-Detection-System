# LED Signal Detection Circuit

## Overview

The LED Signal Detection Circuit was developed as an initial prototype to investigate how an incoming electric guitar signal could be detected using analogue electronics.

The objective was to create a circuit that responds to the presence of an incoming guitar signal by controlling an LED.

This prototype introduced several important concepts in analogue signal conditioning, including AC coupling, DC biasing, operational amplifier gain, and Schmitt trigger hysteresis.

## AC Coupling and DC Biasing

An electric guitar produces an alternating voltage signal. However, the operational amplifier circuit operates from a single supply and requires an appropriate operating point.

To accommodate this, the input signal is AC-coupled and biased around a DC reference voltage.

For a 5 V supply, a voltage divider consisting of two 10 kΩ resistors produces an ideal midpoint voltage:

$$
V_{\text{bias}} =
V_{\text{CC}}\frac{R_2}{R_1+R_2}
$$

Using equal resistor values:

$$
V_{\text{bias}} =
5\frac{10,000}{10,000+10,000}
$$

$$
\boxed{V_{\text{bias}}=2.5\text{ V}}
$$

The AC coupling capacitor blocks the incoming signal's DC component while allowing its changing AC component to pass.

The resulting signal is centred around the bias voltage:

$$
V_{\text{in}}(t)=V_{\text{bias}}+v_{\text{AC}}(t)
$$

This allows the guitar waveform to be processed using a single-supply circuit.

## Non-Inverting Amplification

The next stage amplifies the incoming AC signal.

A non-inverting amplifier was used to provide voltage gain while preserving the signal's polarity.

For a conventional non-inverting amplifier:

$$
A_v=1+\frac{R_f}{R_g}
$$

Using the selected resistor values:

- Feedback resistor: \(R_f=10\,\text{k}\Omega\)
- Ground-leg resistor: \(R_g=1\,\text{k}\Omega\)

The theoretical gain is:

$$
A_v=1+\frac{10,000}{1,000}
$$

$$
\boxed{A_v=11}
$$

### The DC Gain Problem

An important design issue appeared when considering the DC bias voltage.

If the amplifier amplifies both the AC signal and the DC bias, the 2.5 V operating point would also be multiplied by the gain.

For example:

$$
V_{\text{out,DC}}=11(2.5)
$$

$$
V_{\text{out,DC}}=27.5\text{ V}
$$

This output cannot be produced by an amplifier powered from a 5 V supply. The amplifier would saturate.

However, the desired behaviour was to amplify the changing AC waveform while maintaining the DC bias.

### Separating AC and DC Gain

The feedback network was configured with a 1 kΩ resistor in series with a 10 µF capacitor to ground.

At DC, the capacitor behaves approximately as an open circuit. The amplifier therefore approaches unity gain.

At sufficiently high AC frequencies, the capacitor behaves approximately as a short circuit, allowing the 1 kΩ resistor to determine the AC gain.

The gain consequently approaches:

$$
A_{v,\text{DC}}\approx 1
$$

$$
A_{v,\text{AC}}\approx 1+\frac{10\,\text{k}\Omega}{1\,\text{k}\Omega}
$$

$$
\boxed{A_{v,\text{AC}}\approx11}
$$

The approximate frequency associated with the feedback network is:

$$
f_c=\frac{1}{2\pi R_g C}
$$

Substituting the component values:

$$
f_c=
\frac{1}{2\pi(1,000)(10\times10^{-6})}
$$

$$
\boxed{f_c\approx15.9\text{ Hz}}
$$

This arrangement allows the DC operating point to remain near its original value while the AC component receives substantially greater gain.

This was an important design realization: the amplifier must be designed around the signal's AC behaviour without unintentionally amplifying its DC bias.

## Schmitt Trigger

After amplification, the signal is passed to a Schmitt trigger.

A Schmitt trigger is a comparator configuration that uses positive feedback to introduce hysteresis.

Instead of switching at one fixed voltage, it has two switching thresholds:

- Upper threshold: $V_{UT}$
- Lower threshold: $V_{LT}$

The output changes state when the input crosses one of these thresholds.

For example:

- When the input rises above the upper threshold, the output switches to its high state.
- When the input falls below the lower threshold, the output switches to its low state.
- Between the two thresholds, the output retains its previous state.

The difference between the thresholds is called the hysteresis voltage:

$$
\boxed{V_H=V_{UT}-V_{LT}}
$$

### Why Hysteresis Is Useful

An incoming guitar signal is not perfectly stable. It continuously changes with the waveform and may contain small fluctuations or noise.

Without hysteresis, a comparator operating near its switching threshold may rapidly alternate between HIGH and LOW due to small variations.

Hysteresis creates a defined switching region, making the output more stable and less sensitive to small fluctuations.

## LED Output Behaviour

The Schmitt trigger output controls the LED through a current-limiting resistor.

The LED provides a visual indication that the incoming guitar signal has crossed the switching thresholds.

The behaviour of the circuit depends on the relationship between the guitar waveform and the Schmitt trigger thresholds.

### Why the LED Can Remain On

An interesting behaviour observed during testing was that the LED could remain on even after the guitar signal stopped.

This is explained by hysteresis.

When the guitar signal disappears, the input returns toward its DC bias voltage. If this voltage lies between the upper and lower switching thresholds, the Schmitt trigger retains its previous output state.

The output therefore does not automatically return to LOW simply because the AC signal disappears.

The input must cross the opposite threshold to change the output state.

This is expected Schmitt trigger behaviour rather than necessarily indicating a fault.

### Relationship Between Guitar Frequency and LED Activity

The guitar signal is periodic.

For example, the lowest standard guitar string, E2, has a fundamental frequency of approximately:

$$
f=82.4\text{ Hz}
$$

Its waveform completes approximately 82.4 cycles every second.

The Schmitt trigger responds to threshold crossings within that waveform, so its output can switch rapidly as the guitar signal oscillates.

At these frequencies, the LED may appear to flicker, change brightness, or appear relatively steady depending on the signal, switching thresholds, and visual persistence.

The LED is therefore acting as a basic visual indicator of signal activity rather than displaying the individual musical notes.

## Physical Assembly and Input Connection

The circuit also required a physical connection between the electric guitar and the analogue input stage.

A female ¼-inch (6.35 mm) audio jack was soldered to provide the guitar input connection.

This allowed a standard guitar instrument cable to connect directly to the prototype.

The jack provides two electrical connections:

- **Tip:** Carries the guitar's audio signal.
- **Sleeve:** Provides the signal ground connection.

The soldered connections were incorporated into the breadboard prototype to establish a reliable physical input interface.

<img width="736" height="550" alt="image" src="https://github.com/user-attachments/assets/677670e6-e8a8-4ba3-8799-16e8bfc7eec3" />


![Soldered Female 1/4-inch Guitar Input Jack](<img width="736" height="550" alt="image" src="https://github.com/user-attachments/assets/bcb30383-50b0-4683-89b7-c1a5ed8e463f" />
)

This physical assembly connected the theoretical circuit design to a practical guitar input system and allowed the circuit to be tested using an actual electric guitar.

## 6. What This Prototype Established

This circuit provided a practical understanding of several important analogue electronics concepts:

- AC coupling separates an incoming AC signal from its original DC component.
- DC biasing establishes a suitable operating point for single-supply processing.
- Amplifier feedback determines the voltage gain.
- Capacitors can be used within feedback networks to create frequency-dependent gain.
- A Schmitt trigger uses hysteresis to provide stable threshold switching.
- An LED can provide a simple visual indication of signal activity.
- Physical soldering and input connections are necessary to integrate the circuit with an actual instrument.

## Conclusion

The LED Signal Detection Circuit was an important first step in the hardware development process.

It demonstrated how an analogue guitar signal could be conditioned, amplified, and converted into a digital-like HIGH/LOW output.

However, threshold detection does not preserve the complete waveform. Since the final objective is to analyse the frequency content of the guitar signal, a different approach is required.

This led to the development of the current Guitar ADC Input Circuit, which preserves the analogue waveform for sampling and digital signal processing.
