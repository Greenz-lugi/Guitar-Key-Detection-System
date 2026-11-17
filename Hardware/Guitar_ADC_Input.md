# Guitar ADC Input Circuit

## Overview

The Guitar ADC Input Circuit is the current analogue front-end design for the Real-Time Guitar Audio Processing System.

Unlike the previous LED Signal Detection Circuit, which converted the incoming signal into a HIGH/LOW output, this design aims to preserve the analogue guitar waveform for digital signal processing.

The conditioned signal is supplied to an MCP3208 12-bit analogue-to-digital converter (ADC), which communicates with the BeagleY-AI through SPI.

The resulting digital samples can then be processed using C/C++ to analyse frequency content and identify musical notes and chords.

## System Architecture

The current signal path is:

$$
\boxed{
\text{Guitar}
\rightarrow
\text{AC Coupling and Biasing}
\rightarrow
\text{Amplification}
\rightarrow
\text{Low-Pass Filter}
\rightarrow
\text{MCP3208 ADC}
}
$$

The ADC then communicates with the BeagleY-AI for digital signal processing.

## AC Coupling and DC Biasing

The guitar produces an AC waveform that naturally oscillates around 0 V.

However, the ADC input must remain within its permitted voltage range. The current design therefore uses a DC bias to shift the waveform into a positive voltage range.

A voltage divider consisting of two 10 kΩ resistors is used to establish a midpoint reference from the 5 V supply.

The theoretical bias voltage is:

$$
V_{\text{bias}}=
V_{\text{CC}}\frac{R_2}{R_1+R_2}
$$

For equal resistor values:

$$
V_{\text{bias}}=
5\frac{10,000}{10,000+10,000}
$$

$$
\boxed{V_{\text{bias}}=2.5\text{ V}}
$$

The AC coupling capacitor blocks the original DC component of the guitar input while allowing the AC waveform to pass.

The resulting signal is:

$$
V_{\text{in}}(t)=2.5+v_{\text{guitar}}(t)
$$

The waveform is now centred around approximately 2.5 V rather than 0 V.

This is important because the MCP3208 is being used with a positive single-ended input range.

## Non-Inverting Amplification

The next stage uses an MCP6002 operational amplifier in a non-inverting configuration.

A non-inverting amplifier was selected to preserve the polarity of the incoming signal.

Although inverting an audio waveform would not change the musical notes or frequencies it contains, a non-inverting configuration maintains the original waveform orientation and simplifies signal interpretation throughout the analogue chain.

### Voltage Gain

For a conventional non-inverting amplifier:

$$
A_v=1+\frac{R_f}{R_g}
$$

The current design uses:

- Feedback resistor: \(R_f=10\,\text{k}\Omega\)
- Ground-leg resistor: \(R_g=1\,\text{k}\Omega\)

The theoretical AC gain is:

$$
A_v=1+\frac{10,000}{1,000}
$$

$$
\boxed{A_v=11}
$$

### Maintaining the DC Bias

The amplifier must increase the AC component without multiplying the 2.5 V DC bias by eleven.

To achieve this, the 1 kΩ resistor is connected to ground through a 10 µF capacitor.

At DC, the capacitor behaves approximately as an open circuit. The amplifier therefore approaches unity gain.

At sufficiently high AC frequencies, the capacitor behaves approximately as a short circuit, allowing the feedback network to produce a gain approaching eleven.

Consequently:

$$
A_{v,\text{DC}}\approx1
$$

$$
A_{v,\text{AC}}\approx11
$$

The approximate transition frequency of this network is:

$$
f_c=\frac{1}{2\pi R_g C}
$$

$$
f_c=
\frac{1}{2\pi(1,000)(10\times10^{-6})}
$$

$$
\boxed{f_c\approx15.9\text{ Hz}}
$$

This frequency is below the fundamental frequencies of the standard guitar strings, so the amplifier approaches its intended AC gain across the guitar's fundamental range.

### Design Consideration: Amplifier Headroom

Although the theoretical gain is eleven, the output cannot exceed the operational amplifier's available output range.

The signal must also remain within the ADC's input range.

Because the waveform is biased around 2.5 V and the ADC reference is intended to be 3.3 V, there is less positive voltage headroom than negative voltage headroom.

Therefore, the actual amplified waveform must be measured to ensure that it does not exceed the ADC's permitted input range.

Clipping at the amplifier or exceeding the ADC input limits would compromise the signal and could damage the ADC.

## Low-Pass Filtering

After amplification, the signal passes through a passive first-order RC low-pass filter.

The filter consists of:

- Series resistor: $R=220\,\Omega$
- Capacitor to ground: $C=100\,\text{nF}$

A low-pass filter allows lower-frequency components to pass while progressively attenuating higher-frequency components.

This is useful because the objective is to retain the guitar's musical frequency content while reducing unwanted high-frequency components before sampling.

### Why a Low-Pass Filter?

The guitar's fundamental frequencies are important for identifying musical notes.

For example, the lowest standard guitar string, E2, has a fundamental frequency of approximately 82.4 Hz.

The highest fundamental frequencies of interest are approximately 1.3 kHz.

However, a guitar signal also contains harmonics above its fundamental frequency. These harmonics contribute to the waveform's shape and timbre and can provide useful information during signal analysis.

A high-pass filter would attenuate frequencies below its cutoff, which is not desirable for preserving the lower guitar notes.

A low-pass filter is therefore used to attenuate higher-frequency content while retaining the lower-frequency fundamentals.

### Calculating the Cutoff Frequency

For a first-order RC low-pass filter:

$$
f_c=\frac{1}{2\pi RC}
$$

Substituting the selected component values:

$$
R=220\,\Omega
$$

$$
C=100\,\text{nF}=100\times10^{-9}\text{ F}
$$

Therefore:

$$
f_c=
\frac{1}{2\pi(220)(100\times10^{-9})}
$$

$$
\boxed{f_c\approx7,234\text{ Hz}}
$$

The resulting cutoff frequency is approximately:

$$
\boxed{f_c\approx7.23\text{ kHz}}
$$

At the cutoff frequency, the filter's output amplitude is approximately 70.7% of its passband amplitude, corresponding to a -3 dB point.

The filter does not completely remove frequencies above 7.23 kHz. Instead, it progressively attenuates them.

For a first-order RC filter, the roll-off is approximately:

$$
-20\text{ dB/decade}
$$

## Sampling and the MCP3208 ADC

The filtered analogue signal is connected to channel 0 of the MCP3208 ADC.

The MCP3208 is a 12-bit successive-approximation-register (SAR) ADC.

A 12-bit converter provides:

$$
2^{12}=4096
$$

possible digital output levels.

With an intended reference voltage of 3.3 V, the approximate voltage resolution is:

$$
\text{Resolution}=
\frac{V_{\text{REF}}}{2^{12}}
$$

$$
\text{Resolution}=
\frac{3.3}{4096}
$$

$$
\boxed{\text{Resolution}\approx0.806\text{ mV per count}}
$$

The actual conversion range is determined by the ADC's reference and ground connections.

The ADC communicates with the BeagleY-AI through SPI, allowing the software to request and retrieve digital samples.

### Sampling Frequency

Sampling frequency describes how many analogue measurements the ADC takes every second.

For example, a sampling frequency of:

$$
f_s=20,000\text{ samples/second}
$$

means that 20,000 individual voltage measurements are taken every second.

These measurements form a discrete-time representation of the original guitar waveform.

The sampling frequency is an important design parameter because it determines the highest frequency that can theoretically be represented without aliasing.

The current 20 kSPS value is a preliminary development target and must be verified against the ADC's SPI timing and achievable acquisition rate.

## Nyquist Frequency

The Nyquist-Shannon sampling theorem establishes that a sampled signal must be sampled at more than twice its highest frequency component to avoid aliasing.

The Nyquist frequency is:

$$
f_N=\frac{f_s}{2}
$$

For a sampling frequency of 20 kSPS:

$$
f_N=\frac{20,000}{2}
$$

$$
\boxed{f_N=10,000\text{ Hz}=10\text{ kHz}}
$$

This means that frequencies above 10 kHz cannot be uniquely represented by a 20 kSPS sampling system.

For comparison, a sampling frequency of 10 kSPS would give:

$$
f_N=\frac{10,000}{2}
$$

$$
\boxed{f_N=5\text{ kHz}}
$$

The sampling frequency must therefore be selected with consideration for the frequency content that needs to be preserved.

## Aliasing and Anti-Aliasing

Aliasing occurs when frequency components above the Nyquist frequency are sampled without sufficient attenuation.

These components can appear as incorrect lower frequencies in the resulting digital signal.

This is particularly important for musical instruments because the waveform contains harmonics above the fundamental frequency.

For example, a guitar note with a fundamental frequency of 440 Hz may also contain harmonics at:

$$
880\text{ Hz},\quad 1320\text{ Hz},\quad 1760\text{ Hz},\ldots
$$

Preserving some of these harmonics can be useful for analysing the signal.

However, frequency components above the sampling system's Nyquist frequency must be sufficiently attenuated before conversion.

This is the purpose of an anti-aliasing filter.

### Relationship Between the Filter and Sampling Frequency

The current low-pass filter has a cutoff frequency of approximately 7.23 kHz.

With the preliminary sampling target of 20 kSPS:

$$
f_N=10\text{ kHz}
$$

The filter cutoff is below the Nyquist frequency, but this alone does not guarantee adequate alias rejection.

A first-order filter has a relatively gradual roll-off, so frequencies around and above the Nyquist frequency are only partially attenuated.

The current filter is therefore a starting point for the prototype. Its effectiveness must be evaluated through testing, and a higher-order filter or a different cutoff may be required if stronger anti-aliasing is needed.

## Design Decisions and Current Implementation

The current design reflects several decisions made while developing the analogue input stage.

| Design consideration | Current approach | Reason |
|---|---|---|
| Input coupling | AC coupling | Removes the original input DC component |
| Operating point | 2.5 V bias | Allows positive single-supply signal processing |
| Amplifier configuration | Non-inverting | Preserves signal polarity |
| Amplifier gain | Approximately 11 for AC | Increases the signal amplitude |
| Filtering | First-order RC low-pass | Attenuates unwanted high-frequency components |
| Filter cutoff | Approximately 7.23 kHz | Retains useful lower-frequency content and some harmonics |
| ADC | MCP3208 | Converts the analogue waveform into 12-bit digital samples |
| Sampling target | 20 kSPS, preliminary | Provides a 10 kHz Nyquist frequency |

## Circuit Schematic and Demonstration

### Circuit Schematic

The following schematic illustrates the current analogue input design, including the AC coupling, DC biasing, amplification, and low-pass filtering stages.

<img width="509" height="347" alt="image" src="https://github.com/user-attachments/assets/9fd7ac46-5017-4fba-b296-2b52e740d0f5" />

Below here is the setup on the Breadboard, which when seen can look messy and is why we're considering the PCB route; however, we are students and it's finals season, so we may not reach that point.


<img width="551" height="373" alt="image" src="https://github.com/user-attachments/assets/e74e795d-2f2a-46a8-8e7a-b4ecc25aa323" />


And in the following link, we demo the guitar in action; we get a signal, and we're able to read most of it to find out which chord is played. There are occasional bugs and issues with recurring chords or signals that are echoed, but that's currently a work in progress. If you click on the link below, you can view the footage:

[Prototype 1 of the Guitar Key Detection System](https://drive.google.com/file/d/1wev5ISjBDpGWXXvGDHWWp2vrMQXHoieD/view?usp=sharing)

## Further Development

With the analogue input circuit developed and demonstrated, the project can now progress toward integrating the hardware with the digital signal-processing system.

The next development priorities are:

### Software Integration

- Establish communication between the MCP3208 ADC and the BeagleY-AI through SPI.
- Implement continuous sampling of the guitar input signal using C/C++.
- Process the acquired samples to analyse their frequency content.
- Develop and refine note and chord detection algorithms.

### Hardware Collaboration and Integration

- Communicate with the team member responsible for the OP777 circuit.
- Compare the two signal-conditioning approaches and evaluate their compatibility.
- Determine how the independently developed circuits can be integrated into the overall hardware system.

### PCB Development

- Transition the tested analogue input circuit from a breadboard implementation to a PCB.
- Organize component placement and routing.
- Consider power distribution, grounding, input connections, and signal integrity.
- Prepare the design for physical assembly and further testing.

## Conclusion

The Guitar ADC Input Circuit represents the transition from basic analogue signal detection toward waveform acquisition for digital signal processing.

The design combines AC coupling, DC biasing, non-inverting amplification, and low-pass filtering to prepare the guitar signal for conversion by the MCP3208 ADC.

With the analogue input design established, development can now focus on software integration, collaboration between hardware implementations, and PCB development.

This provides the foundation for identifying individual musical notes and, eventually, recognizing guitar chords and musical keys.
